#include "mvk_compat.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "readiness_canary_spv.h"

enum { kEnumerationAttempts = 3 };

_Static_assert(
    sizeof(VkPhysicalDeviceFeatures) % sizeof(VkBool32) == 0,
    "VkPhysicalDeviceFeatures must contain complete VkBool32 fields");

static PFN_vkEnumerateDeviceExtensionProperties g_next_enumerate_device_extensions;
static PFN_vkCreateDevice g_next_create_device;
static PFN_vkGetDeviceProcAddr g_next_get_device_proc_addr;
static PFN_vkDestroyDevice g_next_destroy_device;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR g_next_get_surface_formats;
static Teso4m4CompatLogFunction g_logger;
static atomic_uint_fast64_t g_create_call_counter;
static atomic_uint_fast32_t g_readiness_nonce_counter;
static bool g_device_readiness_required;

static void compat_log(const char* format, ...) {
    if (!g_logger) {
        return;
    }
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    g_logger(message);
}

void teso4m4_compat_reset(void) {
    g_next_enumerate_device_extensions = NULL;
    g_next_create_device = NULL;
    g_next_get_device_proc_addr = NULL;
    g_next_destroy_device = NULL;
    g_next_get_surface_formats = NULL;
    g_logger = NULL;
    g_device_readiness_required = false;
    atomic_store_explicit(&g_create_call_counter, 0, memory_order_relaxed);
    atomic_store_explicit(&g_readiness_nonce_counter, 0, memory_order_relaxed);
}

void teso4m4_compat_set_logger(Teso4m4CompatLogFunction logger) {
    g_logger = logger;
}

void teso4m4_compat_set_enumerate_device_extensions(
    PFN_vkEnumerateDeviceExtensionProperties next_function) {
    g_next_enumerate_device_extensions = next_function;
}

void teso4m4_compat_set_create_device(PFN_vkCreateDevice next_function) {
    g_next_create_device = next_function;
}

void teso4m4_compat_set_device_readiness(
    PFN_vkGetDeviceProcAddr get_device_proc_addr,
    PFN_vkDestroyDevice destroy_device,
    bool required) {
    g_next_get_device_proc_addr = get_device_proc_addr;
    g_next_destroy_device = destroy_device;
    g_device_readiness_required = required;
}

void teso4m4_compat_set_get_surface_formats(
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR next_function) {
    g_next_get_surface_formats = next_function;
}

static bool is_eso_hdr_surface_format(const VkSurfaceFormatKHR* surface_format) {
    return surface_format->format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
           surface_format->colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT;
}

static VkResult load_raw_surface_formats(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    uint32_t* raw_count,
    VkSurfaceFormatKHR** raw_formats) {
    for (unsigned int attempt = 1; attempt <= kEnumerationAttempts; ++attempt) {
        uint32_t count = 0;
        VkResult result = g_next_get_surface_formats(
            physical_device, surface, &count, NULL);
        if (result != VK_SUCCESS) {
            compat_log(
                "SURFACE_FORMAT_FILTER_ERROR: raw count attempt=%u result=%d",
                attempt, result);
            return result;
        }
        if (count == 0) {
            *raw_count = 0;
            *raw_formats = NULL;
            return VK_SUCCESS;
        }

        VkSurfaceFormatKHR* formats =
            calloc((size_t)count, sizeof(*formats));
        if (!formats) {
            compat_log(
                "SURFACE_FORMAT_FILTER_ERROR: allocation failed raw_count=%u",
                count);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        uint32_t written = count;
        result = g_next_get_surface_formats(
            physical_device, surface, &written, formats);
        if (result == VK_SUCCESS && written <= count) {
            *raw_count = written;
            *raw_formats = formats;
            return VK_SUCCESS;
        }
        free(formats);
        if (result != VK_INCOMPLETE && result != VK_SUCCESS) {
            compat_log(
                "SURFACE_FORMAT_FILTER_ERROR: raw data attempt=%u result=%d",
                attempt, result);
            return result;
        }
        compat_log(
            "SURFACE_FORMAT_FILTER_RETRY: attempt=%u count=%u written=%u result=%d",
            attempt, count, written, result);
    }

    compat_log("SURFACE_FORMAT_FILTER_ERROR: raw list did not stabilize");
    return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR VkResult VKAPI_CALL teso4m4_get_physical_device_surface_formats(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    uint32_t* surface_format_count,
    VkSurfaceFormatKHR* surface_formats) {
    if (!g_next_get_surface_formats) {
        compat_log("SURFACE_FORMAT_FILTER_ERROR: next function is unset");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (!surface_format_count) {
        compat_log("SURFACE_FORMAT_FILTER_ERROR: count is NULL");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t raw_count = 0;
    VkSurfaceFormatKHR* raw_formats = NULL;
    VkResult result = load_raw_surface_formats(
        physical_device, surface, &raw_count, &raw_formats);
    if (result != VK_SUCCESS) {
        *surface_format_count = 0;
        return result;
    }

    uint32_t visible_count = 0;
    uint32_t removed_count = 0;
    for (uint32_t index = 0; index < raw_count; ++index) {
        if (is_eso_hdr_surface_format(&raw_formats[index])) {
            ++removed_count;
        } else {
            ++visible_count;
        }
    }

    if (!surface_formats) {
        *surface_format_count = visible_count;
        compat_log(
            "SURFACE_FORMAT_FILTER: physical=%p surface=%p raw=%u visible=%u removed=%u query=count result=%d",
            (void*)physical_device, (void*)surface, raw_count, visible_count,
            removed_count, VK_SUCCESS);
        free(raw_formats);
        return VK_SUCCESS;
    }

    const uint32_t capacity = *surface_format_count;
    uint32_t written = 0;
    for (uint32_t index = 0; index < raw_count; ++index) {
        if (is_eso_hdr_surface_format(&raw_formats[index])) {
            continue;
        }
        if (written < capacity) {
            surface_formats[written] = raw_formats[index];
        }
        ++written;
    }
    const VkResult filtered_result =
        capacity < visible_count ? VK_INCOMPLETE : VK_SUCCESS;
    *surface_format_count = capacity < visible_count ? capacity : visible_count;
    compat_log(
        "SURFACE_FORMAT_FILTER: physical=%p surface=%p raw=%u visible=%u removed=%u query=data capacity=%u written=%u result=%d",
        (void*)physical_device, (void*)surface, raw_count, visible_count,
        removed_count, capacity, *surface_format_count, filtered_result);
    free(raw_formats);
    return filtered_result;
}

static VkResult load_raw_device_extensions(
    VkPhysicalDevice physical_device,
    uint32_t* raw_count,
    VkExtensionProperties** raw_properties) {
    for (unsigned int attempt = 1; attempt <= kEnumerationAttempts; ++attempt) {
        uint32_t count = 0;
        VkResult result = g_next_enumerate_device_extensions(
            physical_device, NULL, &count, NULL);
        if (result != VK_SUCCESS) {
            compat_log("HDR_FILTER_ERROR: raw count attempt=%u result=%d", attempt,
                       result);
            return result;
        }
        if (count == 0) {
            *raw_count = 0;
            *raw_properties = NULL;
            return VK_SUCCESS;
        }

        VkExtensionProperties* properties =
            calloc((size_t)count, sizeof(*properties));
        if (!properties) {
            compat_log("HDR_FILTER_ERROR: allocation failed raw_count=%u", count);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        uint32_t written = count;
        result = g_next_enumerate_device_extensions(
            physical_device, NULL, &written, properties);
        if (result == VK_SUCCESS && written <= count) {
            *raw_count = written;
            *raw_properties = properties;
            return VK_SUCCESS;
        }
        free(properties);
        if (result != VK_INCOMPLETE && result != VK_SUCCESS) {
            compat_log("HDR_FILTER_ERROR: raw data attempt=%u result=%d", attempt,
                       result);
            return result;
        }
        compat_log("HDR_FILTER_RETRY: attempt=%u count=%u written=%u result=%d",
                   attempt, count, written, result);
    }

    compat_log("HDR_FILTER_ERROR: raw extension list did not stabilize");
    return VK_ERROR_INITIALIZATION_FAILED;
}

static uint64_t elapsed_nanoseconds(
    const struct timespec* start,
    const struct timespec* end) {
    const uint64_t seconds = (uint64_t)(end->tv_sec - start->tv_sec);
    if (end->tv_nsec >= start->tv_nsec) {
        return seconds * UINT64_C(1000000000) +
               (uint64_t)(end->tv_nsec - start->tv_nsec);
    }
    return (seconds - 1) * UINT64_C(1000000000) +
           (uint64_t)(UINT64_C(1000000000) + end->tv_nsec - start->tv_nsec);
}

static VkResult run_device_readiness_canary(
    VkDevice device,
    const VkAllocationCallbacks* allocator) {
    if (!g_device_readiness_required) {
        return VK_SUCCESS;
    }
    if (!device || !g_next_get_device_proc_addr || !g_next_destroy_device) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=resolve-core result=%d",
            VK_ERROR_INITIALIZATION_FAILED);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

#define LOAD_DEVICE_PROC(type, variable, name) \
    type variable = (type)g_next_get_device_proc_addr(device, name)
    LOAD_DEVICE_PROC(
        PFN_vkCreateShaderModule, create_shader_module,
        "vkCreateShaderModule");
    LOAD_DEVICE_PROC(
        PFN_vkDestroyShaderModule, destroy_shader_module,
        "vkDestroyShaderModule");
    LOAD_DEVICE_PROC(
        PFN_vkCreateDescriptorSetLayout, create_descriptor_set_layout,
        "vkCreateDescriptorSetLayout");
    LOAD_DEVICE_PROC(
        PFN_vkDestroyDescriptorSetLayout, destroy_descriptor_set_layout,
        "vkDestroyDescriptorSetLayout");
    LOAD_DEVICE_PROC(
        PFN_vkCreatePipelineLayout, create_pipeline_layout,
        "vkCreatePipelineLayout");
    LOAD_DEVICE_PROC(
        PFN_vkDestroyPipelineLayout, destroy_pipeline_layout,
        "vkDestroyPipelineLayout");
    LOAD_DEVICE_PROC(
        PFN_vkCreateComputePipelines, create_compute_pipelines,
        "vkCreateComputePipelines");
    LOAD_DEVICE_PROC(
        PFN_vkDestroyPipeline, destroy_pipeline,
        "vkDestroyPipeline");
#undef LOAD_DEVICE_PROC

    if (!create_shader_module || !destroy_shader_module ||
        !create_descriptor_set_layout || !destroy_descriptor_set_layout ||
        !create_pipeline_layout || !destroy_pipeline_layout ||
        !create_compute_pipelines || !destroy_pipeline) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=resolve-functions result=%d",
            VK_ERROR_INITIALIZATION_FAILED);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    struct timespec start = {0};
    struct timespec end = {0};
    (void)clock_gettime(CLOCK_MONOTONIC, &start);

    uint32_t canary_spirv[
        sizeof(kTeso4m4ReadinessCanarySpirv) /
        sizeof(kTeso4m4ReadinessCanarySpirv[0])];
    memcpy(
        canary_spirv, kTeso4m4ReadinessCanarySpirv,
        sizeof(canary_spirv));
    if (canary_spirv[kTeso4m4ReadinessCanaryNonceWord] !=
        kTeso4m4ReadinessCanaryNonceTemplate) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=prepare-shader result=%d",
            VK_ERROR_INITIALIZATION_FAILED);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const uint32_t nonce_counter = atomic_fetch_add_explicit(
        &g_readiness_nonce_counter, 1, memory_order_relaxed);
    const uint32_t nonce =
        (uint32_t)start.tv_sec ^ (uint32_t)start.tv_nsec ^
        (uint32_t)getpid() ^ nonce_counter;
    canary_spirv[kTeso4m4ReadinessCanaryNonceWord] = nonce;

    VkShaderModule shader = VK_NULL_HANDLE;
    const VkShaderModuleCreateInfo shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(canary_spirv),
        .pCode = canary_spirv,
    };
    VkResult result = create_shader_module(
        device, &shader_info, allocator, &shader);
    if (result != VK_SUCCESS || shader == VK_NULL_HANDLE) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=create-shader result=%d",
            result);
        if (shader != VK_NULL_HANDLE) {
            destroy_shader_module(device, shader, allocator);
        }
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    }

    const VkDescriptorSetLayoutBinding canary_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &canary_binding,
    };
    VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
    result = create_descriptor_set_layout(
        device, &descriptor_layout_info, allocator, &descriptor_layout);
    if (result != VK_SUCCESS || descriptor_layout == VK_NULL_HANDLE) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=create-descriptor-layout result=%d",
            result);
        if (descriptor_layout != VK_NULL_HANDLE) {
            destroy_descriptor_set_layout(
                device, descriptor_layout, allocator);
        }
        destroy_shader_module(device, shader, allocator);
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    }

    VkPipelineLayout layout = VK_NULL_HANDLE;
    const VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
    };
    result = create_pipeline_layout(
        device, &layout_info, allocator, &layout);
    if (result != VK_SUCCESS || layout == VK_NULL_HANDLE) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=create-layout result=%d",
            result);
        if (layout != VK_NULL_HANDLE) {
            destroy_pipeline_layout(device, layout, allocator);
        }
        destroy_descriptor_set_layout(device, descriptor_layout, allocator);
        destroy_shader_module(device, shader, allocator);
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    }

    const VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader,
        .pName = "main",
    };
    const VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stage_info,
        .layout = layout,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    result = create_compute_pipelines(
        device, VK_NULL_HANDLE, 1, &pipeline_info, allocator, &pipeline);

    if (pipeline != VK_NULL_HANDLE) {
        destroy_pipeline(device, pipeline, allocator);
    }
    destroy_pipeline_layout(device, layout, allocator);
    destroy_descriptor_set_layout(device, descriptor_layout, allocator);
    destroy_shader_module(device, shader, allocator);

    (void)clock_gettime(CLOCK_MONOTONIC, &end);
    const uint64_t duration_ns = elapsed_nanoseconds(&start, &end);
    if (result != VK_SUCCESS || pipeline == VK_NULL_HANDLE) {
        compat_log(
            "RUNTIME_READINESS_ERROR: stage=create-compute-pipeline result=%d duration_ns=%" PRIu64,
            result, duration_ns);
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    }

    compat_log(
        "RUNTIME_READINESS: compiler_canary=pass cache=none unique_shader=yes duration_ns=%" PRIu64,
        duration_ns);
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL teso4m4_enumerate_device_extension_properties(
    VkPhysicalDevice physical_device,
    const char* layer_name,
    uint32_t* property_count,
    VkExtensionProperties* properties) {
    if (!g_next_enumerate_device_extensions) {
        compat_log("HDR_FILTER_ERROR: next enumerate function is unset");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (!property_count) {
        compat_log("HDR_FILTER_ERROR: property_count is NULL");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (layer_name) {
        VkResult result = g_next_enumerate_device_extensions(
            physical_device, layer_name, property_count, properties);
        compat_log("HDR_FILTER_LAYER_PASSTHROUGH: layer=%s result=%d count=%u",
                   layer_name, result, *property_count);
        return result;
    }

    uint32_t raw_count = 0;
    VkExtensionProperties* raw_properties = NULL;
    VkResult result = load_raw_device_extensions(
        physical_device, &raw_count, &raw_properties);
    if (result != VK_SUCCESS) {
        *property_count = 0;
        return result;
    }

    uint32_t visible_count = 0;
    uint32_t removed_count = 0;
    for (uint32_t index = 0; index < raw_count; ++index) {
        if (strcmp(raw_properties[index].extensionName,
                   VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
            ++removed_count;
        } else {
            ++visible_count;
        }
    }

    if (!properties) {
        *property_count = visible_count;
        compat_log(
            "HDR_FILTER: physical=%p raw=%u visible=%u removed=%u query=count result=%d",
            (void*)physical_device, raw_count, visible_count, removed_count,
            VK_SUCCESS);
        free(raw_properties);
        return VK_SUCCESS;
    }

    const uint32_t capacity = *property_count;
    uint32_t written = 0;
    for (uint32_t index = 0; index < raw_count; ++index) {
        if (strcmp(raw_properties[index].extensionName,
                   VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
            continue;
        }
        if (written < capacity) {
            properties[written] = raw_properties[index];
        }
        ++written;
    }
    const VkResult filtered_result =
        capacity < visible_count ? VK_INCOMPLETE : VK_SUCCESS;
    *property_count = capacity < visible_count ? capacity : visible_count;
    compat_log(
        "HDR_FILTER: physical=%p raw=%u visible=%u removed=%u query=data capacity=%u written=%u result=%d",
        (void*)physical_device, raw_count, visible_count, removed_count, capacity,
        *property_count, filtered_result);
    free(raw_properties);
    return filtered_result;
}

VKAPI_ATTR VkResult VKAPI_CALL teso4m4_create_device(
    VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDevice* device) {
    if (!g_next_create_device) {
        compat_log("CREATE_DEVICE_ERROR: next create function is unset");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const uint64_t call_id = atomic_fetch_add_explicit(
                                 &g_create_call_counter, 1,
                                 memory_order_relaxed) +
                             1;

    const uint32_t extension_count =
        create_info ? create_info->enabledExtensionCount : 0;
    const char* const* extension_names =
        create_info ? create_info->ppEnabledExtensionNames : NULL;
    bool hdr_enabled = false;
    if (extension_count > 0 && !extension_names) {
        compat_log("CREATE_DEVICE_ERROR: extension count=%u but names are NULL",
                   extension_count);
    }
    for (uint32_t index = 0; extension_names && index < extension_count; ++index) {
        const char* name = extension_names[index];
        if (name && strcmp(name, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
            hdr_enabled = true;
        }
    }

    compat_log("CREATE_DEVICE: call=%" PRIu64
               " physical=%p extensions=%u hdr_enabled=%s",
               call_id, (void*)physical_device, extension_count,
               hdr_enabled ? "yes" : "no");
    for (uint32_t index = 0; extension_names && index < extension_count; ++index) {
        compat_log("CREATE_DEVICE_EXT: call=%" PRIu64 " index=%u name=%s",
                   call_id, index,
                   extension_names[index] ? extension_names[index] : "(null)");
    }

    VkResult result = g_next_create_device(
        physical_device, create_info, allocator, device);
    compat_log("CREATE_DEVICE_RESULT: call=%" PRIu64 " result=%d device=%p",
               call_id, result, device ? (void*)*device : NULL);
    if (result == VK_SUCCESS && device && *device != VK_NULL_HANDLE) {
        const VkResult readiness_result =
            run_device_readiness_canary(*device, allocator);
        if (readiness_result != VK_SUCCESS) {
            if (g_next_destroy_device) {
                g_next_destroy_device(*device, allocator);
            }
            *device = VK_NULL_HANDLE;
            compat_log(
                "CREATE_DEVICE_ERROR: call=%" PRIu64
                " readiness_result=%d device_destroyed=yes",
                call_id, readiness_result);
            return readiness_result;
        }
    }
    return result;
}
