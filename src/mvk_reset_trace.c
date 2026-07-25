#include "mvk_reset_trace.h"
#include "mvk_render_audit.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    kDetailLimit = 48,
    kPresentLimit = 8,
};

typedef enum {
    kAllocateMemory,
    kFreeMemory,
    kMapMemory,
    kUnmapMemory,
    kCreateBuffer,
    kDestroyBuffer,
    kBindBufferMemory,
    kGetBufferMemoryRequirements,
    kCreateImage,
    kDestroyImage,
    kBindImageMemory,
    kGetImageMemoryRequirements,
    kCreateImageView,
    kDestroyImageView,
    kCreateRenderPass,
    kDestroyRenderPass,
    kCreateFramebuffer,
    kDestroyFramebuffer,
    kCreateDescriptorPool,
    kDestroyDescriptorPool,
    kResetDescriptorPool,
    kAllocateDescriptorSets,
    kFreeDescriptorSets,
    kUpdateDescriptorWrites,
    kUpdateDescriptorCopies,
    kCreateDescriptorSetLayout,
    kDestroyDescriptorSetLayout,
    kCreatePipelineLayout,
    kDestroyPipelineLayout,
    kCreateGraphicsPipelines,
    kCreateComputePipelines,
    kDestroyPipeline,
    kAllocateCommandBuffers,
    kFreeCommandBuffers,
    kBeginCommandBuffer,
    kEndCommandBuffer,
    kQueueSubmit,
    kSubmittedCommandBuffers,
    kCmdBeginRenderPass,
    kCmdBindPipeline,
    kCmdBindDescriptorSets,
    kCmdDraw,
    kCmdDrawIndexed,
    kCmdDispatch,
    kCmdEndRenderPass,
    kCounterCount,
} ResetCounter;

static const char* const kCounterNames[kCounterCount] = {
    [kAllocateMemory] = "allocate_memory",
    [kFreeMemory] = "free_memory",
    [kMapMemory] = "map_memory",
    [kUnmapMemory] = "unmap_memory",
    [kCreateBuffer] = "create_buffer",
    [kDestroyBuffer] = "destroy_buffer",
    [kBindBufferMemory] = "bind_buffer_memory",
    [kGetBufferMemoryRequirements] = "get_buffer_memory_requirements",
    [kCreateImage] = "create_image",
    [kDestroyImage] = "destroy_image",
    [kBindImageMemory] = "bind_image_memory",
    [kGetImageMemoryRequirements] = "get_image_memory_requirements",
    [kCreateImageView] = "create_image_view",
    [kDestroyImageView] = "destroy_image_view",
    [kCreateRenderPass] = "create_render_pass",
    [kDestroyRenderPass] = "destroy_render_pass",
    [kCreateFramebuffer] = "create_framebuffer",
    [kDestroyFramebuffer] = "destroy_framebuffer",
    [kCreateDescriptorPool] = "create_descriptor_pool",
    [kDestroyDescriptorPool] = "destroy_descriptor_pool",
    [kResetDescriptorPool] = "reset_descriptor_pool",
    [kAllocateDescriptorSets] = "allocate_descriptor_sets",
    [kFreeDescriptorSets] = "free_descriptor_sets",
    [kUpdateDescriptorWrites] = "update_descriptor_writes",
    [kUpdateDescriptorCopies] = "update_descriptor_copies",
    [kCreateDescriptorSetLayout] = "create_descriptor_set_layout",
    [kDestroyDescriptorSetLayout] = "destroy_descriptor_set_layout",
    [kCreatePipelineLayout] = "create_pipeline_layout",
    [kDestroyPipelineLayout] = "destroy_pipeline_layout",
    [kCreateGraphicsPipelines] = "create_graphics_pipelines",
    [kCreateComputePipelines] = "create_compute_pipelines",
    [kDestroyPipeline] = "destroy_pipeline",
    [kAllocateCommandBuffers] = "allocate_command_buffers",
    [kFreeCommandBuffers] = "free_command_buffers",
    [kBeginCommandBuffer] = "begin_command_buffer",
    [kEndCommandBuffer] = "end_command_buffer",
    [kQueueSubmit] = "queue_submit",
    [kSubmittedCommandBuffers] = "submitted_command_buffers",
    [kCmdBeginRenderPass] = "cmd_begin_render_pass",
    [kCmdBindPipeline] = "cmd_bind_pipeline",
    [kCmdBindDescriptorSets] = "cmd_bind_descriptor_sets",
    [kCmdDraw] = "cmd_draw",
    [kCmdDrawIndexed] = "cmd_draw_indexed",
    [kCmdDispatch] = "cmd_dispatch",
    [kCmdEndRenderPass] = "cmd_end_render_pass",
};

static PFN_vkDeviceWaitIdle g_next_device_wait_idle;
static PFN_vkCreateSwapchainKHR g_next_create_swapchain;
static PFN_vkQueuePresentKHR g_next_queue_present;
static PFN_vkAllocateMemory g_next_allocate_memory;
static PFN_vkFreeMemory g_next_free_memory;
static PFN_vkMapMemory g_next_map_memory;
static PFN_vkUnmapMemory g_next_unmap_memory;
static PFN_vkCreateBuffer g_next_create_buffer;
static PFN_vkDestroyBuffer g_next_destroy_buffer;
static PFN_vkCreateBufferView g_next_create_buffer_view;
static PFN_vkDestroyBufferView g_next_destroy_buffer_view;
static PFN_vkBindBufferMemory g_next_bind_buffer_memory;
static PFN_vkGetBufferMemoryRequirements
    g_next_get_buffer_memory_requirements;
static PFN_vkCreateImage g_next_create_image;
static PFN_vkDestroyImage g_next_destroy_image;
static PFN_vkBindImageMemory g_next_bind_image_memory;
static PFN_vkGetImageMemoryRequirements g_next_get_image_memory_requirements;
static PFN_vkCreateImageView g_next_create_image_view;
static PFN_vkDestroyImageView g_next_destroy_image_view;
static PFN_vkCreateSampler g_next_create_sampler;
static PFN_vkDestroySampler g_next_destroy_sampler;
static PFN_vkCreateRenderPass g_next_create_render_pass;
static PFN_vkDestroyRenderPass g_next_destroy_render_pass;
static PFN_vkCreateFramebuffer g_next_create_framebuffer;
static PFN_vkDestroyFramebuffer g_next_destroy_framebuffer;
static PFN_vkCreateDescriptorPool g_next_create_descriptor_pool;
static PFN_vkDestroyDescriptorPool g_next_destroy_descriptor_pool;
static PFN_vkResetDescriptorPool g_next_reset_descriptor_pool;
static PFN_vkAllocateDescriptorSets g_next_allocate_descriptor_sets;
static PFN_vkFreeDescriptorSets g_next_free_descriptor_sets;
static PFN_vkUpdateDescriptorSets g_next_update_descriptor_sets;
static PFN_vkCreateDescriptorSetLayout g_next_create_descriptor_set_layout;
static PFN_vkDestroyDescriptorSetLayout g_next_destroy_descriptor_set_layout;
static PFN_vkCreatePipelineLayout g_next_create_pipeline_layout;
static PFN_vkDestroyPipelineLayout g_next_destroy_pipeline_layout;
static PFN_vkCreateGraphicsPipelines g_next_create_graphics_pipelines;
static PFN_vkCreateComputePipelines g_next_create_compute_pipelines;
static PFN_vkDestroyPipeline g_next_destroy_pipeline;
static PFN_vkAllocateCommandBuffers g_next_allocate_command_buffers;
static PFN_vkFreeCommandBuffers g_next_free_command_buffers;
static PFN_vkResetCommandBuffer g_next_reset_command_buffer;
static PFN_vkResetCommandPool g_next_reset_command_pool;
static PFN_vkBeginCommandBuffer g_next_begin_command_buffer;
static PFN_vkEndCommandBuffer g_next_end_command_buffer;
static PFN_vkQueueSubmit g_next_queue_submit;
static PFN_vkAcquireNextImageKHR g_next_acquire_next_image;
static PFN_vkCreateSemaphore g_next_create_semaphore;
static PFN_vkDestroySemaphore g_next_destroy_semaphore;
static PFN_vkCreateFence g_next_create_fence;
static PFN_vkDestroyFence g_next_destroy_fence;
static PFN_vkResetFences g_next_reset_fences;
static PFN_vkWaitForFences g_next_wait_for_fences;
static PFN_vkCmdBeginRenderPass g_next_cmd_begin_render_pass;
static PFN_vkCmdBindPipeline g_next_cmd_bind_pipeline;
static PFN_vkCmdBindDescriptorSets g_next_cmd_bind_descriptor_sets;
static PFN_vkCmdDraw g_next_cmd_draw;
static PFN_vkCmdDrawIndexed g_next_cmd_draw_indexed;
static PFN_vkCmdDispatch g_next_cmd_dispatch;
static PFN_vkCmdEndRenderPass g_next_cmd_end_render_pass;
static PFN_vkCmdPipelineBarrier g_next_cmd_pipeline_barrier;
static PFN_vkCmdCopyImage g_next_cmd_copy_image;
static PFN_vkCmdBlitImage g_next_cmd_blit_image;
static PFN_vkCmdResolveImage g_next_cmd_resolve_image;

static Teso4m4ResetTraceLogFunction g_logger;
static pthread_mutex_t g_state_lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool g_active;
static atomic_bool g_complete;
static atomic_bool g_pipeline_cache_bypass;
static atomic_bool g_full_lifetime_audit;
static atomic_uint_fast64_t g_counters[kCounterCount];
static atomic_uint_fast64_t g_failures;
static atomic_uint_fast64_t g_detail_count;
static uint64_t g_wait_count;
static uint64_t g_swapchain_count;
static uint32_t g_reset_present_count;
static VkSwapchainKHR g_reset_swapchain;

static void trace_log(const char* format, ...) {
    if (!g_logger) {
        return;
    }
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    g_logger(message);
}

static bool trace_active(void) {
    return atomic_load_explicit(&g_active, memory_order_relaxed);
}

static void count_add(ResetCounter counter, uint64_t value) {
    if (trace_active()) {
        atomic_fetch_add_explicit(
            &g_counters[counter], value, memory_order_relaxed);
    }
}

static bool detail_slot(void) {
    if (!trace_active()) {
        return false;
    }
    return atomic_fetch_add_explicit(
               &g_detail_count, 1, memory_order_relaxed) < kDetailLimit;
}

static void record_result(const char* operation, VkResult result) {
    if (trace_active() && result < VK_SUCCESS) {
        atomic_fetch_add_explicit(&g_failures, 1, memory_order_relaxed);
        trace_log("RESET_RESOURCE_FAILURE: operation=%s result=%d",
                  operation, result);
    }
}

static void begin_trace(uint64_t wait_count, uint64_t swapchain_count) {
    for (size_t index = 0; index < kCounterCount; ++index) {
        atomic_store_explicit(&g_counters[index], 0, memory_order_relaxed);
    }
    atomic_store_explicit(&g_failures, 0, memory_order_relaxed);
    atomic_store_explicit(&g_detail_count, 0, memory_order_relaxed);
    g_reset_swapchain = VK_NULL_HANDLE;
    g_reset_present_count = 0;
    atomic_store_explicit(&g_active, true, memory_order_release);
    teso4m4_render_audit_begin();
    trace_log(
        "RESET_RESOURCE_TRACE_BEGIN: wait=%" PRIu64
        " established_swapchains=%" PRIu64 " detail_limit=%u present_limit=%u",
        wait_count, swapchain_count, kDetailLimit, kPresentLimit);
}

static void finish_trace(const char* reason) {
    bool expected = false;
    if (!atomic_compare_exchange_strong_explicit(
            &g_complete, &expected, true,
            memory_order_acq_rel, memory_order_relaxed)) {
        return;
    }
    atomic_store_explicit(&g_active, false, memory_order_release);
    teso4m4_render_audit_finish(reason);
    trace_log(
        "RESET_RESOURCE_TRACE_SUMMARY: reason=%s reset_presents=%u "
        "failures=%" PRIuFAST64 " details=%" PRIuFAST64,
        reason, g_reset_present_count,
        atomic_load_explicit(&g_failures, memory_order_relaxed),
        atomic_load_explicit(&g_detail_count, memory_order_relaxed));
    for (size_t index = 0; index < kCounterCount; ++index) {
        trace_log(
            "RESET_RESOURCE_COUNT: name=%s value=%" PRIuFAST64,
            kCounterNames[index],
            atomic_load_explicit(&g_counters[index], memory_order_relaxed));
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_device_wait_idle(
    VkDevice device) {
    if (!g_next_device_wait_idle) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_device_wait_idle(device);
    pthread_mutex_lock(&g_state_lock);
    ++g_wait_count;
    if (result == VK_SUCCESS && g_swapchain_count >= 2 &&
        !atomic_load_explicit(&g_active, memory_order_relaxed) &&
        !atomic_load_explicit(&g_complete, memory_order_relaxed)) {
        begin_trace(g_wait_count, g_swapchain_count);
    }
    pthread_mutex_unlock(&g_state_lock);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_swapchain(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* create_info,
    const VkAllocationCallbacks* allocator,
    VkSwapchainKHR* swapchain) {
    if (!g_next_create_swapchain) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_create_swapchain(device, create_info, allocator, swapchain);
    pthread_mutex_lock(&g_state_lock);
    if (result == VK_SUCCESS && swapchain && *swapchain != VK_NULL_HANDLE) {
        ++g_swapchain_count;
        if (g_swapchain_count == 1) {
            teso4m4_render_audit_enable_mirror();
        }
        if (trace_active() && g_reset_swapchain == VK_NULL_HANDLE) {
            g_reset_swapchain = *swapchain;
            trace_log(
                "RESET_RESOURCE_TRACE_SWAPCHAIN: ordinal=%" PRIu64
                " swapchain=%p extent=%ux%u",
                g_swapchain_count, (void*)*swapchain,
                create_info ? create_info->imageExtent.width : 0,
                create_info ? create_info->imageExtent.height : 0);
        }
    }
    pthread_mutex_unlock(&g_state_lock);
    record_result("vkCreateSwapchainKHR", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_queue_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    if (!g_next_queue_present) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_queue_present(queue, present_info);
    teso4m4_render_audit_queue_present(queue, present_info, result);
    bool finish = false;
    if (trace_active() && present_info) {
        pthread_mutex_lock(&g_state_lock);
        for (uint32_t index = 0;
             index < present_info->swapchainCount;
             ++index) {
            if (present_info->pSwapchains[index] == g_reset_swapchain) {
                ++g_reset_present_count;
                finish |= g_reset_present_count >= kPresentLimit;
            }
        }
        pthread_mutex_unlock(&g_state_lock);
    }
    record_result("vkQueuePresentKHR", result);
    if (finish) {
        finish_trace("present-limit");
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_allocate_memory(
    VkDevice device,
    const VkMemoryAllocateInfo* allocate_info,
    const VkAllocationCallbacks* allocator,
    VkDeviceMemory* memory) {
    if (!g_next_allocate_memory) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_allocate_memory(device, allocate_info, allocator, memory);
    count_add(kAllocateMemory, 1);
    record_result("vkAllocateMemory", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=allocate_memory size=%" PRIu64
            " type=%u result=%d memory=%p",
            allocate_info ? (uint64_t)allocate_info->allocationSize : 0,
            allocate_info ? allocate_info->memoryTypeIndex : UINT32_MAX,
            result, memory ? (void*)*memory : NULL);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_free_memory(
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_free_memory) {
        return;
    }
    g_next_free_memory(device, memory, allocator);
    count_add(kFreeMemory, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_map_memory(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** data) {
    if (!g_next_map_memory) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_map_memory(device, memory, offset, size, flags, data);
    count_add(kMapMemory, 1);
    record_result("vkMapMemory", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_unmap_memory(
    VkDevice device,
    VkDeviceMemory memory) {
    if (!g_next_unmap_memory) {
        return;
    }
    g_next_unmap_memory(device, memory);
    count_add(kUnmapMemory, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_buffer(
    VkDevice device,
    const VkBufferCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkBuffer* buffer) {
    if (!g_next_create_buffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_create_buffer(device, create_info, allocator, buffer);
    if (result == VK_SUCCESS && buffer && *buffer != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_buffer(*buffer);
    }
    count_add(kCreateBuffer, 1);
    record_result("vkCreateBuffer", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=create_buffer size=%" PRIu64
            " usage=0x%x flags=0x%x result=%d buffer=%p",
            create_info ? (uint64_t)create_info->size : 0,
            create_info ? create_info->usage : 0,
            create_info ? create_info->flags : 0,
            result, buffer ? (void*)*buffer : NULL);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_buffer(
    VkDevice device,
    VkBuffer buffer,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_buffer) {
        return;
    }
    teso4m4_render_audit_destroy_buffer(buffer);
    g_next_destroy_buffer(device, buffer, allocator);
    count_add(kDestroyBuffer, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_buffer_view(
    VkDevice device, const VkBufferViewCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkBufferView* buffer_view) {
    if (!g_next_create_buffer_view) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_buffer_view(
        device, create_info, allocator, buffer_view);
    if (result == VK_SUCCESS && buffer_view &&
        *buffer_view != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_buffer_view(*buffer_view);
    }
    record_result("vkCreateBufferView", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_buffer_view(
    VkDevice device, VkBufferView buffer_view,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_buffer_view) {
        return;
    }
    teso4m4_render_audit_destroy_buffer_view(buffer_view);
    g_next_destroy_buffer_view(device, buffer_view, allocator);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_bind_buffer_memory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize offset) {
    if (!g_next_bind_buffer_memory) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_bind_buffer_memory(device, buffer, memory, offset);
    count_add(kBindBufferMemory, 1);
    record_result("vkBindBufferMemory", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_get_buffer_memory_requirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* requirements) {
    if (!g_next_get_buffer_memory_requirements) {
        return;
    }
    g_next_get_buffer_memory_requirements(device, buffer, requirements);
    count_add(kGetBufferMemoryRequirements, 1);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=get_buffer_memory_requirements "
            "size=%" PRIu64 " alignment=%" PRIu64 " type_bits=0x%x",
            requirements ? (uint64_t)requirements->size : 0,
            requirements ? (uint64_t)requirements->alignment : 0,
            requirements ? requirements->memoryTypeBits : 0);
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_image(
    VkDevice device,
    const VkImageCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkImage* image) {
    if (!g_next_create_image) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_create_image(device, create_info, allocator, image);
    if (result == VK_SUCCESS && image && *image != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_image(create_info, *image);
    }
    count_add(kCreateImage, 1);
    record_result("vkCreateImage", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=create_image format=%d "
            "extent=%ux%ux%u mips=%u layers=%u samples=%u tiling=%d "
            "usage=0x%x flags=0x%x result=%d image=%p",
            create_info ? create_info->format : 0,
            create_info ? create_info->extent.width : 0,
            create_info ? create_info->extent.height : 0,
            create_info ? create_info->extent.depth : 0,
            create_info ? create_info->mipLevels : 0,
            create_info ? create_info->arrayLayers : 0,
            create_info ? create_info->samples : 0,
            create_info ? create_info->tiling : 0,
            create_info ? create_info->usage : 0,
            create_info ? create_info->flags : 0,
            result, image ? (void*)*image : NULL);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_image(
    VkDevice device,
    VkImage image,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_image) {
        return;
    }
    teso4m4_render_audit_destroy_image(image);
    g_next_destroy_image(device, image, allocator);
    count_add(kDestroyImage, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_bind_image_memory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize offset) {
    if (!g_next_bind_image_memory) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_bind_image_memory(device, image, memory, offset);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_bind_image(image, memory, offset);
    }
    count_add(kBindImageMemory, 1);
    record_result("vkBindImageMemory", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=bind_image_memory image=%p "
            "memory=%p offset=%" PRIu64 " result=%d",
            (void*)image, (void*)memory, (uint64_t)offset, result);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_get_image_memory_requirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* requirements) {
    if (!g_next_get_image_memory_requirements) {
        return;
    }
    g_next_get_image_memory_requirements(device, image, requirements);
    if (requirements) {
        teso4m4_render_audit_image_requirements(image, requirements);
    }
    count_add(kGetImageMemoryRequirements, 1);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=get_image_memory_requirements "
            "size=%" PRIu64 " alignment=%" PRIu64 " type_bits=0x%x",
            requirements ? (uint64_t)requirements->size : 0,
            requirements ? (uint64_t)requirements->alignment : 0,
            requirements ? requirements->memoryTypeBits : 0);
    }
}

#define DEFINE_CREATE_WRAPPER(                                             \
    suffix, FunctionType, InfoType, HandleType, next_name, counter_name)   \
    static VKAPI_ATTR VkResult VKAPI_CALL traced_##suffix(                 \
        VkDevice device, const InfoType* create_info,                      \
        const VkAllocationCallbacks* allocator, HandleType* handle) {      \
        if (!next_name) {                                                   \
            return VK_ERROR_INITIALIZATION_FAILED;                          \
        }                                                                   \
        VkResult result = next_name(                                        \
            device, create_info, allocator, handle);                        \
        count_add(counter_name, 1);                                         \
        record_result("vk" #FunctionType, result);                          \
        return result;                                                      \
    }

#define DEFINE_DESTROY_WRAPPER(                                            \
    suffix, HandleType, next_name, counter_name)                           \
    static VKAPI_ATTR void VKAPI_CALL traced_##suffix(                     \
        VkDevice device, HandleType handle,                                \
        const VkAllocationCallbacks* allocator) {                          \
        if (!next_name) {                                                   \
            return;                                                         \
        }                                                                   \
        next_name(device, handle, allocator);                               \
        count_add(counter_name, 1);                                         \
    }

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_image_view(
    VkDevice device,
    const VkImageViewCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkImageView* view) {
    if (!g_next_create_image_view) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_create_image_view(device, create_info, allocator, view);
    if (result == VK_SUCCESS && view && *view != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_image_view(create_info, *view);
    }
    count_add(kCreateImageView, 1);
    record_result("vkCreateImageView", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_image_view(
    VkDevice device,
    VkImageView view,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_image_view) {
        return;
    }
    teso4m4_render_audit_destroy_image_view(view);
    g_next_destroy_image_view(device, view, allocator);
    count_add(kDestroyImageView, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_sampler(
    VkDevice device, const VkSamplerCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkSampler* sampler) {
    if (!g_next_create_sampler) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_sampler(
        device, create_info, allocator, sampler);
    if (result == VK_SUCCESS && sampler &&
        *sampler != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_sampler(*sampler);
    }
    record_result("vkCreateSampler", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_sampler(
    VkDevice device, VkSampler sampler,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_sampler) {
        return;
    }
    teso4m4_render_audit_destroy_sampler(sampler);
    g_next_destroy_sampler(device, sampler, allocator);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_render_pass(
    VkDevice device,
    const VkRenderPassCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkRenderPass* render_pass) {
    if (!g_next_create_render_pass) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_render_pass(
        device, create_info, allocator, render_pass);
    if (result == VK_SUCCESS && render_pass &&
        *render_pass != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_render_pass(
            create_info, *render_pass);
    }
    count_add(kCreateRenderPass, 1);
    record_result("vkCreateRenderPass", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_render_pass(
    VkDevice device,
    VkRenderPass render_pass,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_render_pass) {
        return;
    }
    teso4m4_render_audit_destroy_render_pass(render_pass);
    g_next_destroy_render_pass(device, render_pass, allocator);
    count_add(kDestroyRenderPass, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_framebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkFramebuffer* framebuffer) {
    if (!g_next_create_framebuffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_framebuffer(
        device, create_info, allocator, framebuffer);
    if (result == VK_SUCCESS && framebuffer &&
        *framebuffer != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_framebuffer(
            create_info, *framebuffer);
    }
    count_add(kCreateFramebuffer, 1);
    record_result("vkCreateFramebuffer", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_framebuffer(
    VkDevice device,
    VkFramebuffer framebuffer,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_framebuffer) {
        return;
    }
    teso4m4_render_audit_destroy_framebuffer(framebuffer);
    g_next_destroy_framebuffer(device, framebuffer, allocator);
    count_add(kDestroyFramebuffer, 1);
}
DEFINE_CREATE_WRAPPER(
    create_descriptor_pool, CreateDescriptorPool, VkDescriptorPoolCreateInfo,
    VkDescriptorPool, g_next_create_descriptor_pool, kCreateDescriptorPool)
static VKAPI_ATTR void VKAPI_CALL traced_destroy_descriptor_pool(
    VkDevice device,
    VkDescriptorPool pool,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_descriptor_pool) {
        return;
    }
    teso4m4_render_audit_destroy_descriptor_pool(pool);
    g_next_destroy_descriptor_pool(device, pool, allocator);
    count_add(kDestroyDescriptorPool, 1);
}
static VKAPI_ATTR VkResult VKAPI_CALL traced_create_descriptor_set_layout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDescriptorSetLayout* layout) {
    if (!g_next_create_descriptor_set_layout) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_descriptor_set_layout(
        device, create_info, allocator, layout);
    if (result == VK_SUCCESS && layout &&
        *layout != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_descriptor_set_layout(
            create_info, *layout);
    }
    count_add(kCreateDescriptorSetLayout, 1);
    record_result("vkCreateDescriptorSetLayout", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_descriptor_set_layout(
    VkDevice device,
    VkDescriptorSetLayout layout,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_descriptor_set_layout) {
        return;
    }
    teso4m4_render_audit_destroy_descriptor_set_layout(layout);
    g_next_destroy_descriptor_set_layout(device, layout, allocator);
    count_add(kDestroyDescriptorSetLayout, 1);
}
DEFINE_CREATE_WRAPPER(
    create_pipeline_layout, CreatePipelineLayout, VkPipelineLayoutCreateInfo,
    VkPipelineLayout, g_next_create_pipeline_layout, kCreatePipelineLayout)
DEFINE_DESTROY_WRAPPER(
    destroy_pipeline_layout, VkPipelineLayout, g_next_destroy_pipeline_layout,
    kDestroyPipelineLayout)
static VKAPI_ATTR void VKAPI_CALL traced_destroy_pipeline(
    VkDevice device,
    VkPipeline pipeline,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_pipeline) {
        return;
    }
    teso4m4_render_audit_destroy_pipeline(pipeline);
    g_next_destroy_pipeline(device, pipeline, allocator);
    count_add(kDestroyPipeline, 1);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_reset_descriptor_pool(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorPoolResetFlags flags) {
    if (!g_next_reset_descriptor_pool) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_reset_descriptor_pool(device, pool, flags);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_reset_descriptor_pool(pool);
    }
    count_add(kResetDescriptorPool, 1);
    record_result("vkResetDescriptorPool", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_allocate_descriptor_sets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* allocate_info,
    VkDescriptorSet* sets) {
    if (!g_next_allocate_descriptor_sets) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_allocate_descriptor_sets(device, allocate_info, sets);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_allocate_descriptor_sets(
            allocate_info, sets);
    }
    count_add(
        kAllocateDescriptorSets,
        allocate_info ? allocate_info->descriptorSetCount : 0);
    record_result("vkAllocateDescriptorSets", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_free_descriptor_sets(
    VkDevice device,
    VkDescriptorPool pool,
    uint32_t count,
    const VkDescriptorSet* sets) {
    if (!g_next_free_descriptor_sets) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_free_descriptor_sets(device, pool, count, sets);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_free_descriptor_sets(pool, count, sets);
    }
    count_add(kFreeDescriptorSets, count);
    record_result("vkFreeDescriptorSets", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_update_descriptor_sets(
    VkDevice device,
    uint32_t write_count,
    const VkWriteDescriptorSet* writes,
    uint32_t copy_count,
    const VkCopyDescriptorSet* copies) {
    if (!g_next_update_descriptor_sets) {
        return;
    }
    g_next_update_descriptor_sets(
        device, write_count, writes, copy_count, copies);
    teso4m4_render_audit_update_descriptor_sets(
        write_count, writes, copy_count, copies);
    count_add(kUpdateDescriptorWrites, write_count);
    count_add(kUpdateDescriptorCopies, copy_count);
}

static uint32_t count_nonnull_pipelines(
    uint32_t count, const VkPipeline* pipelines) {
    uint32_t nonnull = 0;
    if (pipelines) {
        for (uint32_t index = 0; index < count; ++index) {
            nonnull += pipelines[index] != VK_NULL_HANDLE;
        }
    }
    return nonnull;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_graphics_pipelines(
    VkDevice device,
    VkPipelineCache cache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* create_infos,
    const VkAllocationCallbacks* allocator,
    VkPipeline* pipelines) {
    if (!g_next_create_graphics_pipelines) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const bool bypass_cache =
        trace_active() &&
        atomic_load_explicit(
            &g_pipeline_cache_bypass, memory_order_relaxed) &&
        cache != VK_NULL_HANDLE;
    const VkPipelineCache forwarded_cache =
        bypass_cache ? VK_NULL_HANDLE : cache;
    VkResult result = g_next_create_graphics_pipelines(
        device, forwarded_cache, count, create_infos, allocator, pipelines);
    teso4m4_render_audit_create_graphics_pipelines(
        forwarded_cache, count, create_infos, pipelines);
    count_add(kCreateGraphicsPipelines, count);
    record_result("vkCreateGraphicsPipelines", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=create_graphics_pipelines "
            "requested=%u nonnull=%u result=%d",
            count, count_nonnull_pipelines(count, pipelines), result);
    }
    if (bypass_cache) {
        trace_log(
            "RESET_PIPELINE_CACHE_BYPASS: requested_cache=%p forwarded_cache=%p"
            " pipelines=%u result=%d",
            (void*)cache, (void*)forwarded_cache, count, result);
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_compute_pipelines(
    VkDevice device,
    VkPipelineCache cache,
    uint32_t count,
    const VkComputePipelineCreateInfo* create_infos,
    const VkAllocationCallbacks* allocator,
    VkPipeline* pipelines) {
    if (!g_next_create_compute_pipelines) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_compute_pipelines(
        device, cache, count, create_infos, allocator, pipelines);
    count_add(kCreateComputePipelines, count);
    record_result("vkCreateComputePipelines", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=create_compute_pipelines "
            "requested=%u nonnull=%u result=%d",
            count, count_nonnull_pipelines(count, pipelines), result);
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_allocate_command_buffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* allocate_info,
    VkCommandBuffer* buffers) {
    if (!g_next_allocate_command_buffers) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_allocate_command_buffers(device, allocate_info, buffers);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_allocate_command_buffers(
            allocate_info, buffers);
    }
    count_add(
        kAllocateCommandBuffers,
        allocate_info ? allocate_info->commandBufferCount : 0);
    record_result("vkAllocateCommandBuffers", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_free_command_buffers(
    VkDevice device,
    VkCommandPool pool,
    uint32_t count,
    const VkCommandBuffer* buffers) {
    if (!g_next_free_command_buffers) {
        return;
    }
    teso4m4_render_audit_free_command_buffers(pool, count, buffers);
    g_next_free_command_buffers(device, pool, count, buffers);
    count_add(kFreeCommandBuffers, count);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_reset_command_buffer(
    VkCommandBuffer buffer, VkCommandBufferResetFlags flags) {
    if (!g_next_reset_command_buffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_reset_command_buffer(buffer, flags);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_reset_command_buffer(buffer);
    }
    record_result("vkResetCommandBuffer", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_reset_command_pool(
    VkDevice device, VkCommandPool pool,
    VkCommandPoolResetFlags flags) {
    if (!g_next_reset_command_pool) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_reset_command_pool(device, pool, flags);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_reset_command_pool(pool);
    }
    record_result("vkResetCommandPool", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_begin_command_buffer(
    VkCommandBuffer buffer,
    const VkCommandBufferBeginInfo* begin_info) {
    if (!g_next_begin_command_buffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_begin_command_buffer(buffer, begin_info);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_begin_command_buffer(buffer);
    }
    count_add(kBeginCommandBuffer, 1);
    record_result("vkBeginCommandBuffer", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_end_command_buffer(
    VkCommandBuffer buffer) {
    if (!g_next_end_command_buffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_end_command_buffer(buffer);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_end_command_buffer(buffer);
    }
    count_add(kEndCommandBuffer, 1);
    record_result("vkEndCommandBuffer", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_queue_submit(
    VkQueue queue,
    uint32_t submit_count,
    const VkSubmitInfo* submits,
    VkFence fence) {
    if (!g_next_queue_submit) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_queue_submit(queue, submit_count, submits, fence);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_queue_submit(
            queue, submit_count, submits, fence);
    }
    uint64_t command_buffers = 0;
    for (uint32_t index = 0; submits && index < submit_count; ++index) {
        command_buffers += submits[index].commandBufferCount;
    }
    count_add(kQueueSubmit, 1);
    count_add(kSubmittedCommandBuffers, command_buffers);
    record_result("vkQueueSubmit", result);
    if (detail_slot()) {
        trace_log(
            "RESET_RESOURCE_DETAIL: operation=queue_submit submits=%u "
            "command_buffers=%" PRIu64 " result=%d",
            submit_count, command_buffers, result);
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_acquire_next_image(
    VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, uint32_t* image_index) {
    if (!g_next_acquire_next_image) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_acquire_next_image(
        device, swapchain, timeout, semaphore, fence, image_index);
    teso4m4_render_audit_acquire_next_image(semaphore, fence, result);
    record_result("vkAcquireNextImageKHR", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_semaphore(
    VkDevice device, const VkSemaphoreCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkSemaphore* semaphore) {
    if (!g_next_create_semaphore) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_semaphore(
        device, create_info, allocator, semaphore);
    if (result == VK_SUCCESS && semaphore &&
        *semaphore != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_semaphore(*semaphore);
    }
    record_result("vkCreateSemaphore", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_semaphore(
    VkDevice device, VkSemaphore semaphore,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_semaphore) {
        return;
    }
    teso4m4_render_audit_destroy_semaphore(semaphore);
    g_next_destroy_semaphore(device, semaphore, allocator);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_fence(
    VkDevice device, const VkFenceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkFence* fence) {
    if (!g_next_create_fence) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result =
        g_next_create_fence(device, create_info, allocator, fence);
    if (result == VK_SUCCESS && fence && *fence != VK_NULL_HANDLE) {
        teso4m4_render_audit_create_fence(
            *fence, create_info ? create_info->flags : 0);
    }
    record_result("vkCreateFence", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_fence(
    VkDevice device, VkFence fence,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_fence) {
        return;
    }
    teso4m4_render_audit_destroy_fence(fence);
    g_next_destroy_fence(device, fence, allocator);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_reset_fences(
    VkDevice device, uint32_t fence_count, const VkFence* fences) {
    if (!g_next_reset_fences) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_reset_fences(device, fence_count, fences);
    if (result == VK_SUCCESS) {
        teso4m4_render_audit_reset_fences(fence_count, fences);
    }
    record_result("vkResetFences", result);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_wait_for_fences(
    VkDevice device, uint32_t fence_count, const VkFence* fences,
    VkBool32 wait_all, uint64_t timeout) {
    if (!g_next_wait_for_fences) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_wait_for_fences(
        device, fence_count, fences, wait_all, timeout);
    teso4m4_render_audit_wait_for_fences(
        fence_count, fences, result);
    record_result("vkWaitForFences", result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_begin_render_pass(
    VkCommandBuffer buffer,
    const VkRenderPassBeginInfo* begin_info,
    VkSubpassContents contents) {
    if (!g_next_cmd_begin_render_pass) {
        return;
    }
    g_next_cmd_begin_render_pass(buffer, begin_info, contents);
    teso4m4_render_audit_cmd_begin_render_pass(buffer, begin_info);
    count_add(kCmdBeginRenderPass, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_bind_pipeline(
    VkCommandBuffer buffer,
    VkPipelineBindPoint bind_point,
    VkPipeline pipeline) {
    if (!g_next_cmd_bind_pipeline) {
        return;
    }
    g_next_cmd_bind_pipeline(buffer, bind_point, pipeline);
    teso4m4_render_audit_cmd_bind_pipeline(
        buffer, bind_point, pipeline);
    count_add(kCmdBindPipeline, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_bind_descriptor_sets(
    VkCommandBuffer buffer,
    VkPipelineBindPoint bind_point,
    VkPipelineLayout layout,
    uint32_t first_set,
    uint32_t set_count,
    const VkDescriptorSet* sets,
    uint32_t dynamic_offset_count,
    const uint32_t* dynamic_offsets) {
    if (!g_next_cmd_bind_descriptor_sets) {
        return;
    }
    g_next_cmd_bind_descriptor_sets(
        buffer, bind_point, layout, first_set, set_count, sets,
        dynamic_offset_count, dynamic_offsets);
    teso4m4_render_audit_cmd_bind_descriptor_sets(
        buffer, bind_point, first_set, set_count, sets);
    count_add(kCmdBindDescriptorSets, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_draw(
    VkCommandBuffer buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) {
    if (!g_next_cmd_draw) {
        return;
    }
    g_next_cmd_draw(
        buffer, vertex_count, instance_count, first_vertex, first_instance);
    count_add(kCmdDraw, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_draw_indexed(
    VkCommandBuffer buffer,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance) {
    if (!g_next_cmd_draw_indexed) {
        return;
    }
    g_next_cmd_draw_indexed(
        buffer, index_count, instance_count, first_index, vertex_offset,
        first_instance);
    count_add(kCmdDrawIndexed, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_dispatch(
    VkCommandBuffer buffer,
    uint32_t group_count_x,
    uint32_t group_count_y,
    uint32_t group_count_z) {
    if (!g_next_cmd_dispatch) {
        return;
    }
    g_next_cmd_dispatch(
        buffer, group_count_x, group_count_y, group_count_z);
    count_add(kCmdDispatch, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_end_render_pass(
    VkCommandBuffer buffer) {
    if (!g_next_cmd_end_render_pass) {
        return;
    }
    g_next_cmd_end_render_pass(buffer);
    teso4m4_render_audit_cmd_end_render_pass(buffer);
    count_add(kCmdEndRenderPass, 1);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_pipeline_barrier(
    VkCommandBuffer buffer,
    VkPipelineStageFlags source_stage_mask,
    VkPipelineStageFlags destination_stage_mask,
    VkDependencyFlags dependency_flags,
    uint32_t memory_barrier_count,
    const VkMemoryBarrier* memory_barriers,
    uint32_t buffer_memory_barrier_count,
    const VkBufferMemoryBarrier* buffer_memory_barriers,
    uint32_t image_memory_barrier_count,
    const VkImageMemoryBarrier* image_memory_barriers) {
    if (!g_next_cmd_pipeline_barrier) {
        return;
    }
    g_next_cmd_pipeline_barrier(
        buffer, source_stage_mask, destination_stage_mask, dependency_flags,
        memory_barrier_count, memory_barriers, buffer_memory_barrier_count,
        buffer_memory_barriers, image_memory_barrier_count,
        image_memory_barriers);
    teso4m4_render_audit_cmd_pipeline_barrier(
        buffer, source_stage_mask, destination_stage_mask,
        image_memory_barrier_count, image_memory_barriers);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_copy_image(
    VkCommandBuffer buffer,
    VkImage source_image,
    VkImageLayout source_layout,
    VkImage destination_image,
    VkImageLayout destination_layout,
    uint32_t region_count,
    const VkImageCopy* regions) {
    if (!g_next_cmd_copy_image) {
        return;
    }
    g_next_cmd_copy_image(
        buffer, source_image, source_layout, destination_image,
        destination_layout, region_count, regions);
    teso4m4_render_audit_cmd_copy_image();
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_blit_image(
    VkCommandBuffer buffer,
    VkImage source_image,
    VkImageLayout source_layout,
    VkImage destination_image,
    VkImageLayout destination_layout,
    uint32_t region_count,
    const VkImageBlit* regions,
    VkFilter filter) {
    if (!g_next_cmd_blit_image) {
        return;
    }
    g_next_cmd_blit_image(
        buffer, source_image, source_layout, destination_image,
        destination_layout, region_count, regions, filter);
    teso4m4_render_audit_cmd_blit_image();
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_resolve_image(
    VkCommandBuffer buffer,
    VkImage source_image,
    VkImageLayout source_layout,
    VkImage destination_image,
    VkImageLayout destination_layout,
    uint32_t region_count,
    const VkImageResolve* regions) {
    if (!g_next_cmd_resolve_image) {
        return;
    }
    g_next_cmd_resolve_image(
        buffer, source_image, source_layout, destination_image,
        destination_layout, region_count, regions);
    teso4m4_render_audit_cmd_resolve_image();
}

void teso4m4_reset_trace_reset(void) {
    pthread_mutex_lock(&g_state_lock);
    g_next_device_wait_idle = NULL;
    g_next_create_swapchain = NULL;
    g_next_queue_present = NULL;
    g_next_allocate_memory = NULL;
    g_next_free_memory = NULL;
    g_next_map_memory = NULL;
    g_next_unmap_memory = NULL;
    g_next_create_buffer = NULL;
    g_next_destroy_buffer = NULL;
    g_next_create_buffer_view = NULL;
    g_next_destroy_buffer_view = NULL;
    g_next_bind_buffer_memory = NULL;
    g_next_get_buffer_memory_requirements = NULL;
    g_next_create_image = NULL;
    g_next_destroy_image = NULL;
    g_next_bind_image_memory = NULL;
    g_next_get_image_memory_requirements = NULL;
    g_next_create_image_view = NULL;
    g_next_destroy_image_view = NULL;
    g_next_create_sampler = NULL;
    g_next_destroy_sampler = NULL;
    g_next_create_render_pass = NULL;
    g_next_destroy_render_pass = NULL;
    g_next_create_framebuffer = NULL;
    g_next_destroy_framebuffer = NULL;
    g_next_create_descriptor_pool = NULL;
    g_next_destroy_descriptor_pool = NULL;
    g_next_reset_descriptor_pool = NULL;
    g_next_allocate_descriptor_sets = NULL;
    g_next_free_descriptor_sets = NULL;
    g_next_update_descriptor_sets = NULL;
    g_next_create_descriptor_set_layout = NULL;
    g_next_destroy_descriptor_set_layout = NULL;
    g_next_create_pipeline_layout = NULL;
    g_next_destroy_pipeline_layout = NULL;
    g_next_create_graphics_pipelines = NULL;
    g_next_create_compute_pipelines = NULL;
    g_next_destroy_pipeline = NULL;
    g_next_allocate_command_buffers = NULL;
    g_next_free_command_buffers = NULL;
    g_next_reset_command_buffer = NULL;
    g_next_reset_command_pool = NULL;
    g_next_begin_command_buffer = NULL;
    g_next_end_command_buffer = NULL;
    g_next_queue_submit = NULL;
    g_next_acquire_next_image = NULL;
    g_next_create_semaphore = NULL;
    g_next_destroy_semaphore = NULL;
    g_next_create_fence = NULL;
    g_next_destroy_fence = NULL;
    g_next_reset_fences = NULL;
    g_next_wait_for_fences = NULL;
    g_next_cmd_begin_render_pass = NULL;
    g_next_cmd_bind_pipeline = NULL;
    g_next_cmd_bind_descriptor_sets = NULL;
    g_next_cmd_draw = NULL;
    g_next_cmd_draw_indexed = NULL;
    g_next_cmd_dispatch = NULL;
    g_next_cmd_end_render_pass = NULL;
    g_next_cmd_pipeline_barrier = NULL;
    g_next_cmd_copy_image = NULL;
    g_next_cmd_blit_image = NULL;
    g_next_cmd_resolve_image = NULL;
    g_logger = NULL;
    g_wait_count = 0;
    g_swapchain_count = 0;
    g_reset_present_count = 0;
    g_reset_swapchain = VK_NULL_HANDLE;
    atomic_store_explicit(&g_active, false, memory_order_relaxed);
    atomic_store_explicit(&g_complete, false, memory_order_relaxed);
    atomic_store_explicit(
        &g_pipeline_cache_bypass, false, memory_order_relaxed);
    atomic_store_explicit(
        &g_full_lifetime_audit, false, memory_order_relaxed);
    for (size_t index = 0; index < kCounterCount; ++index) {
        atomic_store_explicit(&g_counters[index], 0, memory_order_relaxed);
    }
    atomic_store_explicit(&g_failures, 0, memory_order_relaxed);
    atomic_store_explicit(&g_detail_count, 0, memory_order_relaxed);
    teso4m4_render_audit_reset();
    pthread_mutex_unlock(&g_state_lock);
}

void teso4m4_reset_trace_set_logger(
    Teso4m4ResetTraceLogFunction logger) {
    pthread_mutex_lock(&g_state_lock);
    g_logger = logger;
    teso4m4_render_audit_set_logger(logger);
    pthread_mutex_unlock(&g_state_lock);
}

void teso4m4_reset_trace_set_pipeline_cache_bypass(bool enabled) {
    atomic_store_explicit(
        &g_pipeline_cache_bypass, enabled, memory_order_release);
}

void teso4m4_reset_trace_set_full_lifetime_audit(bool enabled) {
    atomic_store_explicit(
        &g_full_lifetime_audit, enabled, memory_order_release);
    if (enabled) {
        teso4m4_render_audit_enable_mirror();
    }
}

#define INTERCEPT(function_name, next_name, wrapper_name, FunctionType)     \
    else if (strcmp(name, function_name) == 0) {                           \
        next_name = (FunctionType)next_function;                           \
        returned = (PFN_vkVoidFunction)&wrapper_name;                      \
    }

PFN_vkVoidFunction teso4m4_reset_trace_intercept(
    const char* name,
    PFN_vkVoidFunction next_function) {
    if (!name || !next_function) {
        return next_function;
    }
    PFN_vkVoidFunction returned = next_function;
    pthread_mutex_lock(&g_state_lock);
    if (strcmp(name, "vkDeviceWaitIdle") == 0) {
        g_next_device_wait_idle = (PFN_vkDeviceWaitIdle)next_function;
        returned = (PFN_vkVoidFunction)&traced_device_wait_idle;
    }
    INTERCEPT(
        "vkCreateSwapchainKHR", g_next_create_swapchain,
        traced_create_swapchain, PFN_vkCreateSwapchainKHR)
    INTERCEPT(
        "vkQueuePresentKHR", g_next_queue_present,
        traced_queue_present, PFN_vkQueuePresentKHR)
    INTERCEPT(
        "vkAllocateMemory", g_next_allocate_memory,
        traced_allocate_memory, PFN_vkAllocateMemory)
    INTERCEPT(
        "vkFreeMemory", g_next_free_memory,
        traced_free_memory, PFN_vkFreeMemory)
    INTERCEPT(
        "vkMapMemory", g_next_map_memory,
        traced_map_memory, PFN_vkMapMemory)
    INTERCEPT(
        "vkUnmapMemory", g_next_unmap_memory,
        traced_unmap_memory, PFN_vkUnmapMemory)
    INTERCEPT(
        "vkCreateBuffer", g_next_create_buffer,
        traced_create_buffer, PFN_vkCreateBuffer)
    INTERCEPT(
        "vkDestroyBuffer", g_next_destroy_buffer,
        traced_destroy_buffer, PFN_vkDestroyBuffer)
    INTERCEPT(
        "vkCreateBufferView", g_next_create_buffer_view,
        traced_create_buffer_view, PFN_vkCreateBufferView)
    INTERCEPT(
        "vkDestroyBufferView", g_next_destroy_buffer_view,
        traced_destroy_buffer_view, PFN_vkDestroyBufferView)
    INTERCEPT(
        "vkBindBufferMemory", g_next_bind_buffer_memory,
        traced_bind_buffer_memory, PFN_vkBindBufferMemory)
    INTERCEPT(
        "vkGetBufferMemoryRequirements", g_next_get_buffer_memory_requirements,
        traced_get_buffer_memory_requirements,
        PFN_vkGetBufferMemoryRequirements)
    INTERCEPT(
        "vkCreateImage", g_next_create_image,
        traced_create_image, PFN_vkCreateImage)
    INTERCEPT(
        "vkDestroyImage", g_next_destroy_image,
        traced_destroy_image, PFN_vkDestroyImage)
    INTERCEPT(
        "vkBindImageMemory", g_next_bind_image_memory,
        traced_bind_image_memory, PFN_vkBindImageMemory)
    INTERCEPT(
        "vkGetImageMemoryRequirements", g_next_get_image_memory_requirements,
        traced_get_image_memory_requirements,
        PFN_vkGetImageMemoryRequirements)
    INTERCEPT(
        "vkCreateImageView", g_next_create_image_view,
        traced_create_image_view, PFN_vkCreateImageView)
    INTERCEPT(
        "vkDestroyImageView", g_next_destroy_image_view,
        traced_destroy_image_view, PFN_vkDestroyImageView)
    INTERCEPT(
        "vkCreateSampler", g_next_create_sampler,
        traced_create_sampler, PFN_vkCreateSampler)
    INTERCEPT(
        "vkDestroySampler", g_next_destroy_sampler,
        traced_destroy_sampler, PFN_vkDestroySampler)
    INTERCEPT(
        "vkCreateRenderPass", g_next_create_render_pass,
        traced_create_render_pass, PFN_vkCreateRenderPass)
    INTERCEPT(
        "vkDestroyRenderPass", g_next_destroy_render_pass,
        traced_destroy_render_pass, PFN_vkDestroyRenderPass)
    INTERCEPT(
        "vkCreateFramebuffer", g_next_create_framebuffer,
        traced_create_framebuffer, PFN_vkCreateFramebuffer)
    INTERCEPT(
        "vkDestroyFramebuffer", g_next_destroy_framebuffer,
        traced_destroy_framebuffer, PFN_vkDestroyFramebuffer)
    INTERCEPT(
        "vkCreateDescriptorPool", g_next_create_descriptor_pool,
        traced_create_descriptor_pool, PFN_vkCreateDescriptorPool)
    INTERCEPT(
        "vkDestroyDescriptorPool", g_next_destroy_descriptor_pool,
        traced_destroy_descriptor_pool, PFN_vkDestroyDescriptorPool)
    INTERCEPT(
        "vkResetDescriptorPool", g_next_reset_descriptor_pool,
        traced_reset_descriptor_pool, PFN_vkResetDescriptorPool)
    INTERCEPT(
        "vkAllocateDescriptorSets", g_next_allocate_descriptor_sets,
        traced_allocate_descriptor_sets, PFN_vkAllocateDescriptorSets)
    INTERCEPT(
        "vkFreeDescriptorSets", g_next_free_descriptor_sets,
        traced_free_descriptor_sets, PFN_vkFreeDescriptorSets)
    INTERCEPT(
        "vkUpdateDescriptorSets", g_next_update_descriptor_sets,
        traced_update_descriptor_sets, PFN_vkUpdateDescriptorSets)
    INTERCEPT(
        "vkCreateDescriptorSetLayout", g_next_create_descriptor_set_layout,
        traced_create_descriptor_set_layout, PFN_vkCreateDescriptorSetLayout)
    INTERCEPT(
        "vkDestroyDescriptorSetLayout", g_next_destroy_descriptor_set_layout,
        traced_destroy_descriptor_set_layout,
        PFN_vkDestroyDescriptorSetLayout)
    INTERCEPT(
        "vkCreatePipelineLayout", g_next_create_pipeline_layout,
        traced_create_pipeline_layout, PFN_vkCreatePipelineLayout)
    INTERCEPT(
        "vkDestroyPipelineLayout", g_next_destroy_pipeline_layout,
        traced_destroy_pipeline_layout, PFN_vkDestroyPipelineLayout)
    INTERCEPT(
        "vkCreateGraphicsPipelines", g_next_create_graphics_pipelines,
        traced_create_graphics_pipelines, PFN_vkCreateGraphicsPipelines)
    INTERCEPT(
        "vkCreateComputePipelines", g_next_create_compute_pipelines,
        traced_create_compute_pipelines, PFN_vkCreateComputePipelines)
    INTERCEPT(
        "vkDestroyPipeline", g_next_destroy_pipeline,
        traced_destroy_pipeline, PFN_vkDestroyPipeline)
    INTERCEPT(
        "vkAllocateCommandBuffers", g_next_allocate_command_buffers,
        traced_allocate_command_buffers, PFN_vkAllocateCommandBuffers)
    INTERCEPT(
        "vkFreeCommandBuffers", g_next_free_command_buffers,
        traced_free_command_buffers, PFN_vkFreeCommandBuffers)
    INTERCEPT(
        "vkResetCommandBuffer", g_next_reset_command_buffer,
        traced_reset_command_buffer, PFN_vkResetCommandBuffer)
    INTERCEPT(
        "vkResetCommandPool", g_next_reset_command_pool,
        traced_reset_command_pool, PFN_vkResetCommandPool)
    INTERCEPT(
        "vkBeginCommandBuffer", g_next_begin_command_buffer,
        traced_begin_command_buffer, PFN_vkBeginCommandBuffer)
    INTERCEPT(
        "vkEndCommandBuffer", g_next_end_command_buffer,
        traced_end_command_buffer, PFN_vkEndCommandBuffer)
    INTERCEPT(
        "vkQueueSubmit", g_next_queue_submit,
        traced_queue_submit, PFN_vkQueueSubmit)
    INTERCEPT(
        "vkAcquireNextImageKHR", g_next_acquire_next_image,
        traced_acquire_next_image, PFN_vkAcquireNextImageKHR)
    INTERCEPT(
        "vkCreateSemaphore", g_next_create_semaphore,
        traced_create_semaphore, PFN_vkCreateSemaphore)
    INTERCEPT(
        "vkDestroySemaphore", g_next_destroy_semaphore,
        traced_destroy_semaphore, PFN_vkDestroySemaphore)
    INTERCEPT(
        "vkCreateFence", g_next_create_fence,
        traced_create_fence, PFN_vkCreateFence)
    INTERCEPT(
        "vkDestroyFence", g_next_destroy_fence,
        traced_destroy_fence, PFN_vkDestroyFence)
    INTERCEPT(
        "vkResetFences", g_next_reset_fences,
        traced_reset_fences, PFN_vkResetFences)
    INTERCEPT(
        "vkWaitForFences", g_next_wait_for_fences,
        traced_wait_for_fences, PFN_vkWaitForFences)
    INTERCEPT(
        "vkCmdBeginRenderPass", g_next_cmd_begin_render_pass,
        traced_cmd_begin_render_pass, PFN_vkCmdBeginRenderPass)
    INTERCEPT(
        "vkCmdBindPipeline", g_next_cmd_bind_pipeline,
        traced_cmd_bind_pipeline, PFN_vkCmdBindPipeline)
    INTERCEPT(
        "vkCmdBindDescriptorSets", g_next_cmd_bind_descriptor_sets,
        traced_cmd_bind_descriptor_sets, PFN_vkCmdBindDescriptorSets)
    INTERCEPT(
        "vkCmdDraw", g_next_cmd_draw,
        traced_cmd_draw, PFN_vkCmdDraw)
    INTERCEPT(
        "vkCmdDrawIndexed", g_next_cmd_draw_indexed,
        traced_cmd_draw_indexed, PFN_vkCmdDrawIndexed)
    INTERCEPT(
        "vkCmdDispatch", g_next_cmd_dispatch,
        traced_cmd_dispatch, PFN_vkCmdDispatch)
    INTERCEPT(
        "vkCmdEndRenderPass", g_next_cmd_end_render_pass,
        traced_cmd_end_render_pass, PFN_vkCmdEndRenderPass)
    INTERCEPT(
        "vkCmdPipelineBarrier", g_next_cmd_pipeline_barrier,
        traced_cmd_pipeline_barrier, PFN_vkCmdPipelineBarrier)
    INTERCEPT(
        "vkCmdCopyImage", g_next_cmd_copy_image,
        traced_cmd_copy_image, PFN_vkCmdCopyImage)
    INTERCEPT(
        "vkCmdBlitImage", g_next_cmd_blit_image,
        traced_cmd_blit_image, PFN_vkCmdBlitImage)
    INTERCEPT(
        "vkCmdResolveImage", g_next_cmd_resolve_image,
        traced_cmd_resolve_image, PFN_vkCmdResolveImage)
    pthread_mutex_unlock(&g_state_lock);
    return returned;
}
