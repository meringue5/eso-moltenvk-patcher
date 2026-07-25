#include "mvk_compat.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(
    sizeof(VkPhysicalDeviceFeatures) % sizeof(VkBool32) == 0,
    "VkPhysicalDeviceFeatures must contain complete VkBool32 fields");

static bool g_create_called;

static void test_log(const char* message) {
    printf("compat %s\n", message);
}

static VKAPI_ATTR void VKAPI_CALL fake_get_physical_device_features(
    VkPhysicalDevice physical_device,
    VkPhysicalDeviceFeatures* features) {
    (void)physical_device;
    VkBool32 values[
        sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32)];
    for (size_t index = 0;
         index < sizeof(values) / sizeof(values[0]); ++index) {
        values[index] = VK_TRUE;
    }
    memcpy(features, values, sizeof(values));
}

static VKAPI_ATTR VkResult VKAPI_CALL fake_create_device(
    VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDevice* device) {
    (void)physical_device;
    (void)create_info;
    (void)allocator;
    g_create_called = true;
    *device = (VkDevice)(uintptr_t)0x2;
    return VK_SUCCESS;
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Legacy feature profile smoke failed: %s\n", message);
    }
    return condition;
}

static uint32_t count_enabled(
    const VkPhysicalDeviceFeatures* features) {
    VkBool32 values[
        sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32)];
    memcpy(values, features, sizeof(values));
    uint32_t count = 0;
    for (size_t index = 0;
         index < sizeof(values) / sizeof(values[0]); ++index) {
        count += values[index] == VK_TRUE;
    }
    return count;
}

int main(void) {
    teso4m4_compat_reset();
    teso4m4_compat_set_logger(&test_log);
    teso4m4_compat_set_get_physical_device_features(
        &fake_get_physical_device_features);
    teso4m4_compat_set_create_device(&fake_create_device);
    teso4m4_compat_set_legacy_feature_profile_enabled(true);

    const VkPhysicalDevice physical_device =
        (VkPhysicalDevice)(uintptr_t)0x1;
    VkPhysicalDeviceFeatures features = {0};
    teso4m4_get_physical_device_features(
        physical_device, &features);

    if (!check(
            count_enabled(&features) == 37,
            "exactly 18 of 55 raw enabled features must be masked") ||
        !check(
            features.robustBufferAccess == VK_FALSE &&
                features.fullDrawIndexUint32 == VK_FALSE &&
                features.tessellationShader == VK_FALSE &&
                features.sampleRateShading == VK_FALSE &&
                features.drawIndirectFirstInstance == VK_FALSE &&
                features.multiViewport == VK_FALSE &&
                features.textureCompressionETC2 == VK_FALSE &&
                features.textureCompressionASTC_LDR == VK_FALSE &&
                features.shaderTessellationAndGeometryPointSize == VK_FALSE &&
                features.shaderStorageImageReadWithoutFormat == VK_FALSE &&
                features.shaderStorageImageWriteWithoutFormat == VK_FALSE &&
                features.shaderUniformBufferArrayDynamicIndexing == VK_FALSE &&
                features.shaderSampledImageArrayDynamicIndexing == VK_FALSE &&
                features.shaderStorageBufferArrayDynamicIndexing == VK_FALSE &&
                features.shaderStorageImageArrayDynamicIndexing == VK_FALSE &&
                features.shaderInt64 == VK_FALSE &&
                features.shaderResourceMinLod == VK_FALSE &&
                features.inheritedQueries == VK_FALSE,
            "all 18 M4 features absent from embedded 1.0.18 must be false") ||
        !check(
            features.independentBlend == VK_TRUE &&
                features.dualSrcBlend == VK_TRUE &&
                features.depthClamp == VK_TRUE &&
                features.depthBiasClamp == VK_TRUE &&
                features.fillModeNonSolid == VK_TRUE &&
                features.samplerAnisotropy == VK_TRUE &&
                features.textureCompressionBC == VK_TRUE &&
                features.fragmentStoresAndAtomics == VK_TRUE &&
                features.shaderImageGatherExtended == VK_TRUE &&
                features.shaderClipDistance == VK_TRUE,
            "ESO's ten static device-suitability features must remain true")) {
        return 1;
    }

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pEnabledFeatures = &features,
    };
    VkDevice device = VK_NULL_HANDLE;
    VkResult result = teso4m4_create_device(
        physical_device, &create_info, NULL, &device);
    if (!check(
            result == VK_SUCCESS && g_create_called &&
                device != VK_NULL_HANDLE,
            "the exact masked profile must reach the real device creator")) {
        return 1;
    }

    g_create_called = false;
    features.robustBufferAccess = VK_TRUE;
    result = teso4m4_create_device(
        physical_device, &create_info, NULL, &device);
    if (!check(
            result == VK_ERROR_FEATURE_NOT_PRESENT && !g_create_called,
            "a prohibited feature must fail closed before device creation")) {
        return 1;
    }

    puts("Legacy feature profile smoke: PASS");
    return 0;
}
