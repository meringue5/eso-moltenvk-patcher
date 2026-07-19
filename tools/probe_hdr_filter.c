#include "mvk_compat.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const VkExtensionProperties kRawExtensions[] = {
    {{"VK_KHR_swapchain"}, 70},
    {{VK_EXT_HDR_METADATA_EXTENSION_NAME}, VK_EXT_HDR_METADATA_SPEC_VERSION},
    {{"VK_KHR_maintenance1"}, 2},
    {{"VK_EXT_debug_marker"}, 4},
};

static const VkSurfaceFormatKHR kRawSurfaceFormats[] = {
    {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32,
     VK_COLOR_SPACE_HDR10_ST2084_EXT},
    {VK_FORMAT_A2R10G10B10_UNORM_PACK32,
     VK_COLOR_SPACE_HDR10_ST2084_EXT},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
};

static bool g_create_called;
static uint32_t g_create_extension_count;
static bool g_create_hdr_enabled;
static const char* g_last_layer_name;

static void test_log(const char* message) {
    printf("compat %s\n", message);
}

static VKAPI_ATTR VkResult VKAPI_CALL fake_enumerate_device_extensions(
    VkPhysicalDevice physical_device,
    const char* layer_name,
    uint32_t* property_count,
    VkExtensionProperties* properties) {
    (void)physical_device;
    g_last_layer_name = layer_name;
    const uint32_t available =
        (uint32_t)(sizeof(kRawExtensions) / sizeof(kRawExtensions[0]));
    if (!properties) {
        *property_count = available;
        return VK_SUCCESS;
    }
    const uint32_t capacity = *property_count;
    const uint32_t written = capacity < available ? capacity : available;
    memcpy(properties, kRawExtensions, written * sizeof(*properties));
    *property_count = written;
    return capacity < available ? VK_INCOMPLETE : VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL fake_create_device(
    VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDevice* device) {
    (void)physical_device;
    (void)allocator;
    g_create_called = true;
    g_create_extension_count = create_info->enabledExtensionCount;
    g_create_hdr_enabled = false;
    for (uint32_t index = 0; index < create_info->enabledExtensionCount; ++index) {
        if (strcmp(create_info->ppEnabledExtensionNames[index],
                   VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
            g_create_hdr_enabled = true;
        }
    }
    *device = (VkDevice)(uintptr_t)0x2;
    return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL fake_get_surface_formats(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    uint32_t* surface_format_count,
    VkSurfaceFormatKHR* surface_formats) {
    (void)physical_device;
    (void)surface;
    const uint32_t available =
        (uint32_t)(sizeof(kRawSurfaceFormats) / sizeof(kRawSurfaceFormats[0]));
    if (!surface_formats) {
        *surface_format_count = available;
        return VK_SUCCESS;
    }
    const uint32_t capacity = *surface_format_count;
    const uint32_t written = capacity < available ? capacity : available;
    memcpy(surface_formats, kRawSurfaceFormats,
           written * sizeof(*surface_formats));
    *surface_format_count = written;
    return capacity < available ? VK_INCOMPLETE : VK_SUCCESS;
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "HDR filter smoke failed: %s\n", message);
    }
    return condition;
}

int main(void) {
    teso4m4_compat_reset();
    teso4m4_compat_set_logger(&test_log);
    teso4m4_compat_set_enumerate_device_extensions(
        &fake_enumerate_device_extensions);
    teso4m4_compat_set_create_device(&fake_create_device);
    teso4m4_compat_set_get_surface_formats(&fake_get_surface_formats);

    VkPhysicalDevice physical_device = (VkPhysicalDevice)(uintptr_t)0x1;
    uint32_t count = 0;
    VkResult result = teso4m4_enumerate_device_extension_properties(
        physical_device, NULL, &count, NULL);
    if (!check(result == VK_SUCCESS && count == 3,
               "filtered count query should expose three extensions")) {
        return 1;
    }

    VkExtensionProperties exact[3] = {0};
    count = 3;
    result = teso4m4_enumerate_device_extension_properties(
        physical_device, NULL, &count, exact);
    if (!check(result == VK_SUCCESS && count == 3,
               "exact-capacity data query should succeed") ||
        !check(strcmp(exact[0].extensionName, "VK_KHR_swapchain") == 0 &&
                   strcmp(exact[1].extensionName, "VK_KHR_maintenance1") == 0 &&
                   strcmp(exact[2].extensionName, "VK_EXT_debug_marker") == 0,
               "HDR must be removed without reordering other extensions")) {
        return 1;
    }

    VkExtensionProperties partial[2] = {0};
    count = 2;
    result = teso4m4_enumerate_device_extension_properties(
        physical_device, NULL, &count, partial);
    if (!check(result == VK_INCOMPLETE && count == 2,
               "short data query should return VK_INCOMPLETE") ||
        !check(strcmp(partial[0].extensionName, "VK_KHR_swapchain") == 0 &&
                   strcmp(partial[1].extensionName, "VK_KHR_maintenance1") == 0,
               "short data query must contain only visible extensions")) {
        return 1;
    }

    count = 0;
    result = teso4m4_enumerate_device_extension_properties(
        physical_device, "VK_LAYER_FAKE", &count, NULL);
    if (!check(result == VK_SUCCESS && count == 4 && g_last_layer_name &&
                   strcmp(g_last_layer_name, "VK_LAYER_FAKE") == 0,
               "layer-specific enumeration must pass through unchanged")) {
        return 1;
    }

    VkSurfaceKHR surface = (VkSurfaceKHR)(uintptr_t)0x3;
    count = 0;
    result = teso4m4_get_physical_device_surface_formats(
        physical_device, surface, &count, NULL);
    if (!check(result == VK_SUCCESS && count == 3,
               "surface count should remove the exact ESO HDR pair")) {
        return 1;
    }

    VkSurfaceFormatKHR surface_exact[3] = {0};
    count = 3;
    result = teso4m4_get_physical_device_surface_formats(
        physical_device, surface, &count, surface_exact);
    if (!check(result == VK_SUCCESS && count == 3,
               "exact-capacity surface data query should succeed") ||
        !check(surface_exact[0].format == VK_FORMAT_B8G8R8A8_UNORM &&
                   surface_exact[1].format ==
                       VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                   surface_exact[1].colorSpace ==
                       VK_COLOR_SPACE_HDR10_ST2084_EXT &&
                   surface_exact[2].format == VK_FORMAT_R16G16B16A16_SFLOAT,
               "only the exact ESO HDR pair must be removed without reordering")) {
        return 1;
    }

    VkSurfaceFormatKHR surface_partial[2] = {0};
    count = 2;
    result = teso4m4_get_physical_device_surface_formats(
        physical_device, surface, &count, surface_partial);
    if (!check(result == VK_INCOMPLETE && count == 2,
               "short surface data query should return VK_INCOMPLETE") ||
        !check(surface_partial[0].format == VK_FORMAT_B8G8R8A8_UNORM &&
                   surface_partial[1].format ==
                       VK_FORMAT_A2R10G10B10_UNORM_PACK32,
               "short surface query must contain only visible formats")) {
        return 1;
    }

    const char* enabled_extensions[] = {
        "VK_KHR_swapchain",
        "VK_EXT_debug_marker",
    };
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = enabled_extensions,
    };
    VkDevice device = VK_NULL_HANDLE;
    result = teso4m4_create_device(
        physical_device, &create_info, NULL, &device);
    if (!check(result == VK_SUCCESS && g_create_called &&
                   g_create_extension_count == 2 && device != VK_NULL_HANDLE,
               "create wrapper must log and forward the unchanged request")) {
        return 1;
    }

    const char* hdr_extension[] = {VK_EXT_HDR_METADATA_EXTENSION_NAME};
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = hdr_extension;
    result = teso4m4_create_device(
        physical_device, &create_info, NULL, &device);
    if (!check(result == VK_SUCCESS && g_create_extension_count == 1 &&
                   g_create_hdr_enabled,
               "explicit HDR enablement must be logged and forwarded unchanged")) {
        return 1;
    }

    puts("HDR filter smoke: yes");
    return 0;
}
