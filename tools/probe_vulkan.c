#define VK_USE_PLATFORM_MACOS_MVK 1
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

static bool has_extension(const VkExtensionProperties* properties, uint32_t count, const char* name) {
    for (uint32_t index = 0; index < count; ++index) {
        if (strcmp(properties[index].extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
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

    uint32_t physical_device_count = 0;
    result = enumerate_physical_devices(instance, &physical_device_count, NULL);
    printf("vkEnumeratePhysicalDevices: %d, devices: %u\n", result, physical_device_count);
    if (result == VK_SUCCESS && physical_device_count > 0) {
        VkPhysicalDevice* devices = calloc(physical_device_count, sizeof(*devices));
        enumerate_physical_devices(instance, &physical_device_count, devices);

        uint32_t device_extension_count = 0;
        enumerate_device_extensions(devices[0], NULL, &device_extension_count, NULL);
        VkExtensionProperties* device_extensions =
            calloc(device_extension_count, sizeof(*device_extensions));
        enumerate_device_extensions(
            devices[0], NULL, &device_extension_count, device_extensions);
        const char* requested_device_extensions[] = {
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
                .enabledExtensionCount = 4,
                .ppEnabledExtensionNames = requested_device_extensions,
            };
            VkDevice device = VK_NULL_HANDLE;
            result = create_device(devices[0], &device_info, NULL, &device);
            printf("vkCreateDevice (ESO-era extension set): %d\n", result);
            if (result == VK_SUCCESS) {
                PFN_vkDestroyDevice destroy_device =
                    (PFN_vkDestroyDevice)get_proc(instance, "vkDestroyDevice");
                destroy_device(device, NULL);
            }
        }
        free(device_extensions);
        free(devices);
    }
    destroy_instance(instance, NULL);
    return physical_device_count > 0 ? 0 : 1;
}
