#include <dlfcn.h>
#include <errno.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <MoltenVK/mvk_private_api.h>

#include "mvk_compat.h"
#include "mvk_lifecycle.h"
#include "mvk_reset_trace.h"

typedef struct {
    const char* symbol;
    uint64_t image_offset;
    uint8_t expected[12];
} PatchTarget;

#include "generated_targets.h"

static FILE* g_log;
static char g_run_id[80] = "uninitialized";
static PFN_vkGetInstanceProcAddr g_next_get_instance_proc_addr;
static PFN_vkGetDeviceProcAddr g_next_get_device_proc_addr;
static bool g_reset_trace_enabled;

typedef enum {
    TESO4M4_MODE_DISABLED = 0,
    TESO4M4_MODE_DESCRIPTOR_COMPAT,
    TESO4M4_MODE_LEGACY_ALLOCATION,
    TESO4M4_MODE_RESET_RESOURCE_TRACE,
    TESO4M4_MODE_NO_COMMAND_POOLING,
    TESO4M4_MODE_RENDER_AUDIT,
} Teso4m4Mode;

static void initialize_run_id(void) {
    struct timespec now = {0};
    struct tm utc = {0};
    char timestamp[32] = "unknown-time";
    if (clock_gettime(CLOCK_REALTIME, &now) == 0 &&
        gmtime_r(&now.tv_sec, &utc) != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y%m%dT%H%M%S", &utc);
    }
    snprintf(g_run_id, sizeof(g_run_id), "%s.%09ldZ-pid%ld", timestamp,
             now.tv_nsec, (long)getpid());
}

static void log_message(const char* format, ...) {
    if (!g_log) {
        return;
    }
    flockfile(g_log);
    fprintf(g_log, "[run=%s] ", g_run_id);
    va_list arguments;
    va_start(arguments, format);
    vfprintf(g_log, format, arguments);
    va_end(arguments);
    fputc('\n', g_log);
    fflush(g_log);
    funlockfile(g_log);
}

static void compat_log_message(const char* message) {
    log_message("%s", message);
}

static PFN_vkVoidFunction VKAPI_CALL traced_get_device_proc_addr(
    VkDevice device, const char* name) {
    if (!g_next_get_device_proc_addr) {
        log_message("GDPA_ERROR: next function is unset name=%s",
                    name ? name : "(null)");
        return NULL;
    }
    PFN_vkVoidFunction result = g_next_get_device_proc_addr(device, name);
    PFN_vkVoidFunction lifecycle_returned =
        teso4m4_lifecycle_intercept(name, result);
    PFN_vkVoidFunction returned = lifecycle_returned;
    const char* shim = returned != result ? "lifecycle-trace" : "none";
    if (g_reset_trace_enabled) {
        returned = teso4m4_reset_trace_intercept(name, lifecycle_returned);
        if (returned != lifecycle_returned) {
            shim = "reset-resource-trace";
        }
    }
    log_message(
        "GDPA: device=%p name=%s result=%p returned=%p shim=%s%s",
        (void*)device, name ? name : "(null)", (void*)result,
        (void*)returned, shim,
        result ? "" : " [NULL]");
    return returned;
}

static PFN_vkVoidFunction VKAPI_CALL traced_get_instance_proc_addr(
    VkInstance instance, const char* name) {
    if (!g_next_get_instance_proc_addr) {
        log_message("GIPA_ERROR: next function is unset name=%s",
                    name ? name : "(null)");
        return NULL;
    }
    PFN_vkVoidFunction raw_result =
        g_next_get_instance_proc_addr(instance, name);
    PFN_vkVoidFunction returned_result = raw_result;
    const char* shim = "none";
    if (raw_result && name && strcmp(name, "vkGetDeviceProcAddr") == 0) {
        returned_result = (PFN_vkVoidFunction)&traced_get_device_proc_addr;
        shim = "proc-trace";
    } else if (raw_result && name &&
               strcmp(name, "vkGetInstanceProcAddr") == 0) {
        returned_result = (PFN_vkVoidFunction)&traced_get_instance_proc_addr;
        shim = "proc-trace";
    } else if (raw_result && name &&
               strcmp(name, "vkEnumerateDeviceExtensionProperties") == 0) {
        returned_result = (PFN_vkVoidFunction)
            &teso4m4_enumerate_device_extension_properties;
        shim = "hdr-filter";
    } else if (raw_result && name && strcmp(name, "vkCreateDevice") == 0) {
        returned_result = (PFN_vkVoidFunction)&teso4m4_create_device;
        shim = "device-trace";
    } else if (raw_result && name &&
               strcmp(name, "vkGetPhysicalDeviceSurfaceFormatsKHR") == 0) {
        returned_result = (PFN_vkVoidFunction)
            &teso4m4_get_physical_device_surface_formats;
        shim = "surface-format-filter";
    }
    log_message(
        "GIPA: instance=%p name=%s raw=%p returned=%p shim=%s%s",
        (void*)instance, name ? name : "(null)", (void*)raw_result,
        (void*)returned_result, shim, raw_result ? "" : " [NULL]");
    return returned_result;
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

static Teso4m4Mode marker_mode(const char* directory) {
    char path[4096];
    if (snprintf(path, sizeof(path), "%s/.teso4m4-enable", directory) >= (int)sizeof(path)) {
        return TESO4M4_MODE_DISABLED;
    }
    FILE* marker = fopen(path, "r");
    if (!marker) {
        return TESO4M4_MODE_DISABLED;
    }
    char mode[64] = {0};
    if (fgets(mode, sizeof(mode), marker)) {
        mode[strcspn(mode, "\r\n")] = '\0';
    }
    fclose(marker);
    if (strcmp(mode, "descriptor-compat") == 0) {
        return TESO4M4_MODE_DESCRIPTOR_COMPAT;
    }
    if (strcmp(mode, "legacy-allocation") == 0) {
        return TESO4M4_MODE_LEGACY_ALLOCATION;
    }
    if (strcmp(mode, "reset-resource-trace") == 0) {
        return TESO4M4_MODE_RESET_RESOURCE_TRACE;
    }
    if (strcmp(mode, "no-command-pooling") == 0) {
        return TESO4M4_MODE_NO_COMMAND_POOLING;
    }
    if (strcmp(mode, "render-audit") == 0) {
        return TESO4M4_MODE_RENDER_AUDIT;
    }
    return TESO4M4_MODE_DISABLED;
}

static bool verify_moltenvk_configuration(
    void* moltenvk, Teso4m4Mode mode) {
    PFN_vkGetMoltenVKConfigurationMVK get_configuration =
        (PFN_vkGetMoltenVKConfigurationMVK)dlsym(
            moltenvk, "vkGetMoltenVKConfigurationMVK");
    if (!get_configuration) {
        log_message("ERROR: MoltenVK configuration query is unavailable");
        return false;
    }

    MVKConfiguration configuration = {0};
    size_t configuration_size = sizeof(configuration);
    VkResult result = get_configuration(
        VK_NULL_HANDLE, &configuration, &configuration_size);
    if (result != VK_SUCCESS || configuration_size != sizeof(configuration)) {
        log_message(
            "ERROR: MoltenVK configuration query failed result=%d size=%zu expected=%zu",
            result, configuration_size, sizeof(configuration));
        return false;
    }

    log_message(
        "MOLTENVK_CONFIG: live_resources=%u metal_argument_buffers=%u "
        "use_mtlheap=%d synchronous_queue_submits=%u command_pooling=%u "
        "prefill=%d",
        configuration.liveCheckAllResources,
        configuration.useMetalArgumentBuffers,
        configuration.useMTLHeap,
        configuration.synchronousQueueSubmits,
        configuration.useCommandPooling,
        configuration.prefillMetalCommandBuffers);

    const MVKConfigUseMTLHeap expected_mtlheap =
        mode == TESO4M4_MODE_LEGACY_ALLOCATION
            ? MVK_CONFIG_USE_MTLHEAP_NEVER
            : MVK_CONFIG_USE_MTLHEAP_WHERE_SAFE;
    const VkBool32 expected_command_pooling =
        mode == TESO4M4_MODE_NO_COMMAND_POOLING ? VK_FALSE : VK_TRUE;
    if (configuration.liveCheckAllResources != VK_TRUE ||
        configuration.useMetalArgumentBuffers != VK_FALSE ||
        configuration.useMTLHeap != expected_mtlheap ||
        configuration.synchronousQueueSubmits != VK_TRUE ||
        configuration.useCommandPooling != expected_command_pooling ||
        configuration.prefillMetalCommandBuffers !=
            MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS_STYLE_NO_PREFILL) {
        log_message("ERROR: MoltenVK configuration differs from selected mode");
        return false;
    }
    return true;
}

static bool restore_rx_pages(const uintptr_t* pages, size_t count,
                             mach_vm_size_t page_size) {
    bool restored = true;
    for (size_t index = 0; index < count; ++index) {
        kern_return_t result = mach_vm_protect(
            mach_task_self(), pages[index], page_size, FALSE,
            VM_PROT_READ | VM_PROT_EXECUTE);
        if (result != KERN_SUCCESS) {
            log_message("ERROR: could not restore RX page 0x%lx: %s",
                        (unsigned long)pages[index], mach_error_string(result));
            restored = false;
        }
    }
    return restored;
}

static void require_rx_restore(const uintptr_t* pages, size_t count,
                               mach_vm_size_t page_size) {
    if (!restore_rx_pages(pages, count, page_size)) {
        log_message("FATAL: code-page permissions could not be restored; exiting");
        _exit(126);
    }
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
            log_message("ERROR: original bytes differ at %s", target->symbol);
            return false;
        }
        destinations[index] = dlsym(moltenvk, target->symbol);
        if (!destinations[index]) {
            log_message("ERROR: MoltenVK does not export %s", target->symbol);
            return false;
        }
        if (strcmp(target->symbol, "vkGetInstanceProcAddr") == 0) {
            g_next_get_instance_proc_addr =
                (PFN_vkGetInstanceProcAddr)destinations[index];
            destinations[index] = (void*)&traced_get_instance_proc_addr;
        } else if (g_reset_trace_enabled) {
            destinations[index] = (void*)teso4m4_reset_trace_intercept(
                target->symbol,
                (PFN_vkVoidFunction)destinations[index]);
        }
    }

    g_next_get_device_proc_addr =
        (PFN_vkGetDeviceProcAddr)dlsym(moltenvk, "vkGetDeviceProcAddr");
    PFN_vkEnumerateDeviceExtensionProperties enumerate_device_extensions =
        (PFN_vkEnumerateDeviceExtensionProperties)dlsym(
            moltenvk, "vkEnumerateDeviceExtensionProperties");
    PFN_vkCreateDevice create_device =
        (PFN_vkCreateDevice)dlsym(moltenvk, "vkCreateDevice");
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)dlsym(
            moltenvk, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    if (!g_next_get_instance_proc_addr || !g_next_get_device_proc_addr ||
        !enumerate_device_extensions || !create_device || !get_surface_formats) {
        log_message("ERROR: required compatibility entry point is unavailable");
        return false;
    }
    teso4m4_compat_set_enumerate_device_extensions(
        enumerate_device_extensions);
    teso4m4_compat_set_create_device(create_device);
    teso4m4_compat_set_get_surface_formats(get_surface_formats);

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
            log_message("ERROR: mach_vm_protect: %s", mach_error_string(result));
            require_rx_restore(writable_pages, writable_page_count,
                               (mach_vm_size_t)page_size);
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

    require_rx_restore(writable_pages, writable_page_count, (mach_vm_size_t)page_size);
    return true;
}

__attribute__((constructor)) static void teso4m4_init(void) {
    initialize_run_id();
    g_log = fopen("/tmp/teso4m4.log", "a");
    if (!g_log) {
        return;
    }
    setvbuf(g_log, NULL, _IOLBF, 0);
    teso4m4_compat_reset();
    teso4m4_compat_set_logger(&compat_log_message);
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&compat_log_message);
    teso4m4_reset_trace_reset();
    teso4m4_reset_trace_set_logger(&compat_log_message);
    g_reset_trace_enabled = false;
    log_message("RUN_START: bridge starting pid=%ld", (long)getpid());

    char directory[4096];
    if (!own_directory(directory, sizeof(directory))) {
        log_message("SKIP: could not resolve bridge directory");
        return;
    }
    const Teso4m4Mode mode = marker_mode(directory);
    if (mode == TESO4M4_MODE_DISABLED) {
        log_message("SKIP: enable marker absent");
        return;
    }
    g_reset_trace_enabled =
        mode == TESO4M4_MODE_RESET_RESOURCE_TRACE ||
        mode == TESO4M4_MODE_NO_COMMAND_POOLING ||
        mode == TESO4M4_MODE_RENDER_AUDIT;
    if (setenv("MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES", "1", 1) != 0 ||
        setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1) != 0 ||
        (mode == TESO4M4_MODE_LEGACY_ALLOCATION &&
         setenv("MVK_CONFIG_USE_MTLHEAP", "0", 1) != 0) ||
        (mode == TESO4M4_MODE_NO_COMMAND_POOLING &&
         setenv("MVK_CONFIG_USE_COMMAND_POOLING", "0", 1) != 0)) {
        log_message("ERROR: could not set selected compatibility mode: %s",
                    strerror(errno));
        return;
    }
    if (mode == TESO4M4_MODE_LEGACY_ALLOCATION) {
        log_message(
            "MODE: legacy allocation enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=0");
    } else if (mode == TESO4M4_MODE_RESET_RESOURCE_TRACE) {
        log_message(
            "MODE: reset resource trace enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1");
    } else if (mode == TESO4M4_MODE_NO_COMMAND_POOLING) {
        log_message(
            "MODE: command pooling disabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=0");
    } else if (mode == TESO4M4_MODE_RENDER_AUDIT) {
        log_message(
            "MODE: render audit enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1");
    } else {
        log_message(
            "MODE: descriptor compatibility enabled live_resources=1 "
            "metal_argument_buffers=0");
    }

    const struct mach_header* raw = _dyld_get_image_header(0);
    if (!raw || raw->magic != MH_MAGIC_64 || raw->cputype != CPU_TYPE_X86_64) {
        log_message("SKIP: unexpected main executable architecture");
        return;
    }
    const struct mach_header_64* header = (const struct mach_header_64*)raw;
    if (!has_expected_uuid(header)) {
        log_message("SKIP: ESO UUID mismatch");
        return;
    }

    char moltenvk_path[4096];
    if (snprintf(moltenvk_path, sizeof(moltenvk_path), "%s/libMoltenVK.teso4m4.dylib",
                 directory) >= (int)sizeof(moltenvk_path)) {
        return;
    }
    void* moltenvk = dlopen(moltenvk_path, RTLD_NOW | RTLD_LOCAL);
    if (!moltenvk) {
        log_message("ERROR: dlopen: %s", dlerror());
        return;
    }
    log_message("MOLTENVK: loaded path=%s", moltenvk_path);
    if (!verify_moltenvk_configuration(moltenvk, mode)) {
        return;
    }
    if (!install_patches(header, moltenvk)) {
        log_message("ERROR: patch transaction aborted");
        return;
    }
    log_message("HDR_COMPAT: filter=%s extension=%s", "enabled",
                VK_EXT_HDR_METADATA_EXTENSION_NAME);
    log_message(
        "HDR_SURFACE_COMPAT: filter=enabled format=%d colorSpace=%d",
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_COLOR_SPACE_HDR10_ST2084_EXT);
    log_message("ACTIVE: redirected %d Vulkan entry points", ESO_TARGET_COUNT);
    log_message("ESO SHA-256: %s", ESO_EXPECTED_SHA256);
}
