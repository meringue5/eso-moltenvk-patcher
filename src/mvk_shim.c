#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char* symbol;
    uint64_t image_offset;
    uint8_t expected[12];
} PatchTarget;

#include "generated_targets.h"

static FILE* g_log;
typedef void (*VkVoidFunction)(void);
typedef VkVoidFunction (*VkGetInstanceProcAddrFunction)(void* instance, const char* name);
static VkGetInstanceProcAddrFunction g_next_get_instance_proc_addr;

static void log_line(const char* message) {
    if (g_log) {
        fprintf(g_log, "%s\n", message);
        fflush(g_log);
    }
}

static VkVoidFunction traced_get_instance_proc_addr(void* instance, const char* name) {
    VkVoidFunction result = g_next_get_instance_proc_addr(instance, name);
    if (g_log) {
        fprintf(g_log, "GIPA: %s -> %p%s\n", name ? name : "(null)", (void*)result,
                result ? "" : " [NULL]");
        fflush(g_log);
    }
    return result;
}

static bool has_expected_uuid(const struct mach_header_64* header) {
    const uint8_t* cursor = (const uint8_t*)(header + 1);
    for (uint32_t index = 0; index < header->ncmds; ++index) {
        const struct load_command* command = (const struct load_command*)cursor;
        if (command->cmdsize < sizeof(*command)) {
            return false;
        }
        if (command->cmd == LC_UUID && command->cmdsize >= sizeof(struct uuid_command)) {
            const struct uuid_command* uuid = (const struct uuid_command*)command;
            return memcmp(uuid->uuid, kExpectedUUID, sizeof(kExpectedUUID)) == 0;
        }
        cursor += command->cmdsize;
    }
    return false;
}

static bool own_directory(char* output, size_t capacity) {
    Dl_info info;
    if (dladdr((void*)&own_directory, &info) == 0 || !info.dli_fname) {
        return false;
    }
    if (strlcpy(output, info.dli_fname, capacity) >= capacity) {
        return false;
    }
    char* slash = strrchr(output, '/');
    if (!slash) {
        return false;
    }
    *slash = '\0';
    return true;
}

static bool marker_mode(const char* directory, bool* live_check) {
    char path[4096];
    if (snprintf(path, sizeof(path), "%s/.teso4m4-enable", directory) >= (int)sizeof(path)) {
        return false;
    }
    FILE* marker = fopen(path, "r");
    if (!marker) {
        return false;
    }
    char mode[64] = {0};
    if (fgets(mode, sizeof(mode), marker) && strncmp(mode, "live-check", 10) == 0) {
        *live_check = true;
    }
    fclose(marker);
    return true;
}

static bool install_patches(const struct mach_header_64* header, void* moltenvk) {
    void* destinations[ESO_TARGET_COUNT];
    uintptr_t writable_pages[ESO_TARGET_COUNT];
    size_t writable_page_count = 0;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    for (size_t index = 0; index < ESO_TARGET_COUNT; ++index) {
        const PatchTarget* target = &kPatchTargets[index];
        const uint8_t* address = (const uint8_t*)header + target->image_offset;
        if (memcmp(address, target->expected, sizeof(target->expected)) != 0) {
            fprintf(g_log, "ERROR: original bytes differ at %s\n", target->symbol);
            return false;
        }
        destinations[index] = dlsym(moltenvk, target->symbol);
        if (!destinations[index]) {
            fprintf(g_log, "ERROR: MoltenVK does not export %s\n", target->symbol);
            return false;
        }
        if (strcmp(target->symbol, "vkGetInstanceProcAddr") == 0) {
            g_next_get_instance_proc_addr =
                (VkGetInstanceProcAddrFunction)destinations[index];
            destinations[index] = (void*)&traced_get_instance_proc_addr;
        }
    }

    for (size_t index = 0; index < ESO_TARGET_COUNT; ++index) {
        uintptr_t address = (uintptr_t)header + kPatchTargets[index].image_offset;
        uintptr_t page = address & ~((uintptr_t)page_size - 1);
        bool seen = false;
        for (size_t page_index = 0; page_index < writable_page_count; ++page_index) {
            seen |= writable_pages[page_index] == page;
        }
        if (seen) {
            continue;
        }
        kern_return_t result = mach_vm_protect(
            mach_task_self(), page, (mach_vm_size_t)page_size, FALSE,
            VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
        if (result != KERN_SUCCESS) {
            fprintf(g_log, "ERROR: mach_vm_protect: %s\n", mach_error_string(result));
            for (size_t page_index = 0; page_index < writable_page_count; ++page_index) {
                mach_vm_protect(mach_task_self(), writable_pages[page_index],
                                (mach_vm_size_t)page_size, FALSE,
                                VM_PROT_READ | VM_PROT_EXECUTE);
            }
            return false;
        }
        writable_pages[writable_page_count++] = page;
    }

    for (size_t index = 0; index < ESO_TARGET_COUNT; ++index) {
        uint8_t patch[12] = {0x48, 0xb8};
        uint64_t destination = (uint64_t)(uintptr_t)destinations[index];
        memcpy(&patch[2], &destination, sizeof(destination));
        patch[10] = 0xff;
        patch[11] = 0xe0;
        uint8_t* address = (uint8_t*)header + kPatchTargets[index].image_offset;
        memcpy(address, patch, sizeof(patch));
        __builtin___clear_cache((char*)address, (char*)address + sizeof(patch));
    }

    for (size_t index = 0; index < writable_page_count; ++index) {
        kern_return_t result = mach_vm_protect(
            mach_task_self(), writable_pages[index], (mach_vm_size_t)page_size, FALSE,
            VM_PROT_READ | VM_PROT_EXECUTE);
        if (result != KERN_SUCCESS) {
            fprintf(g_log, "WARNING: could not restore RX page: %s\n",
                    mach_error_string(result));
        }
    }
    return true;
}

__attribute__((constructor)) static void teso4m4_init(void) {
    g_log = fopen("/tmp/teso4m4.log", "a");
    if (g_log) {
        setvbuf(g_log, NULL, _IOLBF, 0);
    }
    log_line("--- teso4m4 bridge starting ---");

    char directory[4096];
    bool live_check = false;
    if (!own_directory(directory, sizeof(directory)) || !marker_mode(directory, &live_check)) {
        log_line("SKIP: enable marker absent");
        return;
    }
    if (live_check) {
        setenv("MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES", "1", 0);
        log_line("MODE: live-resource compatibility check enabled");
    }

    const struct mach_header* raw = _dyld_get_image_header(0);
    if (!raw || raw->magic != MH_MAGIC_64 || raw->cputype != CPU_TYPE_X86_64) {
        log_line("SKIP: unexpected main executable architecture");
        return;
    }
    const struct mach_header_64* header = (const struct mach_header_64*)raw;
    if (!has_expected_uuid(header)) {
        log_line("SKIP: ESO UUID mismatch");
        return;
    }

    char moltenvk_path[4096];
    if (snprintf(moltenvk_path, sizeof(moltenvk_path), "%s/libMoltenVK.teso4m4.dylib",
                 directory) >= (int)sizeof(moltenvk_path)) {
        return;
    }
    void* moltenvk = dlopen(moltenvk_path, RTLD_NOW | RTLD_LOCAL);
    if (!moltenvk) {
        fprintf(g_log, "ERROR: dlopen: %s\n", dlerror());
        return;
    }
    if (!install_patches(header, moltenvk)) {
        log_line("ERROR: patch transaction aborted");
        return;
    }
    fprintf(g_log, "ACTIVE: redirected %d Vulkan entry points\n", ESO_TARGET_COUNT);
    fprintf(g_log, "ESO SHA-256: %s\n", ESO_EXPECTED_SHA256);
    fflush(g_log);
}
