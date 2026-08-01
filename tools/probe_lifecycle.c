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
static uint32_t g_present_pixel_sample_count;
static uint64_t g_present_pixel_generations[32];
static uint32_t g_present_pixel_ordinals[32];
static bool g_present_pixel_arguments_valid;

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

static VkResult VKAPI_CALL fake_queue_submit(
    VkQueue queue,
    uint32_t submit_count,
    const VkSubmitInfo* submits,
    VkFence fence) {
    (void)queue;
    (void)submit_count;
    (void)submits;
    (void)fence;
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_cmd_begin_render_pass(
    VkCommandBuffer command_buffer,
    const VkRenderPassBeginInfo* begin_info,
    VkSubpassContents contents) {
    (void)command_buffer;
    (void)begin_info;
    (void)contents;
}

static void VKAPI_CALL fake_cmd_end_render_pass(
    VkCommandBuffer command_buffer) {
    (void)command_buffer;
}

static void VKAPI_CALL fake_cmd_clear_attachments(
    VkCommandBuffer command_buffer,
    uint32_t attachment_count,
    const VkClearAttachment* attachments,
    uint32_t rect_count,
    const VkClearRect* rects) {
    (void)command_buffer;
    (void)attachment_count;
    (void)attachments;
    (void)rect_count;
    (void)rects;
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

static bool fake_present_pixel_sampler(
    VkQueue queue,
    VkImage image,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint64_t generation,
    uint32_t ordinal,
    uint32_t image_index) {
    const bool extent_valid =
        (generation == 1 && width == 3420 && height == 2148) ||
        (generation == 2 && width == 3420 && height == 2146);
    g_present_pixel_arguments_valid &=
        queue == HANDLE(VkQueue, 0x82) &&
        image == g_swapchain_images[0] &&
        format == VK_FORMAT_B8G8R8A8_UNORM && extent_valid &&
        image_index == 0;
    if (g_present_pixel_sample_count < 32) {
        g_present_pixel_generations[g_present_pixel_sample_count] = generation;
        g_present_pixel_ordinals[g_present_pixel_sample_count] = ordinal;
    }
    ++g_present_pixel_sample_count;
    return true;
}

static bool run_present_pixel_schedule_case(void) {
    g_log_length = 0;
    g_log[0] = '\0';
    g_present_pixel_sample_count = 0;
    memset(g_present_pixel_generations, 0, sizeof(g_present_pixel_generations));
    memset(g_present_pixel_ordinals, 0, sizeof(g_present_pixel_ordinals));
    g_present_pixel_arguments_valid = true;
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&test_log);
    teso4m4_lifecycle_set_present_pixel_sampler(
        &fake_present_pixel_sampler);
    teso4m4_lifecycle_set_startup_color_audit(true);
    teso4m4_lifecycle_set_startup_present_pixel_audit(true);

    PFN_vkCreateSwapchainKHR create_swapchain = (PFN_vkCreateSwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)&fake_create_swapchain);
    PFN_vkGetSwapchainImagesKHR get_images = (PFN_vkGetSwapchainImagesKHR)
        teso4m4_lifecycle_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)&fake_get_swapchain_images);
    PFN_vkQueueSubmit submit = (PFN_vkQueueSubmit)
        teso4m4_lifecycle_intercept(
            "vkQueueSubmit", (PFN_vkVoidFunction)&fake_queue_submit);
    PFN_vkQueuePresentKHR present = (PFN_vkQueuePresentKHR)
        teso4m4_lifecycle_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_queue_present);

    const VkDevice device = HANDLE(VkDevice, 0x81);
    const VkQueue queue = HANDLE(VkQueue, 0x82);
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {3420, 2148},
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (create_swapchain(device, &create_info, NULL, &swapchain) != VK_SUCCESS) {
        return check(false, "pixel schedule first swapchain creation failed");
    }
    uint32_t image_count = 2;
    VkImage images[2] = {0};
    if (get_images(device, swapchain, &image_count, images) != VK_SUCCESS) {
        return check(false, "pixel schedule first image query failed");
    }
    uint32_t image_index = 0;
    VkSemaphore signal = HANDLE(VkSemaphore, 0x900);
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signal,
    };
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signal,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };
    if (submit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS ||
        present(queue, &present_info) != VK_SUCCESS) {
        return check(false, "pixel schedule first present failed");
    }

    create_info.oldSwapchain = swapchain;
    create_info.imageExtent.height = 2146;
    if (create_swapchain(device, &create_info, NULL, &swapchain) != VK_SUCCESS) {
        return check(false, "pixel schedule replacement creation failed");
    }
    image_count = 2;
    memset(images, 0, sizeof(images));
    if (get_images(device, swapchain, &image_count, images) != VK_SUCCESS) {
        return check(false, "pixel schedule replacement image query failed");
    }
    present_info.pSwapchains = &swapchain;
    for (uint32_t ordinal = 1; ordinal <= 180; ++ordinal) {
        signal = HANDLE(VkSemaphore, 0x900 + ordinal);
        if (submit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS ||
            present(queue, &present_info) != VK_SUCCESS) {
            return check(false, "pixel schedule replacement present failed");
        }
    }

    const uint32_t expected_ordinals[] = {
        1, 10, 20, 30, 40, 50, 60, 70, 80, 90,
        100, 110, 120, 130, 140, 150, 160, 170, 180,
    };
    bool schedule_valid =
        g_present_pixel_sample_count == 20 &&
        g_present_pixel_generations[0] == 1 &&
        g_present_pixel_ordinals[0] == 1;
    for (size_t index = 0;
         index < sizeof(expected_ordinals) / sizeof(expected_ordinals[0]);
         ++index) {
        schedule_valid &=
            g_present_pixel_generations[index + 1] == 2 &&
            g_present_pixel_ordinals[index + 1] == expected_ordinals[index];
    }
    return check(
        schedule_valid && g_present_pixel_arguments_valid &&
            strstr(g_log, "STARTUP_PRESENT_PIXEL_SKIP:") == NULL &&
            strstr(
                g_log,
                "STARTUP_COLOR_AUDIT_FINISH: "
                "reason=generation-2-present-limit generation=2 ordinal=180") != NULL,
        "present pixel audit must sample the exact complete schedule");
}

static bool run_consumed_semaphore_case(void) {
    g_log_length = 0;
    g_log[0] = '\0';
    g_present_pixel_sample_count = 0;
    g_present_pixel_arguments_valid = true;
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&test_log);
    teso4m4_lifecycle_set_present_pixel_sampler(
        &fake_present_pixel_sampler);
    teso4m4_lifecycle_set_startup_color_audit(true);
    teso4m4_lifecycle_set_startup_present_pixel_audit(true);

    PFN_vkCreateSwapchainKHR create_swapchain = (PFN_vkCreateSwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)&fake_create_swapchain);
    PFN_vkGetSwapchainImagesKHR get_images = (PFN_vkGetSwapchainImagesKHR)
        teso4m4_lifecycle_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)&fake_get_swapchain_images);
    PFN_vkQueueSubmit submit = (PFN_vkQueueSubmit)
        teso4m4_lifecycle_intercept(
            "vkQueueSubmit", (PFN_vkVoidFunction)&fake_queue_submit);
    PFN_vkQueuePresentKHR present = (PFN_vkQueuePresentKHR)
        teso4m4_lifecycle_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_queue_present);

    const VkDevice device = HANDLE(VkDevice, 0x81);
    const VkQueue queue = HANDLE(VkQueue, 0x82);
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {3420, 2148},
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    uint32_t image_count = 2;
    VkImage images[2] = {0};
    if (create_swapchain(device, &create_info, NULL, &swapchain) != VK_SUCCESS ||
        get_images(device, swapchain, &image_count, images) != VK_SUCCESS) {
        return check(false, "consumed semaphore setup failed");
    }
    VkSemaphore semaphore = HANDLE(VkSemaphore, 0xa00);
    const VkSubmitInfo signal_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &semaphore,
    };
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    const VkSubmitInfo consume_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &semaphore,
        .pWaitDstStageMask = &wait_stage,
    };
    uint32_t image_index = 0;
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &semaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };
    if (submit(queue, 1, &signal_info, VK_NULL_HANDLE) != VK_SUCCESS ||
        submit(queue, 1, &consume_info, VK_NULL_HANDLE) != VK_SUCCESS ||
        present(queue, &present_info) != VK_SUCCESS) {
        return check(false, "consumed semaphore forwarding failed");
    }
    return check(
        g_present_pixel_sample_count == 0 &&
            strstr(g_log, "synchronization=unconfirmed") != NULL,
        "a submit-consumed semaphore must not authorize pixel readback");
}

static bool run_startup_color_case(
    const char* label,
    bool issue_clear,
    float red,
    float green,
    float blue,
    float alpha,
    const char* expected_rgba) {
    g_log_length = 0;
    g_log[0] = '\0';
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&test_log);
    teso4m4_lifecycle_set_startup_color_audit(true);

    PFN_vkCreateSwapchainKHR create_swapchain = (PFN_vkCreateSwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)&fake_create_swapchain);
    PFN_vkGetSwapchainImagesKHR get_images = (PFN_vkGetSwapchainImagesKHR)
        teso4m4_lifecycle_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)&fake_get_swapchain_images);
    PFN_vkCreateImageView create_image_view = (PFN_vkCreateImageView)
        teso4m4_lifecycle_intercept(
            "vkCreateImageView", (PFN_vkVoidFunction)&fake_create_image_view);
    PFN_vkCreateRenderPass create_render_pass = (PFN_vkCreateRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCreateRenderPass",
            (PFN_vkVoidFunction)&fake_create_render_pass);
    PFN_vkCreateFramebuffer create_framebuffer = (PFN_vkCreateFramebuffer)
        teso4m4_lifecycle_intercept(
            "vkCreateFramebuffer",
            (PFN_vkVoidFunction)&fake_create_framebuffer);
    PFN_vkCmdBeginRenderPass begin_render_pass = (PFN_vkCmdBeginRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCmdBeginRenderPass",
            (PFN_vkVoidFunction)&fake_cmd_begin_render_pass);
    PFN_vkCmdClearAttachments clear_attachments =
        (PFN_vkCmdClearAttachments)teso4m4_lifecycle_intercept(
            "vkCmdClearAttachments",
            (PFN_vkVoidFunction)&fake_cmd_clear_attachments);
    PFN_vkCmdEndRenderPass end_render_pass = (PFN_vkCmdEndRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCmdEndRenderPass",
            (PFN_vkVoidFunction)&fake_cmd_end_render_pass);
    PFN_vkQueuePresentKHR present = (PFN_vkQueuePresentKHR)
        teso4m4_lifecycle_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_queue_present);
    PFN_vkQueueSubmit submit = (PFN_vkQueueSubmit)
        teso4m4_lifecycle_intercept(
            "vkQueueSubmit", (PFN_vkVoidFunction)&fake_queue_submit);

    VkDevice device = HANDLE(VkDevice, 0x71);
    VkQueue queue = HANDLE(VkQueue, 0x72);
    VkCommandBuffer command_buffer = HANDLE(VkCommandBuffer, 0x73);
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {3420, 2148},
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR first = VK_NULL_HANDLE;
    if (create_swapchain(device, &swapchain_info, NULL, &first) != VK_SUCCESS) {
        return check(false, "startup audit first swapchain creation failed");
    }
    uint32_t image_count = 2;
    VkImage images[2] = {0};
    if (get_images(device, first, &image_count, images) != VK_SUCCESS) {
        return check(false, "startup audit image query failed");
    }
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = images[0],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
    };
    VkImageView view = VK_NULL_HANDLE;
    if (create_image_view(device, &view_info, NULL, &view) != VK_SUCCESS) {
        return check(false, "startup audit image view creation failed");
    }
    const VkAttachmentDescription attachment_description = {
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    const VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment_description,
    };
    VkRenderPass render_pass = VK_NULL_HANDLE;
    if (create_render_pass(
            device, &render_pass_info, NULL, &render_pass) != VK_SUCCESS) {
        return check(false, "startup audit render pass creation failed");
    }
    const VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &view,
        .width = 3420,
        .height = 2148,
        .layers = 1,
    };
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (create_framebuffer(
            device, &framebuffer_info, NULL, &framebuffer) != VK_SUCCESS) {
        return check(false, "startup audit framebuffer creation failed");
    }
    const VkRenderPassBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {{0, 0}, {3420, 2148}},
    };
    begin_render_pass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    if (issue_clear) {
        const VkClearAttachment clear_attachment = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .colorAttachment = 0,
            .clearValue = {.color = {{red, green, blue, alpha}}},
        };
        const VkClearRect clear_rect = {
            .rect = {{0, 0}, {3420, 2148}},
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        clear_attachments(
            command_buffer, 1, &clear_attachment, 1, &clear_rect);
    }
    end_render_pass(command_buffer);
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    if (submit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        return check(false, "startup audit queue submit failed");
    }
    uint32_t image_index = 0;
    VkResult item_result = VK_SUCCESS;
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &first,
        .pImageIndices = &image_index,
        .pResults = &item_result,
    };
    if (present(queue, &present_info) != VK_SUCCESS) {
        return check(false, "startup audit present failed");
    }

    swapchain_info.oldSwapchain = first;
    swapchain_info.imageExtent.height = 2146;
    VkSwapchainKHR second = VK_NULL_HANDLE;
    if (create_swapchain(device, &swapchain_info, NULL, &second) != VK_SUCCESS) {
        return check(false, "startup audit replacement creation failed");
    }
    image_count = 2;
    memset(images, 0, sizeof(images));
    if (get_images(device, second, &image_count, images) != VK_SUCCESS) {
        return check(false, "startup audit replacement image query failed");
    }
    const VkImageViewCreateInfo second_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = images[0],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
    };
    VkImageView second_view = VK_NULL_HANDLE;
    if (create_image_view(
            device, &second_view_info, NULL, &second_view) != VK_SUCCESS) {
        return check(false, "startup audit replacement image view failed");
    }
    const VkFramebufferCreateInfo second_framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &second_view,
        .width = 3420,
        .height = 2146,
        .layers = 1,
    };
    VkFramebuffer second_framebuffer = VK_NULL_HANDLE;
    if (create_framebuffer(
            device, &second_framebuffer_info, NULL,
            &second_framebuffer) != VK_SUCCESS) {
        return check(false, "startup audit replacement framebuffer failed");
    }
    const VkRenderPassBeginInfo second_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = second_framebuffer,
        .renderArea = {{0, 0}, {3420, 2146}},
    };
    begin_render_pass(
        command_buffer, &second_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    const VkClearAttachment second_clear = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .colorAttachment = 0,
        .clearValue = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
    };
    const VkClearRect second_rect = {
        .rect = {{0, 0}, {3420, 2146}},
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    clear_attachments(
        command_buffer, 1, &second_clear, 1, &second_rect);
    end_render_pass(command_buffer);
    if (submit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        return check(false, "startup audit replacement submit failed");
    }
    const VkPresentInfoKHR second_present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &second,
        .pImageIndices = &image_index,
        .pResults = &item_result,
    };
    for (uint32_t ordinal = 0; ordinal < 180; ++ordinal) {
        if (present(queue, &second_present_info) != VK_SUCCESS) {
            return check(false, "startup audit replacement present failed");
        }
    }
    const size_t finished_length = g_log_length;
    clear_attachments(command_buffer, 1, NULL, 0, NULL);

    const bool has_clear =
        strstr(g_log, "STARTUP_COLOR_CLEAR: generation=1") != NULL;
    const bool expected_clear = issue_clear && expected_rgba &&
        strstr(g_log, expected_rgba) != NULL;
    return check(
        strstr(g_log, "STARTUP_COLOR_AUDIT_BEGIN:") != NULL &&
            strstr(g_log, "STARTUP_COLOR_BEGIN: generation=1") != NULL &&
            strstr(g_log, "framebuffer_extent=3420x2148") != NULL &&
            strstr(g_log, "STARTUP_COLOR_SUBMIT: generation=1") != NULL &&
            strstr(g_log, "STARTUP_COLOR_BEGIN: generation=2") != NULL &&
            strstr(g_log, "framebuffer_extent=3420x2146") != NULL &&
            strstr(g_log, "STARTUP_COLOR_CLEAR: generation=2") != NULL &&
            strstr(g_log, "STARTUP_COLOR_SUBMIT: generation=2") != NULL &&
            strstr(g_log,
                   "STARTUP_COLOR_AUDIT_FINISH: "
                   "reason=generation-2-present-limit generation=2 "
                   "ordinal=180") != NULL &&
            g_log_length == finished_length &&
            (issue_clear ? (has_clear && expected_clear) : !has_clear),
        label);
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

    if (!run_startup_color_case(
            "neon-pink startup clear must be recorded distinctly", true,
            1.0f, 0.0f, 1.0f, 1.0f, "rgba=1,0,1,1") ||
        !run_startup_color_case(
            "black startup clear must be recorded distinctly", true,
            0.0f, 0.0f, 0.0f, 1.0f, "rgba=0,0,0,1") ||
        !run_startup_color_case(
            "load-only startup submission must contain no clear record", false,
            0.0f, 0.0f, 0.0f, 0.0f, NULL)) {
        return 1;
    }
    if (!run_present_pixel_schedule_case() ||
        !run_consumed_semaphore_case()) {
        return 1;
    }

    printf(
        "Lifecycle trace smoke: yes startup_color_cases=3 "
        "present_pixel_cases=2 steady_pair_ns=%" PRIu64 "\n",
        pair_nanoseconds);
    return 0;
}
