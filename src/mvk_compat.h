#pragma once

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
