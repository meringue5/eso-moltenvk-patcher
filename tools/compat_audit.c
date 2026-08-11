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

#include "generated_compat_audit.h"

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
    uint64_t cursor = sizeof(*header);
    for (uint32_t index = 0; index < header->ncmds; ++index) {
        if (!range_ok(size, cursor, sizeof(struct load_command))) {
            return false;
        }
        const struct load_command* command =
            (const struct load_command*)(data + cursor);
        if (command->cmdsize < sizeof(*command) ||
            !range_ok(size, cursor, command->cmdsize)) {
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
                if (strncmp(section[section_index].segname, "__TEXT", 16) == 0 &&
                    strncmp(section[section_index].sectname, "__text", 16) == 0) {
                    image->text_address = section[section_index].addr;
                    image->text_size = section[section_index].size;
                    image->text_offset = section[section_index].offset;
                }
            }
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
    return image->base != UINT64_MAX && image->text_size != 0 &&
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
    uint64_t start = image->base + COMPAT_OLD_TEXT_START;
    uint64_t end = image->base + COMPAT_OLD_TEXT_END;
    return address >= start && address < end;
}

static bool record_reference(const Image* image, uint64_t source,
                             uint64_t target, uint8_t kind,
                             uint32_t* counts) {
    if (!in_old_text(image, target) || in_old_text(image, source)) {
        return true;
    }
    uint64_t target_offset = target - image->base;
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
            segment_starts->page_size == 0) {
            return false;
        }
        const uint16_t* overflow =
            segment_starts->page_start + segment_starts->page_count;
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
            uint16_t overflow_index = start & ~DYLD_CHAINED_PTR_START_MULTI;
            for (;;) {
                const uint16_t* entry = overflow + overflow_index++;
                const uint8_t* entry_bytes = (const uint8_t*)entry;
                const uint8_t* info_end =
                    (const uint8_t*)segment_starts + segment_starts->size;
                if (entry_bytes + sizeof(*entry) > info_end) {
                    return false;
                }
                uint16_t chain_start = *entry;
                valid &= scan_chain(
                    image, &image->segments[segment_index],
                    segment_starts->pointer_format,
                    (uint64_t)page * segment_starts->page_size +
                        (chain_start & ~DYLD_CHAINED_PTR_START_LAST),
                    counts);
                if ((chain_start & DYLD_CHAINED_PTR_START_LAST) != 0) {
                    break;
                }
            }
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
            segment_offset += value;
        } else if (opcode == REBASE_OPCODE_ADD_ADDR_IMM_SCALED) {
            segment_offset += (uint64_t)immediate * sizeof(uint64_t);
        } else if (opcode == REBASE_OPCODE_DO_REBASE_IMM_TIMES) {
            count = immediate;
            for (uint64_t index = 0; index < count; ++index) {
                valid &= record_legacy_rebase(
                    image, segment_index, segment_offset, type, counts);
                segment_offset += sizeof(uint64_t);
            }
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES) {
            if (!read_uleb(stream, image->rebase_size, &cursor, &count)) {
                return false;
            }
            for (uint64_t index = 0; index < count; ++index) {
                valid &= record_legacy_rebase(
                    image, segment_index, segment_offset, type, counts);
                segment_offset += sizeof(uint64_t);
            }
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB) {
            valid &= record_legacy_rebase(
                image, segment_index, segment_offset, type, counts);
            if (!read_uleb(stream, image->rebase_size, &cursor, &value)) {
                return false;
            }
            segment_offset += sizeof(uint64_t) + value;
        } else if (opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB) {
            if (!read_uleb(stream, image->rebase_size, &cursor, &count) ||
                !read_uleb(stream, image->rebase_size, &cursor, &skip)) {
                return false;
            }
            for (uint64_t index = 0; index < count; ++index) {
                valid &= record_legacy_rebase(
                    image, segment_index, segment_offset, type, counts);
                segment_offset += sizeof(uint64_t) + skip;
            }
        } else {
            return false;
        }
    }
    return valid;
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
        uint8_t route = slot == COMPAT_GIPA_SLOT
                            ? ROUTE_GIPA
                            : (slot == COMPAT_GDPA_SLOT ? ROUTE_GDPA : 0);
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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s /path/to/eso\n", argv[0]);
        return 2;
    }
    int descriptor = open(argv[1], O_RDONLY);
    if (descriptor < 0) {
        fprintf(stderr, "cannot open ESO executable: %s\n", strerror(errno));
        return 2;
    }
    struct stat status;
    if (fstat(descriptor, &status) != 0 || status.st_size <= 0) {
        close(descriptor);
        return 2;
    }
    const uint8_t* data = mmap(
        NULL, (size_t)status.st_size, PROT_READ, MAP_PRIVATE, descriptor, 0);
    if (data == MAP_FAILED) {
        close(descriptor);
        return 2;
    }
    Image image;
    bool valid = parse_image(data, (size_t)status.st_size, &image);
    if (getenv("ESO_COMPAT_AUDIT_DEBUG")) {
        fprintf(stderr,
                "image base=0x%llx segments=%u rebase_offset=%u rebase_size=%u fixups_size=%u\n",
                (unsigned long long)image.base, image.segment_count,
                image.rebase_offset, image.rebase_size, image.fixups_size);
    }
    if (!valid) {
        fprintf(stderr, "unsupported ESO Mach-O layout\n");
    }
    for (size_t index = 0; valid && index < COMPAT_PATCH_COUNT; ++index) {
        const uint8_t* site = vm_pointer(
            &image, image.base + kCompatPatches[index].image_offset, 12);
        if (!site || memcmp(site, kCompatPatches[index].expected, 12) != 0) {
            fprintf(stderr, "patch bytes changed at 0x%llx\n",
                    (unsigned long long)kCompatPatches[index].image_offset);
            valid = false;
        }
    }
    uint32_t reference_counts[COMPAT_REFERENCE_COUNT] = {0};
    uint32_t query_counts[COMPAT_QUERY_COUNT] = {0};
    if (valid) {
        valid &= scan_text_references(&image, reference_counts);
        valid &= scan_fixups(&image, reference_counts);
        valid &= scan_legacy_rebases(&image, reference_counts);
        valid &= scan_queries(&image, query_counts);
        valid &= validate_counts(reference_counts, query_counts);
    }
    munmap((void*)data, (size_t)status.st_size);
    close(descriptor);
    if (!valid) {
        puts("INCOMPATIBLE_UPDATE");
        return 3;
    }
    puts("COMPATIBLE_UPDATE");
    return 0;
}
