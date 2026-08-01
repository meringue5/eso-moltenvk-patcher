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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <MoltenVK/mvk_private_api.h>

#include "mvk_compat.h"
#include "eso_fx_sentinel.h"
#include "mvk_lifecycle.h"
#include "mvk_present_pixel.h"
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
static bool g_legacy_feature_profile_enabled;
static bool g_startup_color_audit_enabled;
static bool g_startup_present_pixel_audit_enabled;
static bool g_startup_draw_audit_enabled;
static bool g_startup_input_audit_enabled;
static bool g_startup_compositor_audit_enabled;
static bool g_startup_compositor_neutralize_enabled;

typedef enum {
    TESO4M4_LOG_ERROR = 0,
    TESO4M4_LOG_WARN,
    TESO4M4_LOG_INFO,
    TESO4M4_LOG_DEBUG,
    TESO4M4_LOG_TRACE,
} Teso4m4LogLevel;

static Teso4m4LogLevel g_log_level = TESO4M4_LOG_INFO;

typedef enum {
    TESO4M4_MODE_DISABLED = 0,
    TESO4M4_MODE_DESCRIPTOR_COMPAT,
    TESO4M4_MODE_LEGACY_ALLOCATION,
    TESO4M4_MODE_RESET_RESOURCE_TRACE,
    TESO4M4_MODE_NO_COMMAND_POOLING,
    TESO4M4_MODE_RENDER_AUDIT,
    TESO4M4_MODE_RESET_NO_PIPELINE_CACHE,
    TESO4M4_MODE_FULL_LIFETIME_AUDIT,
    TESO4M4_MODE_TEXTURE_CACHE_FIX,
    TESO4M4_MODE_LEGACY_FEATURE_PROFILE,
    TESO4M4_MODE_PERFORMANCE_SAFE,
    TESO4M4_MODE_PERFORMANCE_AGGRESSIVE,
    TESO4M4_MODE_STARTUP_COLOR_AUDIT,
    TESO4M4_MODE_STARTUP_FX_NEUTRALIZE,
    TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT,
    TESO4M4_MODE_STARTUP_DRAW_AUDIT,
    TESO4M4_MODE_STARTUP_INPUT_AUDIT,
    TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT,
    TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE,
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

static const char* log_level_name(Teso4m4LogLevel level) {
    switch (level) {
        case TESO4M4_LOG_ERROR:
            return "error";
        case TESO4M4_LOG_WARN:
            return "warn";
        case TESO4M4_LOG_INFO:
            return "info";
        case TESO4M4_LOG_DEBUG:
            return "debug";
        case TESO4M4_LOG_TRACE:
            return "trace";
    }
    return "info";
}

static void configure_log_level(void) {
    const char* requested = getenv("TESO4M4_LOG_LEVEL");
    if (!requested || strcmp(requested, "") == 0 ||
        strcmp(requested, "info") == 0) {
        return;
    }
    if (strcmp(requested, "error") == 0) {
        g_log_level = TESO4M4_LOG_ERROR;
    } else if (strcmp(requested, "warn") == 0) {
        g_log_level = TESO4M4_LOG_WARN;
    } else if (strcmp(requested, "debug") == 0) {
        g_log_level = TESO4M4_LOG_DEBUG;
    } else if (strcmp(requested, "trace") == 0) {
        g_log_level = TESO4M4_LOG_TRACE;
    }
}

static bool starts_with(const char* message, const char* prefix) {
    return strncmp(message, prefix, strlen(prefix)) == 0;
}

static Teso4m4LogLevel classify_log_message(const char* message) {
    if (starts_with(message, "ERROR:") || starts_with(message, "FATAL:") ||
        starts_with(message, "GIPA_ERROR:") ||
        starts_with(message, "GDPA_ERROR:") ||
        strstr(message, "_ERROR:") != NULL) {
        return TESO4M4_LOG_ERROR;
    }
    if (starts_with(message, "SKIP:")) {
        return TESO4M4_LOG_WARN;
    }
    if (starts_with(message, "GIPA:") || starts_with(message, "GDPA:") ||
        starts_with(message, "STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS:") ||
        starts_with(message, "STARTUP_COLOR_")) {
        return TESO4M4_LOG_TRACE;
    }
    if (starts_with(message, "RUN_START:") || starts_with(message, "MODE:") ||
        starts_with(message, "MOLTENVK_CONFIG:") ||
        starts_with(message, "MOLTENVK:") || starts_with(message, "HDR_") ||
        starts_with(message, "ACTIVE:") ||
        starts_with(message, "ESO SHA-256:") ||
        starts_with(message, "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN:") ||
        starts_with(message, "STARTUP_COMPOSITOR_NEUTRALIZE_LATCH:")) {
        return TESO4M4_LOG_INFO;
    }
    return TESO4M4_LOG_DEBUG;
}

static FILE* open_production_log(void) {
    const char* home = getenv("HOME");
    if (home && home[0] != '\0') {
        char library[4096];
        char logs[4096];
        char product[4096];
        char path[4096];
        if (snprintf(library, sizeof(library), "%s/Library", home) <
                (int)sizeof(library) &&
            snprintf(logs, sizeof(logs), "%s/Logs", library) <
                (int)sizeof(logs) &&
            snprintf(product, sizeof(product), "%s/ESO MoltenVK Patcher", logs) <
                (int)sizeof(product) &&
            snprintf(path, sizeof(path), "%s/bridge.log", product) <
                (int)sizeof(path)) {
            if ((mkdir(library, 0700) == 0 || errno == EEXIST) &&
                (mkdir(logs, 0700) == 0 || errno == EEXIST) &&
                (mkdir(product, 0700) == 0 || errno == EEXIST)) {
                FILE* file = fopen(path, "a");
                if (file) {
                    return file;
                }
            }
        }
    }
    return fopen("/tmp/teso4m4.log", "a");
}

static void log_message(const char* format, ...) {
    if (!g_log) {
        return;
    }
    char message[4096];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (classify_log_message(message) > g_log_level) {
        return;
    }
    flockfile(g_log);
    fprintf(g_log, "[run=%s] ", g_run_id);
    fputs(message, g_log);
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
    } else if (raw_result && name && g_legacy_feature_profile_enabled &&
               strcmp(name, "vkGetPhysicalDeviceFeatures") == 0) {
        returned_result =
            (PFN_vkVoidFunction)&teso4m4_get_physical_device_features;
        shim = "legacy-feature-profile";
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
    if (strcmp(mode, "reset-no-pipeline-cache") == 0) {
        return TESO4M4_MODE_RESET_NO_PIPELINE_CACHE;
    }
    if (strcmp(mode, "full-lifetime-audit") == 0) {
        return TESO4M4_MODE_FULL_LIFETIME_AUDIT;
    }
    if (strcmp(mode, "texture-cache-fix") == 0) {
        return TESO4M4_MODE_TEXTURE_CACHE_FIX;
    }
    if (strcmp(mode, "legacy-feature-profile") == 0) {
        return TESO4M4_MODE_LEGACY_FEATURE_PROFILE;
    }
    if (strcmp(mode, "performance-safe") == 0) {
        return TESO4M4_MODE_PERFORMANCE_SAFE;
    }
    if (strcmp(mode, "performance-aggressive") == 0) {
        return TESO4M4_MODE_PERFORMANCE_AGGRESSIVE;
    }
    if (strcmp(mode, "startup-color-audit") == 0) {
        return TESO4M4_MODE_STARTUP_COLOR_AUDIT;
    }
    if (strcmp(mode, "startup-fx-neutralize") == 0) {
        return TESO4M4_MODE_STARTUP_FX_NEUTRALIZE;
    }
    if (strcmp(mode, "startup-present-pixel-audit") == 0) {
        return TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT;
    }
    if (strcmp(mode, "startup-draw-audit") == 0) {
        return TESO4M4_MODE_STARTUP_DRAW_AUDIT;
    }
    if (strcmp(mode, "startup-input-audit") == 0) {
        return TESO4M4_MODE_STARTUP_INPUT_AUDIT;
    }
    if (strcmp(mode, "startup-compositor-audit") == 0) {
        return TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT;
    }
    if (strcmp(mode, "startup-compositor-neutralize") == 0) {
        return TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
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
        "prefill=%d maximize_concurrent_compilation=%u",
        configuration.liveCheckAllResources,
        configuration.useMetalArgumentBuffers,
        configuration.useMTLHeap,
        configuration.synchronousQueueSubmits,
        configuration.useCommandPooling,
        configuration.prefillMetalCommandBuffers,
        configuration.shouldMaximizeConcurrentCompilation);

    const MVKConfigUseMTLHeap expected_mtlheap =
        mode == TESO4M4_MODE_LEGACY_ALLOCATION
            ? MVK_CONFIG_USE_MTLHEAP_NEVER
            : MVK_CONFIG_USE_MTLHEAP_WHERE_SAFE;
    const VkBool32 expected_command_pooling =
        mode == TESO4M4_MODE_NO_COMMAND_POOLING ? VK_FALSE : VK_TRUE;
    const bool performance_mode =
        mode == TESO4M4_MODE_PERFORMANCE_SAFE ||
        mode == TESO4M4_MODE_PERFORMANCE_AGGRESSIVE ||
        mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE ||
        mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    const VkBool32 expected_live_resources =
        (mode == TESO4M4_MODE_PERFORMANCE_AGGRESSIVE ||
         mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT ||
         mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE ||
         mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
         mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
         mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
         mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
         mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE)
            ? VK_FALSE
            : VK_TRUE;
    const VkBool32 expected_synchronous_submits =
        performance_mode ? VK_FALSE : VK_TRUE;
    const VkBool32 expected_concurrent_compilation =
        performance_mode ? VK_TRUE : VK_FALSE;
    if (configuration.liveCheckAllResources != expected_live_resources ||
        configuration.useMetalArgumentBuffers != VK_FALSE ||
        configuration.useMTLHeap != expected_mtlheap ||
        configuration.synchronousQueueSubmits != expected_synchronous_submits ||
        configuration.useCommandPooling != expected_command_pooling ||
        configuration.prefillMetalCommandBuffers !=
            MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS_STYLE_NO_PREFILL ||
        configuration.shouldMaximizeConcurrentCompilation !=
            expected_concurrent_compilation) {
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
        } else if (g_startup_color_audit_enabled) {
            destinations[index] = (void*)teso4m4_lifecycle_intercept(
                target->symbol,
                (PFN_vkVoidFunction)destinations[index]);
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
    PFN_vkGetPhysicalDeviceFeatures get_physical_device_features =
        (PFN_vkGetPhysicalDeviceFeatures)dlsym(
            moltenvk, "vkGetPhysicalDeviceFeatures");
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)dlsym(
            moltenvk, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    if (!g_next_get_instance_proc_addr || !g_next_get_device_proc_addr ||
        !enumerate_device_extensions || !create_device ||
        !get_physical_device_features || !get_surface_formats) {
        log_message("ERROR: required compatibility entry point is unavailable");
        return false;
    }
    teso4m4_compat_set_enumerate_device_extensions(
        enumerate_device_extensions);
    teso4m4_compat_set_create_device(create_device);
    teso4m4_compat_set_get_physical_device_features(
        get_physical_device_features);
    teso4m4_compat_set_legacy_feature_profile_enabled(
        g_legacy_feature_profile_enabled);
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
    configure_log_level();
    g_log = open_production_log();
    if (!g_log) {
        return;
    }
    setvbuf(g_log, NULL, _IOLBF, 0);
    teso4m4_compat_reset();
    teso4m4_compat_set_logger(&compat_log_message);
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&compat_log_message);
    teso4m4_present_pixel_reset();
    teso4m4_reset_trace_reset();
    teso4m4_reset_trace_set_logger(&compat_log_message);
    teso4m4_fx_sentinel_reset();
    teso4m4_fx_sentinel_set_logger(&compat_log_message);
    teso4m4_fx_sentinel_set_window_function(
        &teso4m4_lifecycle_startup_window_open);
    g_reset_trace_enabled = false;
    g_legacy_feature_profile_enabled = false;
    g_startup_color_audit_enabled = false;
    g_startup_present_pixel_audit_enabled = false;
    g_startup_draw_audit_enabled = false;
    g_startup_input_audit_enabled = false;
    g_startup_compositor_audit_enabled = false;
    g_startup_compositor_neutralize_enabled = false;
    log_message("RUN_START: bridge starting log_level=%s",
                log_level_name(g_log_level));

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
    if (mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE &&
        !ESO_HAS_FX_SENTINEL_TARGET) {
        log_message("SKIP: selected target has no FX sentinel profile");
        return;
    }
    g_reset_trace_enabled =
        mode == TESO4M4_MODE_RESET_RESOURCE_TRACE ||
        mode == TESO4M4_MODE_NO_COMMAND_POOLING ||
        mode == TESO4M4_MODE_RENDER_AUDIT ||
        mode == TESO4M4_MODE_RESET_NO_PIPELINE_CACHE ||
        mode == TESO4M4_MODE_FULL_LIFETIME_AUDIT;
    g_legacy_feature_profile_enabled =
        mode == TESO4M4_MODE_LEGACY_FEATURE_PROFILE;
    g_startup_color_audit_enabled =
        mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE ||
        mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    g_startup_present_pixel_audit_enabled =
        mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT;
    g_startup_draw_audit_enabled =
        mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    g_startup_input_audit_enabled =
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    g_startup_compositor_audit_enabled =
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT;
    g_startup_compositor_neutralize_enabled =
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    const bool performance_mode =
        mode == TESO4M4_MODE_PERFORMANCE_SAFE ||
        mode == TESO4M4_MODE_PERFORMANCE_AGGRESSIVE ||
        mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE ||
        mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
        mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE;
    teso4m4_lifecycle_set_enabled(
        !performance_mode || g_startup_color_audit_enabled);
    teso4m4_lifecycle_set_startup_color_audit(
        g_startup_color_audit_enabled);
    teso4m4_lifecycle_set_startup_present_pixel_audit(
        g_startup_present_pixel_audit_enabled);
    teso4m4_lifecycle_set_startup_draw_audit(
        g_startup_draw_audit_enabled);
    teso4m4_lifecycle_set_startup_input_audit(
        g_startup_input_audit_enabled);
    teso4m4_lifecycle_set_startup_compositor_audit(
        g_startup_compositor_audit_enabled);
    teso4m4_lifecycle_set_startup_compositor_neutralize(
        g_startup_compositor_neutralize_enabled);
    teso4m4_reset_trace_set_pipeline_cache_bypass(
        mode == TESO4M4_MODE_RESET_NO_PIPELINE_CACHE);
    teso4m4_reset_trace_set_full_lifetime_audit(
        mode == TESO4M4_MODE_FULL_LIFETIME_AUDIT);
    if (setenv(
            "MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES",
            (mode == TESO4M4_MODE_PERFORMANCE_AGGRESSIVE ||
             mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT ||
             mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE ||
             mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT ||
             mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT ||
             mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT ||
             mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT ||
             mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE) ? "0" : "1",
            1) != 0 ||
        setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1) != 0 ||
        (mode == TESO4M4_MODE_LEGACY_ALLOCATION &&
         setenv("MVK_CONFIG_USE_MTLHEAP", "0", 1) != 0) ||
        (mode == TESO4M4_MODE_NO_COMMAND_POOLING &&
         setenv("MVK_CONFIG_USE_COMMAND_POOLING", "0", 1) != 0) ||
        (performance_mode &&
         (setenv("MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS", "0", 1) != 0 ||
          setenv(
              "MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION",
              "1", 1) != 0))) {
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
    } else if (mode == TESO4M4_MODE_RESET_NO_PIPELINE_CACHE) {
        log_message(
            "MODE: reset pipeline cache bypass enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1");
    } else if (mode == TESO4M4_MODE_FULL_LIFETIME_AUDIT) {
        log_message(
            "MODE: full lifetime audit enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1");
    } else if (mode == TESO4M4_MODE_TEXTURE_CACHE_FIX) {
        log_message(
            "MODE: texture cache fix enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1");
    } else if (mode == TESO4M4_MODE_LEGACY_FEATURE_PROFILE) {
        log_message(
            "MODE: legacy feature profile enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1");
    } else if (mode == TESO4M4_MODE_PERFORMANCE_SAFE) {
        log_message(
            "MODE: performance safe enabled live_resources=1 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "lifecycle_trace=0");
    } else if (mode == TESO4M4_MODE_PERFORMANCE_AGGRESSIVE) {
        log_message(
            "MODE: performance aggressive enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "lifecycle_trace=0");
    } else if (mode == TESO4M4_MODE_STARTUP_COLOR_AUDIT) {
        log_message(
            "MODE: startup color audit enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180");
    } else if (mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE) {
        log_message(
            "MODE: startup FX neutralize enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180");
    } else if (mode == TESO4M4_MODE_STARTUP_PRESENT_PIXEL_AUDIT) {
        log_message(
            "MODE: startup present pixel audit enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "pixel_samples=20");
    } else if (mode == TESO4M4_MODE_STARTUP_DRAW_AUDIT) {
        log_message(
            "MODE: startup draw audit enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "pixel_samples=20 draw_provenance=enabled");
    } else if (mode == TESO4M4_MODE_STARTUP_INPUT_AUDIT) {
        log_message(
            "MODE: startup input audit enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "pixel_samples=20 draw_provenance=enabled "
            "input_provenance=enabled");
    } else if (mode == TESO4M4_MODE_STARTUP_COMPOSITOR_AUDIT) {
        log_message(
            "MODE: startup compositor audit enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "pixel_samples=20 draw_provenance=enabled "
            "input_provenance=enabled descriptor_classes=enabled");
    } else if (mode == TESO4M4_MODE_STARTUP_COMPOSITOR_NEUTRALIZE) {
        log_message(
            "MODE: startup compositor neutralize enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "draw_provenance=enabled input_provenance=enabled "
            "pixel_readback=disabled fallback=forward");
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
    if (g_startup_present_pixel_audit_enabled) {
        PFN_vkVoidFunction get_mtl_texture =
            (PFN_vkVoidFunction)dlsym(moltenvk, "vkGetMTLTextureMVK");
        PFN_vkQueueWaitIdle queue_wait_idle =
            (PFN_vkQueueWaitIdle)dlsym(moltenvk, "vkQueueWaitIdle");
        if (!teso4m4_present_pixel_configure(
                get_mtl_texture, queue_wait_idle, &compat_log_message)) {
            log_message(
                "ERROR: startup present pixel sampler is unavailable");
            return;
        }
        teso4m4_lifecycle_set_present_pixel_sampler(
            &teso4m4_present_pixel_sample);
        if (g_startup_compositor_audit_enabled) {
            teso4m4_lifecycle_set_compositor_image_sampler(
                &teso4m4_present_pixel_sample_compositor_image);
        }
        log_message(
            "STARTUP_PRESENT_PIXEL_READY: synchronization=queue-wait-idle "
            "samples=20 points_per_sample=5");
        if (g_startup_compositor_audit_enabled) {
            log_message(
                "STARTUP_COMPOSITOR_IMAGE_READY: synchronization="
                "queue-wait-idle points_per_image=5 formats="
                "rgba8,bgra8,rgba16f");
        }
    }
    if (!install_patches(header, moltenvk)) {
        log_message("ERROR: patch transaction aborted");
        return;
    }
    if (mode == TESO4M4_MODE_STARTUP_FX_NEUTRALIZE &&
        !teso4m4_fx_sentinel_install(header, &kFxSentinelTarget)) {
        log_message("ERROR: startup FX sentinel patch was not installed");
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
