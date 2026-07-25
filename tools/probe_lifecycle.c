#include "mvk_lifecycle.h"

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define HANDLE(type, value) ((type)(uintptr_t)(value))

static char g_log[32768];
static size_t g_log_length;
static uintptr_t g_next_handle = 0x100;
static VkImage g_swapchain_images[] = {
    HANDLE(VkImage, 0x201),
    HANDLE(VkImage, 0x202),
};

static void test_log(const char* message) {
    const int written = snprintf(
        g_log + g_log_length, sizeof(g_log) - g_log_length, "%s\n", message);
    if (written > 0 && (size_t)written < sizeof(g_log) - g_log_length) {
        g_log_length += (size_t)written;
    }
}

static VkResult VKAPI_CALL fake_device_wait_idle(VkDevice device) {
    (void)device;
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_swapchain(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* create_info,
    const VkAllocationCallbacks* allocator,
    VkSwapchainKHR* swapchain) {
    (void)device;
    (void)create_info;
    (void)allocator;
    *swapchain = HANDLE(VkSwapchainKHR, ++g_next_handle);
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
    uint32_t* count,
    VkImage* images) {
    (void)device;
    (void)swapchain;
    if (!images) {
        *count = 2;
        return VK_SUCCESS;
    }
    const uint32_t written = *count < 2 ? *count : 2;
    memcpy(images, g_swapchain_images, written * sizeof(*images));
    *count = written;
    return written < 2 ? VK_INCOMPLETE : VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_image_view(
    VkDevice device,
    const VkImageViewCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkImageView* image_view) {
    (void)device;
    (void)create_info;
    (void)allocator;
    *image_view = HANDLE(VkImageView, ++g_next_handle);
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_destroy_image_view(
    VkDevice device,
    VkImageView image_view,
    const VkAllocationCallbacks* allocator) {
    (void)device;
    (void)image_view;
    (void)allocator;
}

static VkResult VKAPI_CALL fake_create_render_pass(
    VkDevice device,
    const VkRenderPassCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkRenderPass* render_pass) {
    (void)device;
    (void)create_info;
    (void)allocator;
    *render_pass = HANDLE(VkRenderPass, ++g_next_handle);
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_destroy_render_pass(
    VkDevice device,
    VkRenderPass render_pass,
    const VkAllocationCallbacks* allocator) {
    (void)device;
    (void)render_pass;
    (void)allocator;
}

static VkResult VKAPI_CALL fake_create_framebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkFramebuffer* framebuffer) {
    (void)device;
    (void)create_info;
    (void)allocator;
    *framebuffer = HANDLE(VkFramebuffer, ++g_next_handle);
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_destroy_framebuffer(
    VkDevice device,
    VkFramebuffer framebuffer,
    const VkAllocationCallbacks* allocator) {
    (void)device;
    (void)framebuffer;
    (void)allocator;
}

static VkResult VKAPI_CALL fake_acquire_next_image(
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
    *image_index = 1;
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_queue_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    (void)queue;
    if (present_info->pResults) {
        for (uint32_t index = 0; index < present_info->swapchainCount; ++index) {
            present_info->pResults[index] = VK_SUCCESS;
        }
    }
    return VK_SUCCESS;
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Lifecycle probe failed: %s\n%s", message, g_log);
    }
    return condition;
}

static uint64_t monotonic_nanoseconds(void) {
    struct timespec value = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0;
    }
    return (uint64_t)value.tv_sec * 1000000000ULL +
           (uint64_t)value.tv_nsec;
}

int main(void) {
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_enabled(false);
    const struct {
        const char* name;
        PFN_vkVoidFunction function;
    } performance_entries[] = {
        {"vkDeviceWaitIdle", (PFN_vkVoidFunction)&fake_device_wait_idle},
        {"vkCreateSwapchainKHR", (PFN_vkVoidFunction)&fake_create_swapchain},
        {"vkDestroySwapchainKHR", (PFN_vkVoidFunction)&fake_destroy_swapchain},
        {"vkGetSwapchainImagesKHR", (PFN_vkVoidFunction)&fake_get_swapchain_images},
        {"vkCreateImageView", (PFN_vkVoidFunction)&fake_create_image_view},
        {"vkDestroyImageView", (PFN_vkVoidFunction)&fake_destroy_image_view},
        {"vkCreateRenderPass", (PFN_vkVoidFunction)&fake_create_render_pass},
        {"vkDestroyRenderPass", (PFN_vkVoidFunction)&fake_destroy_render_pass},
        {"vkCreateFramebuffer", (PFN_vkVoidFunction)&fake_create_framebuffer},
        {"vkDestroyFramebuffer", (PFN_vkVoidFunction)&fake_destroy_framebuffer},
        {"vkAcquireNextImageKHR", (PFN_vkVoidFunction)&fake_acquire_next_image},
        {"vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_queue_present},
    };
    for (size_t index = 0;
         index < sizeof(performance_entries) / sizeof(performance_entries[0]);
         ++index) {
        if (!check(
                teso4m4_lifecycle_intercept(
                    performance_entries[index].name,
                    performance_entries[index].function) ==
                    performance_entries[index].function,
                "disabled lifecycle mode must return the original function")) {
            return 1;
        }
    }

    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&test_log);

    PFN_vkDeviceWaitIdle wait_idle = (PFN_vkDeviceWaitIdle)
        teso4m4_lifecycle_intercept(
            "vkDeviceWaitIdle", (PFN_vkVoidFunction)&fake_device_wait_idle);
    PFN_vkCreateSwapchainKHR create_swapchain = (PFN_vkCreateSwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)&fake_create_swapchain);
    PFN_vkDestroySwapchainKHR destroy_swapchain = (PFN_vkDestroySwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkDestroySwapchainKHR",
            (PFN_vkVoidFunction)&fake_destroy_swapchain);
    PFN_vkGetSwapchainImagesKHR get_images = (PFN_vkGetSwapchainImagesKHR)
        teso4m4_lifecycle_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)&fake_get_swapchain_images);
    PFN_vkCreateImageView create_image_view = (PFN_vkCreateImageView)
        teso4m4_lifecycle_intercept(
            "vkCreateImageView", (PFN_vkVoidFunction)&fake_create_image_view);
    PFN_vkDestroyImageView destroy_image_view = (PFN_vkDestroyImageView)
        teso4m4_lifecycle_intercept(
            "vkDestroyImageView",
            (PFN_vkVoidFunction)&fake_destroy_image_view);
    PFN_vkCreateRenderPass create_render_pass = (PFN_vkCreateRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCreateRenderPass",
            (PFN_vkVoidFunction)&fake_create_render_pass);
    PFN_vkDestroyRenderPass destroy_render_pass = (PFN_vkDestroyRenderPass)
        teso4m4_lifecycle_intercept(
            "vkDestroyRenderPass",
            (PFN_vkVoidFunction)&fake_destroy_render_pass);
    PFN_vkCreateFramebuffer create_framebuffer = (PFN_vkCreateFramebuffer)
        teso4m4_lifecycle_intercept(
            "vkCreateFramebuffer",
            (PFN_vkVoidFunction)&fake_create_framebuffer);
    PFN_vkDestroyFramebuffer destroy_framebuffer = (PFN_vkDestroyFramebuffer)
        teso4m4_lifecycle_intercept(
            "vkDestroyFramebuffer",
            (PFN_vkVoidFunction)&fake_destroy_framebuffer);
    PFN_vkAcquireNextImageKHR acquire = (PFN_vkAcquireNextImageKHR)
        teso4m4_lifecycle_intercept(
            "vkAcquireNextImageKHR",
            (PFN_vkVoidFunction)&fake_acquire_next_image);
    PFN_vkQueuePresentKHR present = (PFN_vkQueuePresentKHR)
        teso4m4_lifecycle_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_queue_present);

    VkDevice device = HANDLE(VkDevice, 0x1);
    VkQueue queue = HANDLE(VkQueue, 0x2);
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {2048, 1280},
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR first = VK_NULL_HANDLE;
    if (!check(wait_idle(device) == VK_SUCCESS, "device wait should pass") ||
        !check(create_swapchain(device, &create_info, NULL, &first) ==
                   VK_SUCCESS,
               "first swapchain create should pass")) {
        return 1;
    }

    uint32_t image_count = 2;
    VkImage images[2] = {0};
    if (!check(get_images(device, first, &image_count, images) == VK_SUCCESS,
               "swapchain images should pass")) {
        return 1;
    }
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = images[0],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
    };
    VkImageView view = VK_NULL_HANDLE;
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    };
    VkRenderPass render_pass = VK_NULL_HANDLE;
    if (!check(create_image_view(device, &view_info, NULL, &view) == VK_SUCCESS,
               "image view create should pass") ||
        !check(create_render_pass(
                   device, &render_pass_info, NULL, &render_pass) == VK_SUCCESS,
               "render pass create should pass")) {
        return 1;
    }
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &view,
        .width = 2048,
        .height = 1280,
        .layers = 1,
    };
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (!check(create_framebuffer(
                   device, &framebuffer_info, NULL, &framebuffer) == VK_SUCCESS,
               "framebuffer create should pass")) {
        return 1;
    }

    uint32_t image_index = 0;
    VkResult item_result = VK_SUCCESS;
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &first,
        .pImageIndices = &image_index,
        .pResults = &item_result,
    };
    if (!check(acquire(
                   device, first, 0, VK_NULL_HANDLE, VK_NULL_HANDLE,
                   &image_index) == VK_SUCCESS,
               "acquire should pass") ||
        !check(present(queue, &present_info) == VK_SUCCESS,
               "present should pass")) {
        return 1;
    }

    enum { kBenchmarkIterations = 100000 };
    const uint64_t benchmark_start = monotonic_nanoseconds();
    for (unsigned int iteration = 0;
         iteration < kBenchmarkIterations;
         ++iteration) {
        if (acquire(
                device, first, 0, VK_NULL_HANDLE, VK_NULL_HANDLE,
                &image_index) != VK_SUCCESS ||
            present(queue, &present_info) != VK_SUCCESS) {
            fprintf(stderr, "Lifecycle probe benchmark forwarding failed\n");
            return 1;
        }
    }
    const uint64_t benchmark_end = monotonic_nanoseconds();
    if (!check(
            benchmark_start != 0 && benchmark_end > benchmark_start,
            "benchmark clock must advance")) {
        return 1;
    }
    const uint64_t pair_nanoseconds =
        (benchmark_end - benchmark_start) / kBenchmarkIterations;
    if (!check(
            pair_nanoseconds < 50000,
            "steady acquire/present wrapper pair must remain below 50 us")) {
        return 1;
    }

    create_info.oldSwapchain = first;
    create_info.imageExtent = (VkExtent2D){1920, 1200};
    VkSwapchainKHR second = VK_NULL_HANDLE;
    if (!check(create_swapchain(device, &create_info, NULL, &second) ==
                   VK_SUCCESS,
               "replacement swapchain create should pass")) {
        return 1;
    }

    destroy_framebuffer(device, framebuffer, NULL);
    destroy_image_view(device, view, NULL);
    destroy_render_pass(device, render_pass, NULL);
    destroy_swapchain(device, first, NULL);

    if (!check(strstr(g_log, "generation=1") != NULL,
               "first generation must be logged") ||
        !check(strstr(g_log, "old_generation=1") != NULL &&
                   strstr(g_log, "extent=1920x1200") != NULL &&
                   strstr(g_log, "generation=2") != NULL,
               "replacement generation must link to the first") ||
        !check(strstr(g_log, "SWAPCHAIN_IMAGE_VIEW_CREATE") != NULL &&
                   strstr(g_log, "SWAPCHAIN_FRAMEBUFFER_CREATE") != NULL &&
                   strstr(g_log, "first_generation=1") != NULL,
               "dependent resource lifecycle must be logged") ||
        !check(strstr(g_log, "SWAPCHAIN_ACQUIRE") != NULL &&
                   strstr(g_log, "SWAPCHAIN_PRESENT") != NULL,
               "first acquire and presentation must be logged") ||
        !check(strstr(g_log, "LIFECYCLE_ERROR") == NULL,
               "probe must not report lifecycle errors")) {
        return 1;
    }

    printf(
        "Lifecycle trace smoke: yes steady_pair_ns=%" PRIu64 "\n",
        pair_nanoseconds);
    return 0;
}
