#include "update_profile.h"
#include "third_party/hde64/hde64.h"
#include <CommonCrypto/CommonDigest.h>
#include <mach-o/nlist.h>
#include <mach/vm_prot.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <mach-o/fixup-chains.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    uint64_t image_offset;
    uint8_t expected[12];
} CompatPatch;

typedef struct {
    uint64_t target_offset;
    uint8_t kind;
    uint32_t expected_count;
} CompatReference;

typedef struct {
    uint8_t route;
    const char* name;
    uint32_t expected_count;
} CompatQuery;

#include "generated_update_rules.h"

enum {
    REF_CALL = 1,
    REF_JUMP = 2,
    REF_ADDRESS = 3,
    REF_IMMEDIATE = 4,
    REF_POINTER = 5,
    ROUTE_GIPA = 1,
    ROUTE_GDPA = 2,
};

typedef struct {
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    vm_prot_t prot;
} Segment;

typedef struct {
    const uint8_t* data;
    size_t size;
    uint64_t base;
    Segment segments[64];
    uint32_t segment_count;
    uint64_t text_address;
    uint64_t text_size;
    uint64_t text_offset;
    uint32_t fixups_offset;
    uint32_t fixups_size;
    uint32_t rebase_offset;
    uint32_t rebase_size;
    uint64_t old_start, old_end, gipa, gdpa;
    uint32_t functions_offset, functions_size;
    uint32_t symoff, nsyms, stroff, strsize, indirectoff, nindirect;
    struct section_64 stubs;
    uint8_t uuid[16];
    unsigned uuid_count;
} Image;

static bool range_ok(size_t size, uint64_t offset, uint64_t length) {
    return offset <= size && length <= size - offset;
}

static bool parse_image(const uint8_t* data, size_t size, Image* image) {
    if (!range_ok(size, 0, sizeof(struct mach_header_64))) {
        return false;
    }
    const struct mach_header_64* header =
        (const struct mach_header_64*)data;
    if (header->magic != MH_MAGIC_64 || header->cputype != CPU_TYPE_X86_64) {
        return false;
    }
    memset(image, 0, sizeof(*image));
    image->data = data;
    image->size = size;
    image->base = UINT64_MAX;
    if (header->ncmds > 4096 || !range_ok(size, sizeof(*header), header->sizeofcmds)) return false;
    uint64_t cursor = sizeof(*header);
    for (uint32_t index = 0; index < header->ncmds; ++index) {
        if (!range_ok(size, cursor, sizeof(struct load_command))) {
            return false;
        }
        const struct load_command* command =
            (const struct load_command*)(data + cursor);
        if (command->cmdsize < sizeof(*command) ||
            !range_ok(size, cursor, command->cmdsize) ||
            cursor + command->cmdsize > sizeof(*header) + header->sizeofcmds) {
            return false;
        }
        if (command->cmd == LC_SEGMENT_64) {
            const struct segment_command_64* segment =
                (const struct segment_command_64*)command;
            if (command->cmdsize < sizeof(*segment) ||
                image->segment_count >= 64) {
                return false;
            }
            Segment* saved = &image->segments[image->segment_count++];
            saved->vmaddr = segment->vmaddr;
            saved->vmsize = segment->vmsize;
            saved->fileoff = segment->fileoff;
            saved->filesize = segment->filesize;
            saved->prot = segment->initprot;
            if (!range_ok(size, saved->fileoff, saved->filesize)) return false;
            if (segment->filesize != 0 &&
                segment->vmaddr >= segment->fileoff &&
                segment->vmaddr - segment->fileoff < image->base) {
                image->base = segment->vmaddr - segment->fileoff;
            }
            const struct section_64* section =
                (const struct section_64*)(segment + 1);
            if (sizeof(*segment) +
                    (uint64_t)segment->nsects * sizeof(*section) >
                command->cmdsize) {
                return false;
            }
            for (uint32_t section_index = 0;
                 section_index < segment->nsects; ++section_index) {
                if ((section[section_index].flags & SECTION_TYPE) == S_SYMBOL_STUBS) image->stubs = section[section_index];
                if (strncmp(section[section_index].segname, "__TEXT", 16) == 0 &&
                    strncmp(section[section_index].sectname, "__text", 16) == 0) {
                    image->text_address = section[section_index].addr;
                    image->text_size = section[section_index].size;
                    image->text_offset = section[section_index].offset;
                }
            }
        } else if (command->cmd == LC_UUID) {
            if (command->cmdsize != sizeof(struct uuid_command)) return false;
            if (++image->uuid_count != 1) return false;
            memcpy(image->uuid, ((const struct uuid_command*)command)->uuid, 16);
        } else if (command->cmd == LC_SYMTAB) {
            if (command->cmdsize < sizeof(struct symtab_command)) return false;
            const struct symtab_command *s = (const void*)command;
            image->symoff=s->symoff; image->nsyms=s->nsyms;
            image->stroff=s->stroff; image->strsize=s->strsize;
        } else if (command->cmd == LC_DYSYMTAB) {
            if (command->cmdsize < sizeof(struct dysymtab_command)) return false;
            const struct dysymtab_command *s = (const void*)command;
            image->indirectoff=s->indirectsymoff; image->nindirect=s->nindirectsyms;
        } else if (command->cmd == LC_FUNCTION_STARTS) {
            if (command->cmdsize < sizeof(struct linkedit_data_command)) return false;
            const struct linkedit_data_command *s=(const void*)command;
            image->functions_offset=s->dataoff; image->functions_size=s->datasize;
        } else if (command->cmd == LC_DYLD_CHAINED_FIXUPS) {
            const struct linkedit_data_command* fixups =
                (const struct linkedit_data_command*)command;
            if (command->cmdsize < sizeof(*fixups)) {
                return false;
            }
            image->fixups_offset = fixups->dataoff;
            image->fixups_size = fixups->datasize;
        } else if (command->cmd == LC_DYLD_INFO ||
                   command->cmd == LC_DYLD_INFO_ONLY) {
            const struct dyld_info_command* info =
                (const struct dyld_info_command*)command;
            if (command->cmdsize < sizeof(*info)) {
                return false;
            }
            image->rebase_offset = info->rebase_off;
            image->rebase_size = info->rebase_size;
        }
        cursor += command->cmdsize;
    }
    return image->uuid_count == 1 && image->base != UINT64_MAX && image->text_size != 0 &&
           range_ok(size, image->text_offset, image->text_size);
}

static const uint8_t* vm_pointer(const Image* image, uint64_t address,
                                 uint64_t length) {
    for (uint32_t index = 0; index < image->segment_count; ++index) {
        const Segment* segment = &image->segments[index];
        if (address >= segment->vmaddr &&
            address - segment->vmaddr <= segment->filesize &&
            length <= segment->filesize - (address - segment->vmaddr)) {
            uint64_t offset = segment->fileoff + address - segment->vmaddr;
            return range_ok(image->size, offset, length)
                       ? image->data + offset
                       : NULL;
        }
    }
    return NULL;
}

static bool in_old_text(const Image* image, uint64_t address) {
    uint64_t start = image->base + image->old_start;
    uint64_t end = image->base + image->old_end;
    return address >= start && address < end;
}

static int instruction_boundary(const Image*, uint64_t);

static bool record_reference(const Image* image, uint64_t source,
                             uint64_t target, uint8_t kind,
                             uint32_t* counts) {
    if (!in_old_text(image, target) || in_old_text(image, source)) {
        return true;
    }
    /* Only an instruction proven to contain the scanned opcode is ignored. */
    if (kind != REF_POINTER) {
        int boundary = instruction_boundary(image, source);
        if (boundary < 0) {
            fprintf(stderr,"instruction boundary unavailable at 0x%llx\n",(unsigned long long)(source-image->base));
            return false;
        }
        if (boundary == 0) return true;
    }
    uint64_t target_offset = target - image->base - image->old_start + COMPAT_OLD_TEXT_START;
    for (size_t index = 0; index < COMPAT_REFERENCE_COUNT; ++index) {
        if (kCompatReferences[index].target_offset == target_offset &&
            kCompatReferences[index].kind == kind) {
            ++counts[index];
            return true;
        }
    }
    fprintf(stderr,
            "unexpected external MoltenVK reference target=0x%llx kind=%u\n",
            (unsigned long long)target_offset, kind);
    return false;
}

static bool scan_text_references(const Image* image, uint32_t* counts) {
    const uint8_t* code = image->data + image->text_offset;
    size_t size = image->text_size;
    bool valid = true;
    for (size_t offset = 0; offset < size; ++offset) {
        uint64_t source = image->text_address + offset;
        if (offset + 5 <= size && (code[offset] == 0xe8 || code[offset] == 0xe9)) {
            int32_t displacement = 0;
            memcpy(&displacement, code + offset + 1, sizeof(displacement));
            valid &= record_reference(
                image, source, source + 5 + displacement,
                code[offset] == 0xe8 ? REF_CALL : REF_JUMP, counts);
        }
        if (offset + 7 <= size && code[offset] >= 0x48 &&
            code[offset] <= 0x4f && (code[offset] & 0x08) != 0 &&
            code[offset + 1] == 0x8d && (code[offset + 2] & 0xc7) == 0x05) {
            int32_t displacement = 0;
            memcpy(&displacement, code + offset + 3, sizeof(displacement));
            valid &= record_reference(image, source,
                                      source + 7 + displacement,
                                      REF_ADDRESS, counts);
        }
        if (offset + 10 <= size && code[offset] >= 0x48 &&
            code[offset] <= 0x4f && (code[offset] & 0x08) != 0 &&
            code[offset + 1] >= 0xb8 && code[offset + 1] <= 0xbf) {
            uint64_t target = 0;
            memcpy(&target, code + offset + 2, sizeof(target));
            valid &= record_reference(image, source, target,
                                      REF_IMMEDIATE, counts);
        }
    }
    return valid;
}

static bool scan_chain(const Image* image, const Segment* segment,
                       uint16_t pointer_format, uint64_t chain_offset,
                       uint32_t* counts) {
    if (pointer_format != DYLD_CHAINED_PTR_64 &&
        pointer_format != DYLD_CHAINED_PTR_64_OFFSET) {
        fprintf(stderr, "unsupported chained pointer format: %u\n", pointer_format);
        return false;
    }
    bool valid = true;
    for (;;) {
        if (chain_offset > segment->filesize || sizeof(uint64_t) > segment->filesize-chain_offset) return false;
        uint64_t file_offset = segment->fileoff + chain_offset;
        if (!range_ok(image->size, file_offset, sizeof(uint64_t))) {
            return false;
        }
        uint64_t raw = 0;
        memcpy(&raw, image->data + file_offset, sizeof(raw));
        uint64_t next = (raw >> 51) & 0xfff;
        bool bind = (raw >> 63) != 0;
        if (!bind) {
            uint64_t target = raw & ((1ULL << 36) - 1);
            uint64_t high8 = (raw >> 36) & 0xff;
            if (pointer_format == DYLD_CHAINED_PTR_64_OFFSET) {
                target += image->base;
            } else {
                target |= high8 << 56;
            }
            uint64_t source = segment->vmaddr + chain_offset;
            valid &= record_reference(
                image, source, target, REF_POINTER, counts);
        }
        if (next == 0) {
            return valid;
        }
        if (chain_offset > segment->filesize ||
            next * 4 > segment->filesize - chain_offset) {
            return false;
        }
        chain_offset += next * 4;
    }
}

static bool scan_fixups(const Image* image, uint32_t* counts) {
    if (image->fixups_size == 0) {
        return true;
    }
    if (!range_ok(image->size, image->fixups_offset, image->fixups_size) ||
        image->fixups_size < sizeof(struct dyld_chained_fixups_header)) {
        return false;
    }
    const uint8_t* payload = image->data + image->fixups_offset;
    const struct dyld_chained_fixups_header* header =
        (const struct dyld_chained_fixups_header*)payload;
    if (header->starts_offset >= image->fixups_size) {
        return false;
    }
    const struct dyld_chained_starts_in_image* starts =
        (const struct dyld_chained_starts_in_image*)(payload + header->starts_offset);
    uint64_t starts_size = sizeof(uint32_t) +
        (uint64_t)starts->seg_count * sizeof(uint32_t);
    if (!range_ok(image->fixups_size, header->starts_offset, starts_size) ||
        starts->seg_count > image->segment_count) {
        return false;
    }
    bool valid = true;
    for (uint32_t segment_index = 0; segment_index < starts->seg_count;
         ++segment_index) {
        uint32_t info_offset = starts->seg_info_offset[segment_index];
        if (info_offset == 0) {
            continue;
        }
        uint64_t absolute_info = header->starts_offset + info_offset;
        if (!range_ok(image->fixups_size, absolute_info,
                      sizeof(struct dyld_chained_starts_in_segment))) {
            return false;
        }
        const struct dyld_chained_starts_in_segment* segment_starts =
            (const struct dyld_chained_starts_in_segment*)(payload + absolute_info);
        if (!range_ok(image->fixups_size, absolute_info, segment_starts->size) ||
            segment_starts->page_size == 0 ||
            segment_starts->size < offsetof(struct dyld_chained_starts_in_segment,page_start) + (uint64_t)segment_starts->page_count*2) {
            return false;
        }
        for (uint16_t page = 0; page < segment_starts->page_count; ++page) {
            uint16_t start = segment_starts->page_start[page];
            if (start == DYLD_CHAINED_PTR_START_NONE) {
                continue;
            }
            if ((start & DYLD_CHAINED_PTR_START_MULTI) == 0) {
                valid &= scan_chain(
                    image, &image->segments[segment_index],
                    segment_starts->pointer_format,
                    (uint64_t)page * segment_starts->page_size + start, counts);
                continue;
            }
            /* Multi-start pages are not covered by the native fixture yet. */
            fprintf(stderr, "unsupported multi-start chained page\n");
            return false;
        }
    }
    return valid;
}

static bool read_uleb(const uint8_t* data, size_t size, size_t* cursor,
                      uint64_t* value) {
    uint64_t result = 0;
    unsigned shift = 0;
    while (*cursor < size && shift < 64) {
        uint8_t byte = data[(*cursor)++];
        if (shift == 63 && (byte & 0x7e)) return false;
        result |= (uint64_t)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            *value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

static bool record_legacy_rebase(const Image* image, uint32_t segment_index,
                                 uint64_t segment_offset, uint8_t type,
                                 uint32_t* counts) {
    if (segment_index >= image->segment_count || type != REBASE_TYPE_POINTER) {
        return false;
    }
    const Segment* segment = &image->segments[segment_index];
    if (segment_offset > segment->filesize ||
        sizeof(uint64_t) > segment->filesize - segment_offset) {
        return false;
    }
    uint64_t target = 0;
    memcpy(&target, image->data + segment->fileoff + segment_offset,
           sizeof(target));
    static unsigned debug_count = 0;
    if (getenv("ESO_COMPAT_AUDIT_DEBUG") && debug_count++ < 8) {
        fprintf(stderr,
                "rebase segment=%u offset=0x%llx source=0x%llx target=0x%llx type=%u\n",
                segment_index, (unsigned long long)segment_offset,
                (unsigned long long)(segment->vmaddr + segment_offset),
                (unsigned long long)target, type);
    }
    return record_reference(image, segment->vmaddr + segment_offset, target,
                            REF_POINTER, counts);
}

static bool scan_legacy_rebases(const Image* image, uint32_t* counts) {
    if (image->rebase_size == 0) {
        return true;
    }
    if (!range_ok(image->size, image->rebase_offset, image->rebase_size)) {
        return false;
    }
    const uint8_t* stream = image->data + image->rebase_offset;
    size_t cursor = 0;
    uint8_t type = 0;
    uint32_t segment_index = 0;
    uint64_t segment_offset = 0;
    bool valid = true;
    while (cursor < image->rebase_size) {
        uint8_t byte = stream[cursor++];
        uint8_t opcode = byte & REBASE_OPCODE_MASK;
        uint8_t immediate = byte & REBASE_IMMEDIATE_MASK;
        uint64_t value = 0;
        uint64_t count = 0;
        uint64_t skip = 0;
        if (opcode == REBASE_OPCODE_DONE) {
            return valid;
        } else if (opcode == REBASE_OPCODE_SET_TYPE_IMM) {
            type = immediate;
        } else if (opcode == REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB) {
            segment_index = immediate;
            if (!read_uleb(stream, image->rebase_size, &cursor,
                           &segment_offset)) {
                return false;
            }
        } else if (opcode == REBASE_OPCODE_ADD_ADDR_ULEB) {
            if (!read_uleb(stream, image->rebase_size, &cursor, &value)) {
                return false;
            }
            if(value>UINT64_MAX-segment_offset)return false;
            segment_offset += value;
        } else if (opcode == REBASE_OPCODE_ADD_ADDR_IMM_SCALED) {
            if ((uint64_t)immediate*sizeof(uint64_t)>UINT64_MAX-segment_offset) return false;
            segment_offset += (uint64_t)immediate * sizeof(uint64_t);
        } else if (opcode == REBASE_OPCODE_DO_REBASE_IMM_TIMES) {
            count = immediate;
            if (count > image->size / 8) return false;
            for (uint64_t index = 0; index < count; ++index) {
                if (!record_legacy_rebase(image, segment_index, segment_offset, type, counts)) return false;
                if (segment_offset > UINT64_MAX-sizeof(uint64_t)) return false;
                segment_offset += sizeof(uint64_t);
            }
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES) {
            if (!read_uleb(stream, image->rebase_size, &cursor, &count)) {
                return false;
            }
            if (count > image->size / 8) return false;
            for (uint64_t index = 0; index < count; ++index) {
                if (!record_legacy_rebase(image, segment_index, segment_offset, type, counts)) return false;
                if (segment_offset > UINT64_MAX-sizeof(uint64_t)) return false;
                segment_offset += sizeof(uint64_t);
            }
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB) {
            if (!record_legacy_rebase(image, segment_index, segment_offset, type, counts)) return false;
            if (!read_uleb(stream, image->rebase_size, &cursor, &value)) {
                return false;
            }
            if(value>UINT64_MAX-sizeof(uint64_t)||segment_offset>UINT64_MAX-sizeof(uint64_t)-value)return false;
            segment_offset += sizeof(uint64_t) + value;
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB) {
            if (!read_uleb(stream, image->rebase_size, &cursor, &count) ||
                !read_uleb(stream, image->rebase_size, &cursor, &skip)) {
                return false;
            }
            if (count > image->size / 8) return false;
            for (uint64_t index = 0; index < count; ++index) {
                if (!record_legacy_rebase(image, segment_index, segment_offset, type, counts)) return false;
                if(skip>UINT64_MAX-sizeof(uint64_t)||segment_offset>UINT64_MAX-sizeof(uint64_t)-skip)return false;
                segment_offset += sizeof(uint64_t) + skip;
            }
        } else {
            return false;
        }
    }
    return false; /* A complete rebase stream must terminate explicitly. */
}

static const char* read_proc_name(const Image* image, const uint8_t* code,
                                  size_t code_size, size_t call_offset) {
    const char* result = NULL;
    size_t start = call_offset > 256 ? call_offset - 256 : 0;
    for (size_t offset = start; offset + 7 <= call_offset; ++offset) {
        if (code[offset] < 0x48 || code[offset] > 0x4f ||
            (code[offset] & 0x08) == 0 || code[offset + 1] != 0x8d ||
            (code[offset + 2] & 0xc7) != 0x05) {
            continue;
        }
        int32_t displacement = 0;
        memcpy(&displacement, code + offset + 3, sizeof(displacement));
        uint64_t target = image->text_address + offset + 7 + displacement;
        const uint8_t* string = vm_pointer(image, target, 3);
        if (!string || string[0] != 'v' || string[1] != 'k' ||
            string[2] < 'A' || string[2] > 'Z') {
            continue;
        }
        const uint8_t* end = vm_pointer(image, target, 128);
        if (!end) {
            continue;
        }
        if (memchr(string, '\0', 128) != NULL) {
            result = (const char*)string;
        }
    }
    (void)code_size;
    return result;
}

static bool scan_queries(const Image* image, uint32_t* counts) {
    const uint8_t* code = image->data + image->text_offset;
    size_t size = image->text_size;
    bool valid = true;
    for (size_t offset = 0; offset + 6 <= size; ++offset) {
        if (code[offset] != 0xff || code[offset + 1] != 0x15) {
            continue;
        }
        int32_t displacement = 0;
        memcpy(&displacement, code + offset + 2, sizeof(displacement));
        uint64_t source = image->text_address + offset;
        uint64_t slot = source + 6 + displacement - image->base;
        uint8_t route = slot == image->gipa
                            ? ROUTE_GIPA
                            : (slot == image->gdpa ? ROUTE_GDPA : 0);
        if (route == 0) {
            continue;
        }
        const char* name = read_proc_name(image, code, size, offset);
        if (!name) {
            fprintf(stderr, "proc query name unavailable at 0x%llx\n",
                    (unsigned long long)(source - image->base));
            valid = false;
            continue;
        }
        bool matched = false;
        for (size_t index = 0; index < COMPAT_QUERY_COUNT; ++index) {
            if (kCompatQueries[index].route == route &&
                strcmp(kCompatQueries[index].name, name) == 0) {
                ++counts[index];
                matched = true;
                break;
            }
        }
        if (!matched) {
            fprintf(stderr, "unexpected proc query route=%u name=%s\n", route,
                    name);
            valid = false;
        }
    }
    return valid;
}

static bool validate_counts(const uint32_t* reference_counts,
                            const uint32_t* query_counts) {
    bool valid = true;
    for (size_t index = 0; index < COMPAT_REFERENCE_COUNT; ++index) {
        if (reference_counts[index] != kCompatReferences[index].expected_count) {
            fprintf(stderr,
                    "reference count mismatch target=0x%llx kind=%u expected=%u actual=%u\n",
                    (unsigned long long)kCompatReferences[index].target_offset,
                    kCompatReferences[index].kind,
                    kCompatReferences[index].expected_count,
                    reference_counts[index]);
            valid = false;
        }
    }
    for (size_t index = 0; index < COMPAT_QUERY_COUNT; ++index) {
        if (query_counts[index] != kCompatQueries[index].expected_count) {
            fprintf(stderr,
                    "proc query count mismatch route=%u name=%s expected=%u actual=%u\n",
                    kCompatQueries[index].route, kCompatQueries[index].name,
                    kCompatQueries[index].expected_count, query_counts[index]);
            valid = false;
        }
    }
    return valid;
}

/* Decode from the enclosing LC_FUNCTION_STARTS entry. Unknown encodings stop
   validation; a byte-scan coincidence is discarded only after positive proof. */
static int instruction_boundary(const Image *image, uint64_t address) {
    if (!range_ok(image->size, image->functions_offset, image->functions_size)) return -1;
    size_t cursor=0;
    uint64_t current=image->base, start=0, delta=0;
    const uint8_t *table=image->data+image->functions_offset;
    while (cursor < image->functions_size) {
        if (!read_uleb(table, image->functions_size, &cursor, &delta)) return -1;
        if (!delta) break;
        if (delta > UINT64_MAX-current) return -1;
        current+=delta;
        if (current > address) break;
        start=current;
    }
    if (!start || address-start > 1024*1024) return -1;
    while (start < address) {
        const uint8_t *p=vm_pointer(image,start,1);
        if (!p) return -1;
        uint8_t padded[64]={0};
        size_t available=0;
        while (available<sizeof(padded) && vm_pointer(image,start,available+1)) ++available;
        memcpy(padded,p,available);
        hde64s decoded;
        unsigned length;
        if (padded[0]==0xc5 || padded[0]==0xc4) {
            /* Closed SIMD VEX forms preserve ModRM/displacement lengths.
               Explicit map-2/map-3 substitutions below also preserve immediate
               lengths. Other VEX/EVEX instructions fail closed. */
            unsigned vex=padded[0]==0xc5?2:3;
            uint8_t map=vex==3?(padded[1]&31):1;
            uint8_t pp=padded[vex-1]&3, op=padded[vex];
            bool supported=((op==0x6f||op==0x7f)&&pp==2)||
                ((op==0xef||op==0xeb||op==0xdb||op==0xdf||op==0x6e||op==0x62||op==0x72||op==0xfe)&&pp==1)||
                ((op==0x10||op==0x11||op==0x28||op==0x29||op==0x57)&&pp==0);
            supported=supported && map==1;
            uint8_t legacy_op=op;
            /* VPSHUFB has ModRM; VPINSRD/Q adds exactly one immediate byte. */
            if(map==2&&op==0&&pp==1){supported=true;legacy_op=0xef;}
            if(map==3&&op==0x22&&pp==1){supported=true;legacy_op=0x70;}
            if(vex==2&&op==0x77&&padded[1]==0xf8) {length=3;memset(&decoded,0,sizeof(decoded));}
            else if(supported) {
                uint8_t legacy[64]={0};size_t prefix=pp?1:0;
                if(pp)legacy[0]=pp==1?0x66:pp==2?0xf3:0xf2;
                legacy[prefix]=0x0f;memcpy(legacy+prefix+1,padded+vex,32);legacy[prefix+1]=legacy_op;
                length=hde64_disasm(legacy,&decoded)+vex-1-(unsigned)prefix;
            } else {if(getenv("TESO_UPDATE_DEBUG"))fprintf(stderr,"unsupported VEX map=%u opcode=%02x pp=%u at %llx\n",map,op,pp,(unsigned long long)(start-image->base));return -1;}
        } else length=hde64_disasm(padded,&decoded);
        if (!length || length>15 || length>available || (decoded.flags & F_ERROR)) {
            if(getenv("TESO_UPDATE_DEBUG"))fprintf(stderr,"decode stop at %llx bytes %02x %02x %02x %02x flags %x\n",(unsigned long long)(start-image->base),padded[0],padded[1],padded[2],padded[3],decoded.flags);
            return -1;
        }
        start+=length;
    }
    return start==address ? 1 : 0;
}

typedef struct { uint8_t *bytes; size_t size; struct stat status; } Snapshot;

static bool snapshot_read(const char *path, size_t limit, Snapshot *snapshot) {
    memset(snapshot,0,sizeof(*snapshot));
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if (fd<0) return false;
    struct stat after;
    bool valid=fstat(fd,&snapshot->status)==0 && S_ISREG(snapshot->status.st_mode) &&
        snapshot->status.st_size>0 && (uint64_t)snapshot->status.st_size<=limit;
    if (valid) {
        snapshot->size=(size_t)snapshot->status.st_size;
        snapshot->bytes=malloc(snapshot->size);
        valid=snapshot->bytes!=NULL;
    }
    size_t done=0;
    while (valid && done<snapshot->size) {
        ssize_t n=read(fd,snapshot->bytes+done,snapshot->size-done);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) {valid=false;break;}
        done+=(size_t)n;
    }
    valid=valid && fstat(fd,&after)==0 && after.st_ino==snapshot->status.st_ino &&
        after.st_size==snapshot->status.st_size &&
        after.st_mtimespec.tv_sec==snapshot->status.st_mtimespec.tv_sec &&
        after.st_mtimespec.tv_nsec==snapshot->status.st_mtimespec.tv_nsec;
    close(fd);
    if (!valid) {free(snapshot->bytes);snapshot->bytes=NULL;}
    return valid;
}

static void hex_bytes(const uint8_t *bytes,size_t size,char *output) {
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<size;++i) {output[i*2]=digits[bytes[i]>>4];output[i*2+1]=digits[bytes[i]&15];}
    output[size*2]=0;
}

static void hash_bytes(const void *bytes,size_t size,char output[65]) {
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(bytes,(CC_LONG)size,digest);hex_bytes(digest,sizeof(digest),output);
}

bool teso_update_file_sha256(const char *path,char output[65]) {
    Snapshot s;
    if(!snapshot_read(path,512*1024*1024,&s)) return false;
    hash_bytes(s.bytes,s.size,output);free(s.bytes);return true;
}

static bool snapshot_unchanged(const char *path,const Snapshot *before) {
    Snapshot after;
    if(!snapshot_read(path,512*1024*1024,&after)) return false;
    bool equal=after.size==before->size && after.status.st_ino==before->status.st_ino &&
        memcmp(after.bytes,before->bytes,before->size)==0;
    free(after.bytes);return equal;
}

static bool decimal_field(const uint8_t *bytes,size_t length,uint64_t *value) {
    uint64_t n=0;bool digit=false;
    for(size_t i=0;i<length;++i) {
        unsigned c=bytes[i];
        if(c==' ' || c==0) {
            for(;i<length;++i) if(bytes[i]!=' ' && bytes[i]!=0) return false;
            break;
        }
        if(c<'0'||c>'9'||n>(UINT64_MAX-(c-'0'))/10) return false;
        digit=true;n=n*10+c-'0';
    }
    *value=n;return digit;
}

static const uint8_t *archive_object(const Snapshot *archive,size_t *object_size) {
    if(archive->size<8 || memcmp(archive->bytes,"!<arch>\n",8)) return NULL;
    size_t cursor=8;const uint8_t *result=NULL;
    while(cursor<archive->size) {
        if(!range_ok(archive->size,cursor,60)) return NULL;
        const uint8_t *header=archive->bytes+cursor;uint64_t size=0;
        if(memcmp(header+58,"`\n",2)||!decimal_field(header+48,10,&size)||
           !range_ok(archive->size,cursor+60,size)) return NULL;
        const uint8_t *payload=header+60;uint64_t name_size=0;
        char name[256]={0};
        if(!memcmp(header,"#1/",3)) {
            if(!decimal_field(header+3,13,&name_size)||name_size>255||name_size>size) return NULL;
            memcpy(name,payload,(size_t)name_size);
        } else {
            memcpy(name,header,16);name[strcspn(name," /")]=0;
        }
        if(!strcmp(name,COMPAT_OBJECT_MEMBER)) {
            if(result) return NULL;
            result=payload+name_size;*object_size=(size_t)(size-name_size);
        }
        cursor+=60+(size_t)size+((size_t)size&1);
    }
    return cursor==archive->size?result:NULL;
}

static bool object_text(const uint8_t *object,size_t size,struct section_64 *text) {
    if(!range_ok(size,0,sizeof(struct mach_header_64))) return false;
    const struct mach_header_64 *header=(const void*)object;
    if(header->magic!=MH_MAGIC_64||header->cputype!=CPU_TYPE_X86_64||header->filetype!=MH_OBJECT||
       header->ncmds>4096||!range_ok(size,sizeof(*header),header->sizeofcmds)) return false;
    uint64_t cursor=sizeof(*header);unsigned found=0;
    for(uint32_t i=0;i<header->ncmds;++i) {
        if(!range_ok(size,cursor,sizeof(struct load_command)))return false;
        const struct load_command *c=(const void*)(object+cursor);
        if(c->cmdsize<sizeof(*c)||!range_ok(size,cursor,c->cmdsize))return false;
        if(c->cmd==LC_SEGMENT_64) {
            const struct segment_command_64 *s=(const void*)c;
            if(c->cmdsize<sizeof(*s)||s->nsects>(c->cmdsize-sizeof(*s))/sizeof(struct section_64))return false;
            const struct section_64 *sections=(const void*)(s+1);
            for(uint32_t j=0;j<s->nsects;++j) if(!strncmp(sections[j].sectname,"__text",16)) {*text=sections[j];++found;}
        }
        cursor+=c->cmdsize;
    }
    return found==1 && text->size>64 && range_ok(size,text->offset,text->size) &&
        range_ok(size,text->reloff,(uint64_t)text->nreloc*8);
}

static bool locate_object(Image *image,const uint8_t *object,size_t object_size) {
    struct section_64 section;
    if(!object_text(object,object_size,&section)||section.addr!=COMPAT_OBJECT_TEXT_ADDRESS)return false;
    size_t n=(size_t)section.size;
    uint8_t *masked=calloc(n,1),*relaxed=calloc(n,1);
    if(!masked||!relaxed) {free(masked);free(relaxed);return false;}
    const uint8_t *text=object+section.offset;
    bool valid=true;
    for(uint32_t i=0;i<section.nreloc;++i) {
        int32_t field;uint32_t info;
        memcpy(&field,object+section.reloff+i*8,4);memcpy(&info,object+section.reloff+i*8+4,4);
        size_t length=(size_t)1<<((info>>25)&3);
        if(field<0||!range_ok(n,(uint32_t)field,length)) {valid=false;break;}
        memset(masked+field,1,length);
        if((info>>28)==3 && field>=2 && text[field-2]==0x8b) relaxed[field-2]=0x8d;
    }
    size_t best=0,anchor=0,run=0;
    for(size_t i=0;valid&&i<n;++i) {run=masked[i]?0:run+1;if(run>best){best=run;anchor=i+1-run;}}
    const uint8_t *code=image->data+image->text_offset;unsigned matches=0;
    if(best<64||image->text_size<n)valid=false;
    for(size_t start=0;valid&&start<=image->text_size-n;++start) {
        if(memcmp(code+start+anchor,text+anchor,64))continue;
        bool equal=true;
        for(size_t j=0;j<n;++j)if(!masked[j]&&code[start+j]!=text[j]&&
            (!relaxed[j]||code[start+j]!=relaxed[j])){equal=false;break;}
        if(equal){++matches;image->old_start=image->text_address-image->base+start;image->old_end=image->old_start+n;}
    }
    free(masked);free(relaxed);return valid&&matches==1;
}

static bool address_has_protection(const Image *image,uint64_t offset,uint64_t length,vm_prot_t required) {
    if(offset>UINT64_MAX-image->base)return false;
    uint64_t address=image->base+offset;
    for(uint32_t i=0;i<image->segment_count;++i) {
        const Segment *s=&image->segments[i];
        if(address>=s->vmaddr&&address-s->vmaddr<=s->vmsize&&length<=s->vmsize-(address-s->vmaddr))
            return (s->prot&required)==required;
    }
    return false;
}

static bool locate_slots(Image *image) {
    struct Slot {uint64_t offset;uint32_t counts[COMPAT_QUERY_COUNT];unsigned unknown;} slots[256]={0};
    size_t count=0;const uint8_t *code=image->data+image->text_offset;
    for(size_t i=0;i+6<=image->text_size;++i) {
        if(code[i]!=0xff||code[i+1]!=0x15)continue;
        const char *name=read_proc_name(image,code,image->text_size,i);
        if(!name)continue;
        int32_t displacement;memcpy(&displacement,code+i+2,4);
        uint64_t slot=image->text_address+i+6+displacement-image->base;
        size_t index=0;for(;index<count&&slots[index].offset!=slot;++index){}
        if(index==count){if(count==256)return false;slots[count++].offset=slot;}
        bool known=false;
        for(size_t q=0;q<COMPAT_QUERY_COUNT;++q)if(!strcmp(kCompatQueries[q].name,name)){
            ++slots[index].counts[q];known=true;
        }
        if(!known)++slots[index].unknown;
    }
    unsigned found[2]={0};
    for(size_t i=0;i<count;++i)for(uint8_t route=1;route<=2;++route) {
        bool equal=slots[i].unknown==0;
        for(size_t q=0;q<COMPAT_QUERY_COUNT;++q)if(kCompatQueries[q].route==route&&
            slots[i].counts[q]!=kCompatQueries[q].expected_count)equal=false;
        /* Names absent from the expected route must also be absent. */
        for(size_t q=0;q<COMPAT_QUERY_COUNT;++q)if(slots[i].counts[q]) {
            bool in_route=false;
            for(size_t j=0;j<COMPAT_QUERY_COUNT;++j)if(kCompatQueries[j].route==route&&
                !strcmp(kCompatQueries[j].name,kCompatQueries[q].name))in_route=true;
            if(!in_route)equal=false;
        }
        if(equal){++found[route-1];if(route==1)image->gipa=slots[i].offset;else image->gdpa=slots[i].offset;}
    }
    return found[0]==1&&found[1]==1&&image->gipa!=image->gdpa&&
        address_has_protection(image,image->gipa,8,VM_PROT_WRITE)&&address_has_protection(image,image->gdpa,8,VM_PROT_WRITE);
}

static bool is_usleep_stub(const Image *image,uint64_t target) {
    const struct section_64 *s=&image->stubs;
    if(!s->reserved2||target<s->addr||target-s->addr>=s->size||(target-s->addr)%s->reserved2)return false;
    uint64_t index=s->reserved1+(target-s->addr)/s->reserved2;
    if(index>=image->nindirect||!range_ok(image->size,image->indirectoff,(uint64_t)image->nindirect*4))return false;
    uint32_t symbol;memcpy(&symbol,image->data+image->indirectoff+index*4,4);
    if(symbol>=image->nsyms||!range_ok(image->size,image->symoff,(uint64_t)image->nsyms*sizeof(struct nlist_64))||
       !range_ok(image->size,image->stroff,image->strsize))return false;
    struct nlist_64 n;memcpy(&n,image->data+image->symoff+(uint64_t)symbol*sizeof(n),sizeof(n));
    return n.n_un.n_strx<image->strsize&&image->strsize-n.n_un.n_strx>=8&&
        !memcmp(image->data+image->stroff+n.n_un.n_strx,"_usleep\0",8);
}

static bool locate_pacing(const Image *image,Teso4m4InactivePacingTarget *pacing) {
    const uint8_t prefix[]={0x75,0x27,0xbf,0xa0,0x86,0x01,0,0xe8};
    const uint8_t flag_test[]={0x41,0x80,0x7d,0,0};
    const uint8_t rejoin_bytes[]={0x8a,0x43,0x18,0x84,0xc0};
    const uint8_t *code=image->data+image->text_offset;size_t site=0;unsigned found=0;
    for(size_t i=5;i+14<image->text_size;++i)if(!memcmp(code+i,prefix,sizeof(prefix))){site=i;++found;}
    if(found!=1||memcmp(code+site-5,flag_test,5)||code[site+12]!=0xeb||instruction_boundary(image,image->text_address+site)!=1)return false;
    int32_t displacement;memcpy(&displacement,code+site+8,4);
    if(!is_usleep_stub(image,image->text_address+site+12+displacement))return false;
    int64_t rejoin=(int64_t)site+14+(int8_t)code[site+13];
    if(rejoin<0||!range_ok(image->text_size,(uint64_t)rejoin,5)||memcmp(code+rejoin,rejoin_bytes,5)||
       site+2+code[site+1]!=(uint64_t)rejoin+3)return false;
    size_t lea=0;found=0;
    for(size_t i=site>128?site-128:0;i+7<=site;++i)if(!memcmp(code+i,"\x4c\x8d\x2d",3)){lea=i;++found;}
    if(found!=1||instruction_boundary(image,image->text_address+lea)!=1)return false;
    memcpy(&displacement,code+lea+3,4);
    pacing->image_offset=image->text_address+site-image->base;
    pacing->active_flag_offset=image->text_address+lea+7+displacement-image->base;
    memcpy(pacing->expected,code+site,12);
    return address_has_protection(image,pacing->image_offset,12,VM_PROT_EXECUTE)&&
        address_has_protection(image,pacing->active_flag_offset,1,VM_PROT_WRITE);
}

static bool build_stamp_path(const char *executable,char output[4096]) {
    const char *slash=strrchr(executable,'/');
    return slash && snprintf(output,4096,"%.*s/../../../../../depot/_databuild/databuild.stamp",(int)(slash-executable),executable)<4096;
}

static bool client_identity(const Snapshot *stamp,TesoUpdateProfile *profile) {
    char text[1025];if(stamp->size>=sizeof(text))return false;
    memcpy(text,stamp->bytes,stamp->size);text[stamp->size]=0;
    char *line1=strchr(text,'\n');if(!line1)return false;*line1++=0;
    text[strcspn(text,"\r")]=0;
    char *line2=strchr(line1,'\n');if(!line2)return false;*line2++=0;
    line2[strcspn(line2,"\r\n")]=0;
    char *build=strstr(text,".live.");if(!build)return false;build+=6;
    if(!*build || strlen(build)>=sizeof(profile->client_databuild))return false;
    for(char *p=build;*p;++p)if(*p<'0'||*p>'9')return false;
    unsigned dots=0;bool digit=false;
    if(!*line2||strlen(line2)>=sizeof(profile->client_version))return false;
    for(char *p=line2;*p;++p){if(*p=='.'){if(!digit)return false;++dots;digit=false;}else if(*p>='0'&&*p<='9')digit=true;else return false;}
    if(dots!=2||!digit)return false;
    strlcpy(profile->client_version,line2,sizeof(profile->client_version));
    strlcpy(profile->client_databuild,build,sizeof(profile->client_databuild));return true;
}

bool teso_update_resolve(const char *executable,const char *archive,TesoUpdateProfile *profile) {
    Snapshot exe={0},ar={0},stamp={0};bool valid=false;Image image;char digest[65],stamp_path[4096];
    memset(profile,0,sizeof(*profile));
    if(!snapshot_read(executable,512*1024*1024,&exe)||!snapshot_read(archive,512*1024*1024,&ar))goto done;
    if(!build_stamp_path(executable,stamp_path)||!snapshot_read(stamp_path,1024,&stamp)||!client_identity(&stamp,profile))goto done;
    hash_bytes(ar.bytes,ar.size,digest);if(strcmp(digest,COMPAT_ARCHIVE_SHA))goto done;
    if(!parse_image(exe.bytes,exe.size,&image)||((const struct mach_header_64*)exe.bytes)->filetype!=MH_EXECUTE)goto done;
    size_t object_size=0;const uint8_t *object=archive_object(&ar,&object_size);
    if(!object)goto done;
    hash_bytes(object,object_size,digest);if(strcmp(digest,COMPAT_OBJECT_SHA))goto done;
    if(!locate_object(&image,object,object_size)||!locate_slots(&image)||!locate_pacing(&image,&profile->pacing))goto done;
    for(size_t i=0;i<COMPAT_PATCH_COUNT;++i) {
        uint64_t offset=image.old_start+(kCompatPatches[i].image_offset-COMPAT_OLD_TEXT_START);
        const uint8_t *p=vm_pointer(&image,image.base+offset,12);
        if(!p||!address_has_protection(&image,offset,12,VM_PROT_EXECUTE)||memcmp(p,kCompatPatches[i].expected,12))goto done;
        profile->targets[i].offset=offset;memcpy(profile->targets[i].expected,p,12);
        for(size_t j=0;j<i;++j)if(offset<profile->targets[j].offset+12&&profile->targets[j].offset<offset+12)goto done;
        if(offset<profile->pacing.image_offset+12&&profile->pacing.image_offset<offset+12)goto done;
    }
    uint32_t references[COMPAT_REFERENCE_COUNT]={0},queries[COMPAT_QUERY_COUNT]={0};
    if(!scan_text_references(&image,references)){fprintf(stderr,"text boundary validation failed\n");goto done;}
    if(!scan_fixups(&image,references)){fprintf(stderr,"chained fixup validation failed\n");goto done;}
    if(!scan_legacy_rebases(&image,references)){fprintf(stderr,"classic rebase validation failed\n");goto done;}
    if(!scan_queries(&image,queries)){fprintf(stderr,"proc query validation failed\n");goto done;}
    if(!validate_counts(references,queries))goto done;
    /* The final check includes pacing discovery and every other analysis step. */
    if(!snapshot_unchanged(executable,&exe)||!snapshot_unchanged(archive,&ar)||!snapshot_unchanged(stamp_path,&stamp))goto done;
    hash_bytes(exe.bytes,exe.size,profile->executable_sha256);memcpy(profile->uuid,image.uuid,16);
    profile->placement=image.old_start;profile->gipa_slot=image.gipa;profile->gdpa_slot=image.gdpa;
    valid=true;
done:
    if(!valid)fprintf(stderr,"Dynamic profile: structural validation refused\n");
    free(exe.bytes);free(ar.bytes);free(stamp.bytes);return valid;
}

size_t teso_update_render(const TesoUpdateProfile *profile,char *output,size_t capacity) {
    char uuid[33],pacing_bytes[25];hex_bytes(profile->uuid,16,uuid);hex_bytes(profile->pacing.expected,12,pacing_bytes);
    int n=snprintf(output,capacity,"schema=1\nabi=1\nrules=%s\nexecutable=%s\nuuid=%s\nclient=%s:%s\narchive=%s\nruntime=%s\noriginal=%s\nrenamed=%s\nplacement=%llx\ngipa=%llx\ngdpa=%llx\npacing=%llx:%s:%llx\n",
        COMPAT_RULE_ID,profile->executable_sha256,uuid,profile->client_version,profile->client_databuild,COMPAT_ARCHIVE_SHA,COMPAT_RUNTIME_SHA,COMPAT_ORIGINAL_SHA,COMPAT_RENAMED_SHA,
        (unsigned long long)profile->placement,(unsigned long long)profile->gipa_slot,(unsigned long long)profile->gdpa_slot,
        (unsigned long long)profile->pacing.image_offset,pacing_bytes,(unsigned long long)profile->pacing.active_flag_offset);
    if(n<0||(size_t)n>=capacity)return 0;
    size_t used=(size_t)n;
    for(size_t i=0;i<TESO_UPDATE_TARGET_COUNT;++i){char bytes[25];hex_bytes(profile->targets[i].expected,12,bytes);
        n=snprintf(output+used,capacity-used,"target=%zu:%s:%llx:%s\n",i,kCompatNames[i],(unsigned long long)profile->targets[i].offset,bytes);
        if(n<0||(size_t)n>=capacity-used)return 0;used+=(size_t)n;
    }
    return used;
}

bool teso_update_verify_file(const TesoUpdateProfile *profile,const char *path,char digest[65]) {
    char expected[TESO_UPDATE_PROFILE_LIMIT];size_t length=teso_update_render(profile,expected,sizeof(expected));
    Snapshot supplied;
    if(!length||!snapshot_read(path,TESO_UPDATE_PROFILE_LIMIT,&supplied))return false;
    bool valid=(supplied.status.st_uid==getuid()||supplied.status.st_uid==0)&&
        !(supplied.status.st_mode&0077)&&supplied.size==length&&!memcmp(expected,supplied.bytes,length);
    if(valid)hash_bytes(supplied.bytes,supplied.size,digest);
    free(supplied.bytes);return valid;
}

bool teso_update_payloads_match(const char *runtime,const char *renamed_original) {
    char digest[65];
    return teso_update_file_sha256(runtime,digest)&&!strcmp(digest,COMPAT_RUNTIME_SHA)&&
        teso_update_file_sha256(renamed_original,digest)&&!strcmp(digest,COMPAT_RENAMED_SHA);
}
