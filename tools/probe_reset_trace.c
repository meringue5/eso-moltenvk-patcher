#include "mvk_reset_trace.h"
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
static VkPipelineCache g_last_graphics_cache;

static void test_log(const char* message) {
    const int written = snprintf(
        g_log + g_log_length, sizeof(g_log) - g_log_length, "%s\n", message);
    if (written > 0 && (size_t)written < sizeof(g_log) - g_log_length) {
        g_log_length += (size_t)written;
    }
}

static VkResult VKAPI_CALL fake_wait(VkDevice device) {
    (void)device;
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_swapchain(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* info,
    const VkAllocationCallbacks* allocator,
    VkSwapchainKHR* swapchain) {
    (void)device;
    (void)info;
    (void)allocator;
    *swapchain = HANDLE(VkSwapchainKHR, ++g_next_handle);
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_present(
    VkQueue queue,
    const VkPresentInfoKHR* info) {
    (void)queue;
    (void)info;
    return VK_SUBOPTIMAL_KHR;
}

static VkResult VKAPI_CALL fake_allocate_memory(
    VkDevice device,
    const VkMemoryAllocateInfo* info,
    const VkAllocationCallbacks* allocator,
    VkDeviceMemory* memory) {
    (void)device;
    (void)info;
    (void)allocator;
    *memory = HANDLE(VkDeviceMemory, ++g_next_handle);
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_image(
    VkDevice device,
    const VkImageCreateInfo* info,
    const VkAllocationCallbacks* allocator,
    VkImage* image) {
    (void)device;
    (void)info;
    (void)allocator;
    *image = HANDLE(VkImage, ++g_next_handle);
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_bind_image_memory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize offset) {
    (void)device;
    (void)image;
    (void)memory;
    (void)offset;
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_create_graphics_pipelines(
    VkDevice device,
    VkPipelineCache cache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* infos,
    const VkAllocationCallbacks* allocator,
    VkPipeline* pipelines) {
    (void)device;
    g_last_graphics_cache = cache;
    (void)infos;
    (void)allocator;
    for (uint32_t index = 0; index < count; ++index) {
        pipelines[index] = HANDLE(VkPipeline, ++g_next_handle);
    }
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL fake_queue_submit(
    VkQueue queue,
    uint32_t count,
    const VkSubmitInfo* submits,
    VkFence fence) {
    (void)queue;
    (void)count;
    (void)submits;
    (void)fence;
    return VK_SUCCESS;
}

static void VKAPI_CALL fake_cmd_draw(
    VkCommandBuffer buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) {
    (void)buffer;
    (void)vertex_count;
    (void)instance_count;
    (void)first_vertex;
    (void)first_instance;
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Reset trace probe failed: %s\n%s", message, g_log);
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

static PFN_vkVoidFunction chain_intercepts(
    const char* name,
    PFN_vkVoidFunction next_function) {
    return teso4m4_reset_trace_intercept(
        name, teso4m4_lifecycle_intercept(name, next_function));
}

int main(void) {
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&test_log);
    teso4m4_reset_trace_reset();
    teso4m4_reset_trace_set_logger(&test_log);
    teso4m4_reset_trace_set_pipeline_cache_bypass(true);
    teso4m4_reset_trace_set_full_lifetime_audit(true);

    PFN_vkDeviceWaitIdle wait = (PFN_vkDeviceWaitIdle)
        chain_intercepts(
            "vkDeviceWaitIdle", (PFN_vkVoidFunction)&fake_wait);
    PFN_vkCreateSwapchainKHR create_swapchain = (PFN_vkCreateSwapchainKHR)
        chain_intercepts(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)&fake_create_swapchain);
    PFN_vkQueuePresentKHR present = (PFN_vkQueuePresentKHR)
        chain_intercepts(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)&fake_present);
    PFN_vkAllocateMemory allocate_memory = (PFN_vkAllocateMemory)
        chain_intercepts(
            "vkAllocateMemory", (PFN_vkVoidFunction)&fake_allocate_memory);
    PFN_vkCreateImage create_image = (PFN_vkCreateImage)
        chain_intercepts(
            "vkCreateImage", (PFN_vkVoidFunction)&fake_create_image);
    PFN_vkBindImageMemory bind_image_memory = (PFN_vkBindImageMemory)
        chain_intercepts(
            "vkBindImageMemory",
            (PFN_vkVoidFunction)&fake_bind_image_memory);
    PFN_vkCreateGraphicsPipelines create_graphics =
        (PFN_vkCreateGraphicsPipelines)chain_intercepts(
            "vkCreateGraphicsPipelines",
            (PFN_vkVoidFunction)&fake_create_graphics_pipelines);
    PFN_vkQueueSubmit submit = (PFN_vkQueueSubmit)
        chain_intercepts(
            "vkQueueSubmit", (PFN_vkVoidFunction)&fake_queue_submit);
    PFN_vkCmdDraw draw = (PFN_vkCmdDraw)
        chain_intercepts(
            "vkCmdDraw", (PFN_vkVoidFunction)&fake_cmd_draw);

    const VkPipelineCache startup_cache =
        HANDLE(VkPipelineCache, 0x443);
    const VkGraphicsPipelineCreateInfo startup_graphics_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    };
    VkPipeline startup_pipeline = VK_NULL_HANDLE;
    if (!check(
            create_graphics(
                HANDLE(VkDevice, 0x1), startup_cache, 1,
                &startup_graphics_info, NULL, &startup_pipeline) ==
                VK_SUCCESS &&
                g_last_graphics_cache == startup_cache,
            "pipeline cache bypass must remain inactive before reset")) {
        return 1;
    }

    enum { kInactiveBenchmarkIterations = 100000 };
    VkCommandBuffer benchmark_buffer = HANDLE(VkCommandBuffer, 0x300);
    const uint64_t benchmark_start = monotonic_nanoseconds();
    for (uint32_t index = 0; index < kInactiveBenchmarkIterations; ++index) {
        draw(benchmark_buffer, 3, 1, 0, 0);
    }
    const uint64_t benchmark_end = monotonic_nanoseconds();
    const uint64_t inactive_draw_nanoseconds =
        benchmark_start != 0 && benchmark_end > benchmark_start
            ? (benchmark_end - benchmark_start) /
                  kInactiveBenchmarkIterations
            : UINT64_MAX;
    if (!check(
            inactive_draw_nanoseconds < 1000,
            "inactive draw wrapper must remain below 1 us")) {
        return 1;
    }

    VkDevice device = HANDLE(VkDevice, 0x1);
    VkQueue queue = HANDLE(VkQueue, 0x2);
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .imageExtent = {2048, 1280},
    };
    VkSwapchainKHR first = VK_NULL_HANDLE;
    VkSwapchainKHR second = VK_NULL_HANDLE;
    VkSwapchainKHR third = VK_NULL_HANDLE;
    if (!check(
            create_swapchain(device, &swapchain_info, NULL, &first) ==
                VK_SUCCESS,
            "first swapchain") ||
        !check(
            create_swapchain(device, &swapchain_info, NULL, &second) ==
                VK_SUCCESS,
            "second swapchain") ||
        !check(wait(device) == VK_SUCCESS, "reset wait")) {
        return 1;
    }

    VkMemoryAllocateInfo memory_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = 4096,
        .memoryTypeIndex = 2,
    };
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {256, 128, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };
    VkImage image = VK_NULL_HANDLE;
    VkGraphicsPipelineCreateInfo graphics_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    };
    VkPipeline pipelines[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    if (!check(
            allocate_memory(device, &memory_info, NULL, &memory) == VK_SUCCESS,
            "memory allocation") ||
        !check(
            create_image(device, &image_info, NULL, &image) == VK_SUCCESS,
            "image creation") ||
        !check(
            bind_image_memory(device, image, memory, 128) == VK_SUCCESS,
            "image binding") ||
        !check(
            create_graphics(
                device, HANDLE(VkPipelineCache, 0x444), 2,
                &graphics_info, NULL, pipelines) ==
                VK_SUCCESS,
            "pipeline creation")) {
        return 1;
    }

    VkCommandBuffer command_buffers[2] = {
        HANDLE(VkCommandBuffer, 0x301),
        HANDLE(VkCommandBuffer, 0x302),
    };
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 2,
        .pCommandBuffers = command_buffers,
    };
    draw(command_buffers[0], 3, 1, 0, 0);
    if (!check(
            submit(queue, 1, &submit_info, VK_NULL_HANDLE) == VK_SUCCESS,
            "queue submit")) {
        return 1;
    }

    swapchain_info.oldSwapchain = second;
    swapchain_info.imageExtent = (VkExtent2D){1920, 1200};
    if (!check(
            create_swapchain(device, &swapchain_info, NULL, &third) ==
                VK_SUCCESS,
            "reset swapchain")) {
        return 1;
    }
    uint32_t image_index = 0;
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &third,
        .pImageIndices = &image_index,
    };
    for (uint32_t index = 0; index < 8; ++index) {
        if (!check(
                present(queue, &present_info) == VK_SUBOPTIMAL_KHR,
                "suboptimal present must pass through")) {
            return 1;
        }
    }

    if (!check(
            strstr(
                g_log,
                "RENDER_AUDIT_BEGIN: mirror=enabled") != NULL &&
                strstr(g_log, "mirror_start_sequence=1") != NULL,
            "descriptor mirror must start at process sequence 1") ||
        !check(
            strstr(g_log, "RESET_RESOURCE_TRACE_BEGIN:") != NULL,
            "trace must arm after two swapchains") ||
        !check(
            strstr(g_log, "RESET_RESOURCE_TRACE_SWAPCHAIN: ordinal=3") != NULL,
            "third swapchain must be selected") ||
        !check(
            strstr(g_log, "RESET_RESOURCE_TRACE_SUMMARY: "
                          "reason=present-limit reset_presents=8 failures=0") !=
                NULL,
            "trace summary must complete without treating suboptimal as error") ||
        !check(
            strstr(g_log, "name=allocate_memory value=1") != NULL &&
                strstr(g_log, "name=create_image value=1") != NULL &&
                strstr(g_log, "name=bind_image_memory value=1") != NULL &&
                strstr(g_log, "name=create_graphics_pipelines value=2") !=
                    NULL &&
                strstr(g_log, "name=submitted_command_buffers value=2") !=
                    NULL &&
                strstr(g_log, "name=cmd_draw value=1") != NULL,
            "resource and command counts must be exact") ||
        !check(
            g_last_graphics_cache == VK_NULL_HANDLE &&
                strstr(g_log, "RESET_PIPELINE_CACHE_BYPASS:") != NULL,
            "active reset pipeline creation must bypass a non-null cache")) {
        return 1;
    }

    const char* const static_resource_targets[] = {
        "vkAllocateMemory",
        "vkFreeMemory",
        "vkMapMemory",
        "vkUnmapMemory",
        "vkBindBufferMemory",
        "vkBindImageMemory",
        "vkGetBufferMemoryRequirements",
        "vkGetImageMemoryRequirements",
        "vkCreateBuffer",
        "vkDestroyBuffer",
        "vkCreateImage",
        "vkDestroyImage",
        "vkCreateBufferView",
        "vkDestroyBufferView",
        "vkCreateSampler",
        "vkDestroySampler",
        "vkEndCommandBuffer",
        "vkResetCommandBuffer",
        "vkResetCommandPool",
        "vkAcquireNextImageKHR",
        "vkCreateSemaphore",
        "vkDestroySemaphore",
        "vkCreateFence",
        "vkDestroyFence",
        "vkResetFences",
        "vkWaitForFences",
        "vkCmdEndRenderPass",
        "vkCmdPipelineBarrier",
        "vkCmdCopyImage",
        "vkCmdBlitImage",
        "vkCmdResolveImage",
    };
    for (size_t index = 0;
         index < sizeof(static_resource_targets) /
                     sizeof(static_resource_targets[0]);
         ++index) {
        PFN_vkVoidFunction fake = (PFN_vkVoidFunction)&fake_cmd_draw;
        if (!check(
                teso4m4_reset_trace_intercept(
                    static_resource_targets[index], fake) != fake,
                "every static resource target must be intercepted")) {
            return 1;
        }
    }

    printf(
        "Reset resource trace smoke: yes inactive_draw_ns=%" PRIu64 "\n",
        inactive_draw_nanoseconds);
    return 0;
}
