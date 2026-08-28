#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mvk_swapchain_experiment.h"

#define HANDLE(type, value) ((type)(uintptr_t)(value))

static uint32_t g_max_images;
static uint32_t g_effective_images;
static char g_log[8192];
static VkSwapchainKHR g_swapchain;
static PFN_vkAcquireNextImageKHR g_wrapped_acquire;
static PFN_vkQueuePresentKHR g_wrapped_present;
static volatile uint64_t g_fake_present_calls;

static uint64_t monotonic_ns(void) {
    struct timespec value = {0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &value) != 0) {
        return 0;
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
           (uint64_t)value.tv_nsec;
}

static void record_log(const char* message) {
    strlcat(g_log, message, sizeof(g_log));
    strlcat(g_log, "\n", sizeof(g_log));
}

static VkResult VKAPI_CALL fake_get_surface_capabilities(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* capabilities) {
    (void)physical_device;
    (void)surface;
    *capabilities = (VkSurfaceCapabilitiesKHR){
        .minImageCount = 2,
        .maxImageCount = g_max_images,
        .currentExtent = {1920, 1200},
    };
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_swapchain(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* create_info,
    const VkAllocationCallbacks* allocator,
    VkSwapchainKHR* swapchain) {
    (void)device;
    (void)allocator;
    g_effective_images = create_info->minImageCount;
    *swapchain = HANDLE(VkSwapchainKHR, 0x44);
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_destroy_swapchain(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* allocator) {
    (void)device;
    (void)swapchain;
    (void)allocator;
}

static VkResult VKAPI_CALL fake_get_swapchain_images(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* image_count,
    VkImage* images) {
    (void)device;
    (void)swapchain;
    if (!images) {
        *image_count = g_effective_images;
        return VK_SUCCESS;
    }
    for (uint32_t index = 0; index < *image_count; ++index) {
        images[index] = HANDLE(VkImage, 0x100 + index);
    }
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_acquire(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* image_index) {
    (void)device;
    (void)swapchain;
    (void)timeout;
    (void)semaphore;
    (void)fence;
    *image_index = 0;
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    (void)queue;
    (void)present_info;
    ++g_fake_present_calls;
    return VK_SUCCESS;
}

static bool run_case(
    Teso4m4SwapchainExperimentMode mode,
    uint32_t max_images,
    uint32_t expected_images) {
    teso4m4_swapchain_experiment_reset();
    teso4m4_swapchain_experiment_configure(mode, &record_log);
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR get_capabilities =
        (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
            teso4m4_swapchain_experiment_intercept(
                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
                (PFN_vkVoidFunction)&fake_get_surface_capabilities);
    PFN_vkCreateSwapchainKHR create_swapchain =
        (PFN_vkCreateSwapchainKHR)teso4m4_swapchain_experiment_intercept(
            "vkCreateSwapchainKHR", (PFN_vkVoidFunction)&fake_create_swapchain);
    (void)teso4m4_swapchain_experiment_intercept(
        "vkDestroySwapchainKHR", (PFN_vkVoidFunction)&fake_destroy_swapchain);
    PFN_vkGetSwapchainImagesKHR get_images =
        (PFN_vkGetSwapchainImagesKHR)teso4m4_swapchain_experiment_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)&fake_get_swapchain_images);
    g_wrapped_acquire = (PFN_vkAcquireNextImageKHR)
        teso4m4_swapchain_experiment_intercept(
            "vkAcquireNextImageKHR", (PFN_vkVoidFunction)&fake_acquire);
    g_wrapped_present = (PFN_vkQueuePresentKHR)
        teso4m4_swapchain_experiment_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_present);

    g_max_images = max_images;
    VkSurfaceCapabilitiesKHR capabilities = {0};
    const VkSurfaceKHR surface = HANDLE(VkSurfaceKHR, 0x22);
    if (get_capabilities(
            HANDLE(VkPhysicalDevice, 0x11), surface, &capabilities) !=
        VK_SUCCESS) {
        return false;
    }
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageExtent = {1920, 1200},
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (create_swapchain(
            HANDLE(VkDevice, 0x33), &create_info, NULL, &swapchain) !=
        VK_SUCCESS) {
        return false;
    }
    g_swapchain = swapchain;
    uint32_t count = 0;
    if (get_images(HANDLE(VkDevice, 0x33), swapchain, &count, NULL) !=
        VK_SUCCESS) {
        return false;
    }
    return g_effective_images == expected_images && count == expected_images;
}

static uint64_t benchmark_pairs(bool wrapped, uint32_t iterations) {
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &g_swapchain,
    };
    volatile uint32_t checksum = 0;
    const uint64_t start = monotonic_ns();
    for (uint32_t index = 0; index < iterations; ++index) {
        uint32_t image_index = 0;
        if (wrapped) {
            (void)g_wrapped_acquire(
                HANDLE(VkDevice, 0x33), g_swapchain, UINT64_MAX,
                VK_NULL_HANDLE, VK_NULL_HANDLE, &image_index);
            (void)g_wrapped_present(HANDLE(VkQueue, 0x55), &present_info);
        } else {
            (void)fake_acquire(
                HANDLE(VkDevice, 0x33), g_swapchain, UINT64_MAX,
                VK_NULL_HANDLE, VK_NULL_HANDLE, &image_index);
            (void)fake_present(HANDLE(VkQueue, 0x55), &present_info);
        }
        checksum += image_index;
    }
    const uint64_t end = monotonic_ns();
    return checksum == UINT32_MAX ? 0 : (end - start) / iterations;
}

int main(void) {
    const bool promoted = run_case(
        TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER, 3, 3);
    const bool bounded = run_case(
        TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER, 2, 2);
    const bool control = run_case(
        TESO4M4_SWAPCHAIN_EXPERIMENT_CONTROL, 3, 2);
    const uint32_t benchmark_iterations = 100000;
    const uint64_t raw_ns = benchmark_pairs(false, benchmark_iterations);
    const uint64_t wrapped_ns = benchmark_pairs(true, benchmark_iterations);
    teso4m4_swapchain_experiment_log_summary();
    const bool summary = strstr(
        g_log, "SWAPCHAIN_EXPERIMENT_SUMMARY: mode=control") != NULL;
    if (!promoted || !bounded || !control || !summary || wrapped_ns == 0 ||
        wrapped_ns > UINT64_C(100000)) {
        fprintf(
            stderr,
            "Swapchain experiment smoke: FAIL promoted=%d bounded=%d "
            "control=%d summary=%d raw_ns=%llu wrapped_ns=%llu\n%s",
            promoted, bounded, control, summary,
            (unsigned long long)raw_ns, (unsigned long long)wrapped_ns, g_log);
        return 1;
    }
    printf(
        "Swapchain experiment smoke: PASS promoted=3 unsupported=2 "
        "control=2 raw_pair_ns=%llu measured_pair_ns=%llu "
        "cpu_budget_at_60fps=%.6f%%\n",
        (unsigned long long)raw_ns, (unsigned long long)wrapped_ns,
        (double)wrapped_ns * 60.0 / 10000000.0);
    return 0;
}
