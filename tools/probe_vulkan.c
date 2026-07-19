#define VK_USE_PLATFORM_MACOS_MVK 1
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "mvk_compat.h"

typedef struct {
    bool hdr_gipa;
    bool hdr_gdpa;
} ProcAddressReport;

static const char* kEsoProcNames[] = {
    "vkCmdClearAttachments",
    "vkCmdBindVertexBuffers",
    "vkCmdBindIndexBuffer",
    "vkCmdBindPipeline",
    "vkCmdSetScissor",
    "vkCmdSetViewport",
    "vkCmdDrawIndexed",
    "vkCmdDraw",
    "vkCmdDispatch",
    "vkCmdCopyBuffer",
    "vkCmdCopyBufferToImage",
    "vkCmdCopyImageToBuffer",
    "vkCmdPipelineBarrier",
    "vkCmdBindDescriptorSets",
    "vkCmdBeginRenderPass",
    "vkBeginCommandBuffer",
    "vkCmdWriteTimestamp",
    "vkCmdResetQueryPool",
    "vkCmdDebugMarkerBeginEXT",
    "vkCmdDebugMarkerEndEXT",
    "vkCmdDebugMarkerInsertEXT",
    "vkEnumerateInstanceLayerProperties",
    "vkEnumerateInstanceExtensionProperties",
    "vkCreateInstance",
    "vkGetDeviceProcAddr",
    "vkGetPhysicalDeviceProperties",
    "vkEnumeratePhysicalDevices",
    "vkGetPhysicalDeviceFeatures",
    "vkGetPhysicalDeviceQueueFamilyProperties",
    "vkGetPhysicalDeviceSurfaceSupportKHR",
    "vkEnumerateDeviceExtensionProperties",
    "vkCreateDevice",
    "vkGetDeviceQueue",
    "vkGetPhysicalDeviceMemoryProperties",
    "vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
    "vkGetPhysicalDeviceSurfacePresentModesKHR",
    "vkGetPhysicalDeviceSurfaceFormatsKHR",
    "vkCreateSwapchainKHR",
    "vkDestroySwapchainKHR",
    "vkSetHdrMetadataEXT",
    "vkDeviceWaitIdle",
    "vkGetSwapchainImagesKHR",
    "vkDestroyPipelineCache",
    "vkDestroyDevice",
    "vkDestroySurfaceKHR",
    "vkDestroyInstance",
    "vkQueuePresentKHR",
    "vkAcquireNextImageKHR",
    "vkCreateBuffer",
    "vkGetBufferMemoryRequirements",
    "vkBindBufferMemory",
    "vkDestroyBuffer",
    "vkMapMemory",
    "vkUnmapMemory",
    "vkCreateImage",
    "vkCreateImageView",
    "vkDestroyImage",
    "vkDestroyImageView",
    "vkCreateBufferView",
    "vkDestroyBufferView",
    "vkCreateCommandPool",
    "vkDestroyCommandPool",
    "vkAllocateCommandBuffers",
    "vkAllocateMemory",
    "vkFreeMemory",
    "vkCreateDescriptorSetLayout",
    "vkDestroyDescriptorSetLayout",
    "vkCreatePipelineLayout",
    "vkDestroyPipelineLayout",
    "vkDestroyPipeline",
    "vkCreateDescriptorPool",
    "vkDestroyDescriptorPool",
    "vkAllocateDescriptorSets",
    "vkResetDescriptorPool",
    "vkUpdateDescriptorSets",
    "vkBindImageMemory",
    "vkGetImageMemoryRequirements",
    "vkCreateShaderModule",
    "vkDestroyShaderModule",
    "vkCreateGraphicsPipelines",
    "vkCreateComputePipelines",
    "vkCreateFence",
    "vkDestroyFence",
    "vkWaitForFences",
    "vkResetFences",
    "vkQueueSubmit",
    "vkCreateSemaphore",
    "vkDestroySemaphore",
    "vkCreateRenderPass",
    "vkDestroyRenderPass",
    "vkCreateFramebuffer",
    "vkDestroyFramebuffer",
    "vkCreateSampler",
    "vkDestroySampler",
    "vkFreeDescriptorSets",
    "vkCreateQueryPool",
    "vkDestroyQueryPool",
    "vkGetQueryPoolResults",
    "vkCreatePipelineCache",
    "vkGetPipelineCacheData",
};

static bool has_extension(const VkExtensionProperties* properties, uint32_t count, const char* name) {
    for (uint32_t index = 0; index < count; ++index) {
        if (strcmp(properties[index].extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

static void probe_compat_log(const char* message) {
    printf("compat %s\n", message);
}

static ProcAddressReport report_proc_addresses(
    VkInstance instance,
    VkDevice device,
    PFN_vkGetInstanceProcAddr get_instance_proc) {
    PFN_vkGetDeviceProcAddr get_device_proc =
        (PFN_vkGetDeviceProcAddr)get_instance_proc(instance, "vkGetDeviceProcAddr");
    size_t missing_instance = 0;
    size_t missing_device = 0;
    ProcAddressReport report = {0};
    for (size_t index = 0; index < sizeof(kEsoProcNames) / sizeof(kEsoProcNames[0]); ++index) {
        const char* name = kEsoProcNames[index];
        PFN_vkVoidFunction instance_result = get_instance_proc(instance, name);
        PFN_vkVoidFunction device_result = get_device_proc(device, name);
        if (strcmp(name, "vkSetHdrMetadataEXT") == 0) {
            report.hdr_gipa = instance_result != NULL;
            report.hdr_gdpa = device_result != NULL;
        }
        missing_instance += instance_result == NULL;
        missing_device += device_result == NULL;
        printf("proc %-44s GIPA=%s GDPA=%s\n", name, instance_result ? "yes" : "NULL",
               device_result ? "yes" : "NULL");
    }
    printf("proc summary candidates=%zu GIPA-null=%zu GDPA-null=%zu\n",
           sizeof(kEsoProcNames) / sizeof(kEsoProcNames[0]), missing_instance,
           missing_device);
    return report;
}

int main(int argc, char** argv) {
#ifdef TESO4M4_STATIC_MOLTENVK
    if (argc != 1) {
        fprintf(stderr, "usage: %s\n", argv[0]);
        return 2;
    }
    PFN_vkGetInstanceProcAddr get_proc = vkGetInstanceProcAddr;
#else
    if (argc != 2) {
        fprintf(stderr, "usage: %s libMoltenVK.dylib\n", argv[0]);
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }

    PFN_vkGetInstanceProcAddr get_proc = (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
#endif
    PFN_vkCreateInstance create_instance =
        (PFN_vkCreateInstance)get_proc(VK_NULL_HANDLE, "vkCreateInstance");
    PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extensions =
        (PFN_vkEnumerateInstanceExtensionProperties)get_proc(
            VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");

    uint32_t instance_extension_count = 0;
    enumerate_instance_extensions(NULL, &instance_extension_count, NULL);
    VkExtensionProperties* instance_extensions =
        calloc(instance_extension_count, sizeof(*instance_extensions));
    enumerate_instance_extensions(NULL, &instance_extension_count, instance_extensions);

    const char* requested_instance_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
    };
    for (size_t index = 0; index < 2; ++index) {
        printf("instance extension %-40s %s\n", requested_instance_extensions[index],
               has_extension(instance_extensions, instance_extension_count,
                             requested_instance_extensions[index])
                   ? "yes"
                   : "NO");
    }
    free(instance_extensions);

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "ESO 1.0 compatibility probe",
        .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = requested_instance_extensions,
    };
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = create_instance(&create_info, NULL, &instance);
    printf("vkCreateInstance (old extension set, no portability flag): %d\n", result);
    if (result != VK_SUCCESS) {
        return 1;
    }

    PFN_vkEnumeratePhysicalDevices enumerate_physical_devices =
        (PFN_vkEnumeratePhysicalDevices)get_proc(instance, "vkEnumeratePhysicalDevices");
    PFN_vkEnumerateDeviceExtensionProperties enumerate_device_extensions =
        (PFN_vkEnumerateDeviceExtensionProperties)get_proc(
            instance, "vkEnumerateDeviceExtensionProperties");
    PFN_vkGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties =
        (PFN_vkGetPhysicalDeviceQueueFamilyProperties)get_proc(
            instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    PFN_vkCreateDevice create_device =
        (PFN_vkCreateDevice)get_proc(instance, "vkCreateDevice");
    PFN_vkDestroyInstance destroy_instance =
        (PFN_vkDestroyInstance)get_proc(instance, "vkDestroyInstance");

    PFN_vkEnumerateDeviceExtensionProperties raw_enumerate_device_extensions =
        enumerate_device_extensions;
    PFN_vkCreateDevice raw_create_device = create_device;
    const bool use_hdr_filter = getenv("TESO4M4_PROBE_HDR_FILTER") != NULL;
    if (use_hdr_filter) {
        teso4m4_compat_reset();
        teso4m4_compat_set_logger(&probe_compat_log);
        teso4m4_compat_set_enumerate_device_extensions(
            raw_enumerate_device_extensions);
        teso4m4_compat_set_create_device(raw_create_device);
        enumerate_device_extensions =
            &teso4m4_enumerate_device_extension_properties;
        create_device = &teso4m4_create_device;
    }

    uint32_t physical_device_count = 0;
    bool device_created = false;
    bool filter_validation_succeeded = !use_hdr_filter;
    result = enumerate_physical_devices(instance, &physical_device_count, NULL);
    printf("vkEnumeratePhysicalDevices: %d, devices: %u\n", result, physical_device_count);
    if (result == VK_SUCCESS && physical_device_count > 0) {
        VkPhysicalDevice* devices = calloc(physical_device_count, sizeof(*devices));
        enumerate_physical_devices(instance, &physical_device_count, devices);

        uint32_t raw_device_extension_count = 0;
        raw_enumerate_device_extensions(
            devices[0], NULL, &raw_device_extension_count, NULL);
        VkExtensionProperties* raw_device_extensions =
            calloc(raw_device_extension_count, sizeof(*raw_device_extensions));
        raw_enumerate_device_extensions(
            devices[0], NULL, &raw_device_extension_count, raw_device_extensions);
        const bool raw_hdr_advertised = has_extension(
            raw_device_extensions, raw_device_extension_count,
            VK_EXT_HDR_METADATA_EXTENSION_NAME);
        printf("raw device extension %-40s %s\n",
               VK_EXT_HDR_METADATA_EXTENSION_NAME,
               raw_hdr_advertised ? "yes" : "NO");

        uint32_t device_extension_count = 0;
        enumerate_device_extensions(devices[0], NULL, &device_extension_count, NULL);
        VkExtensionProperties* device_extensions =
            calloc(device_extension_count, sizeof(*device_extensions));
        enumerate_device_extensions(
            devices[0], NULL, &device_extension_count, device_extensions);
        const bool visible_hdr_advertised = has_extension(
            device_extensions, device_extension_count,
            VK_EXT_HDR_METADATA_EXTENSION_NAME);
        printf("visible extension  %-40s %s\n",
               VK_EXT_HDR_METADATA_EXTENSION_NAME,
               visible_hdr_advertised ? "yes" : "NO");
        const char* requested_device_extensions[5] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
        };
        for (size_t index = 0; index < 4; ++index) {
            printf("device extension   %-40s %s\n", requested_device_extensions[index],
                   has_extension(device_extensions, device_extension_count,
                                 requested_device_extensions[index])
                       ? "yes"
                       : "NO");
        }
        uint32_t requested_device_extension_count = 4;
        bool enable_hdr_metadata =
            getenv("TESO4M4_PROBE_ENABLE_HDR_METADATA") != NULL &&
            has_extension(device_extensions, device_extension_count,
                          VK_EXT_HDR_METADATA_EXTENSION_NAME);
        if (enable_hdr_metadata) {
            requested_device_extensions[requested_device_extension_count++] =
                VK_EXT_HDR_METADATA_EXTENSION_NAME;
        }
        printf("probe enable      %-40s %s\n", VK_EXT_HDR_METADATA_EXTENSION_NAME,
               enable_hdr_metadata ? "yes" : "no");

        uint32_t queue_family_count = 0;
        get_queue_family_properties(devices[0], &queue_family_count, NULL);
        VkQueueFamilyProperties* queue_families =
            calloc(queue_family_count, sizeof(*queue_families));
        get_queue_family_properties(devices[0], &queue_family_count, queue_families);
        uint32_t queue_family_index = UINT32_MAX;
        for (uint32_t index = 0; index < queue_family_count; ++index) {
            if (queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queue_family_index = index;
                break;
            }
        }
        free(queue_families);

        if (queue_family_index != UINT32_MAX) {
            float queue_priority = 1.0f;
            VkDeviceQueueCreateInfo queue_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            };
            VkDeviceCreateInfo device_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queue_info,
                .enabledExtensionCount = requested_device_extension_count,
                .ppEnabledExtensionNames = requested_device_extensions,
            };
            VkDevice device = VK_NULL_HANDLE;
            result = create_device(devices[0], &device_info, NULL, &device);
            printf("vkCreateDevice (ESO-era extension set): %d\n", result);
            if (result == VK_SUCCESS) {
                device_created = true;
                ProcAddressReport report =
                    report_proc_addresses(instance, device, get_proc);
                printf(
                    "hdr negotiation raw-advertised=%s visible=%s enabled=%s GIPA=%s GDPA=%s\n",
                    raw_hdr_advertised ? "yes" : "no",
                    visible_hdr_advertised ? "yes" : "no",
                    enable_hdr_metadata ? "yes" : "no",
                    report.hdr_gipa ? "yes" : "NULL",
                    report.hdr_gdpa ? "yes" : "NULL");
                if (use_hdr_filter) {
                    filter_validation_succeeded =
                        raw_hdr_advertised && !visible_hdr_advertised &&
                        !enable_hdr_metadata && report.hdr_gipa &&
                        !report.hdr_gdpa;
                    printf("hdr filter validation: %s\n",
                           filter_validation_succeeded ? "PASS" : "FAIL");
                }
                PFN_vkDestroyDevice destroy_device =
                    (PFN_vkDestroyDevice)get_proc(instance, "vkDestroyDevice");
                destroy_device(device, NULL);
            }
        }
        free(raw_device_extensions);
        free(device_extensions);
        free(devices);
    }
    destroy_instance(instance, NULL);
    return physical_device_count > 0 && device_created &&
                   filter_validation_succeeded
               ? 0
               : 1;
}
