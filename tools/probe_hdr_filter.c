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
