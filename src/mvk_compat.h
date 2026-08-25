#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#define TESO4M4_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_HIDDEN
#endif

typedef void (*Teso4m4CompatLogFunction)(const char* message);

TESO4M4_HIDDEN void teso4m4_compat_reset(void);
TESO4M4_HIDDEN void teso4m4_compat_set_logger(Teso4m4CompatLogFunction logger);
TESO4M4_HIDDEN void teso4m4_compat_set_enumerate_device_extensions(
    PFN_vkEnumerateDeviceExtensionProperties next_function);
TESO4M4_HIDDEN void teso4m4_compat_set_create_device(
    PFN_vkCreateDevice next_function);
TESO4M4_HIDDEN void teso4m4_compat_set_device_readiness(
    PFN_vkGetDeviceProcAddr get_device_proc_addr,
    PFN_vkDestroyDevice destroy_device,
    bool required);
TESO4M4_HIDDEN void teso4m4_compat_set_get_surface_formats(
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR next_function);

TESO4M4_HIDDEN VKAPI_ATTR VkResult VKAPI_CALL
teso4m4_enumerate_device_extension_properties(
    VkPhysicalDevice physical_device,
    const char* layer_name,
    uint32_t* property_count,
    VkExtensionProperties* properties);

TESO4M4_HIDDEN VKAPI_ATTR VkResult VKAPI_CALL teso4m4_create_device(
    VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDevice* device);

TESO4M4_HIDDEN VKAPI_ATTR VkResult VKAPI_CALL
teso4m4_get_physical_device_surface_formats(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    uint32_t* surface_format_count,
    VkSurfaceFormatKHR* surface_formats);
