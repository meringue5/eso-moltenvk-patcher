#include "mvk_lifecycle.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    kMaxSwapchains = 32,
    kMaxSwapchainImages = 256,
    kMaxImageViews = 8192,
    kMaxRenderPasses = 512,
    kMaxFramebuffers = 512,
    kMaxCommandBuffers = 512,
    kMaxSignaledSemaphores = 512,
    kMaxShaderModules = 4096,
    kMaxGraphicsPipelines = 4096,
    kMaxDescriptorSetLayouts = 2048,
    kMaxPipelineLayouts = 2048,
    kMaxDescriptorSets = 131072,
    kMaxBoundDescriptorSets = 16,
    kMaxTrackedImageBindings = 2,
    kMaxDrawPipelines = 8,
    kMaxRenderPassAttachments = 16,
    kFirstPresentationLimit = 8,
    kStartupAuditGenerationLimit = 2,
    kStartupAuditGeneration2PresentLimit = 180,
    kStartupAuditDetailLimit = 2048,
    kCompositorNeutralizeFirstPresent = 60,
    kCompositorNeutralizeLastPresent = 150,
    kCompositorNeutralizeMaxSuppressedDraws = 96,
};

/* Experiment 0028 identities for ESO 12.0.7's final scene/GUI compositor. */
static const uint64_t kCompositorPipelineSignature =
    UINT64_C(0xc43e4410d3b33fe7);
static const uint64_t kCompositorPipelineLayoutSignature =
    UINT64_C(0xd175d2c1daed112d);
static const uint64_t kCompositorSetLayoutSignatures[] = {
    UINT64_C(0xe3c2499a89df1706),
    UINT64_C(0xd0edad262f8c4230),
};

typedef struct {
    VkSwapchainKHR handle;
    uint64_t generation;
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t acquire_count;
    uint32_t present_count;
    bool alive;
} SwapchainRecord;

typedef struct {
    VkImage handle;
    VkSwapchainKHR swapchain;
    uint64_t generation;
    uint32_t index;
    bool alive;
} ImageRecord;

typedef struct {
    uint32_t binding;
    uint32_t array_element;
    VkImageView view;
    VkSampler sampler;
    VkImageLayout layout;
    VkImage image;
    VkFormat format;
    VkImageViewType view_type;
    uint32_t base_mip_level;
    uint32_t base_array_layer;
    uint64_t signature;
    uint64_t update_call;
    bool valid;
} DescriptorImageBindingState;

typedef struct {
    VkSemaphore handle;
    VkQueue queue;
    uint64_t generation;
    VkCommandBuffer command_buffer;
    VkFramebuffer framebuffer;
    uint32_t tracked_command_count;
    uint32_t draw_count;
    uint32_t indexed_draw_count;
    uint32_t distinct_pipeline_count;
    uint64_t pipeline_signatures[kMaxDrawPipelines];
    uint64_t draw_signature;
    uint64_t first_pipeline_signature;
    uint64_t last_pipeline_signature;
    bool pipeline_overflow;
    uint32_t bound_set_count;
    uint64_t descriptor_layout_signature;
    uint64_t descriptor_handle_signature;
    uint64_t descriptor_update_signature;
    uint64_t push_constant_signature;
    uint32_t push_constant_bytes;
    bool input_complete;
    uint64_t descriptor_set_layout_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_calls[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_calls[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_counts[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_expected_image_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_expected_buffer_counts[kMaxBoundDescriptorSets];
    bool descriptor_class_complete;
    DescriptorImageBindingState descriptor_images
        [kMaxBoundDescriptorSets][kMaxTrackedImageBindings];
    bool occupied;
} SignaledSemaphoreRecord;

typedef struct {
    VkShaderModule handle;
    uint64_t code_hash;
    size_t code_size;
    bool alive;
} ShaderModuleRecord;

typedef struct {
    VkDescriptorSetLayout handle;
    uint64_t signature;
    uint32_t binding_count;
    uint32_t descriptor_count;
    uint32_t image_count;
    uint32_t buffer_count;
    uint32_t sampler_count;
    uint32_t input_attachment_count;
    bool alive;
} DescriptorSetLayoutRecord;

typedef struct {
    VkPipelineLayout handle;
    uint64_t signature;
    uint32_t set_layout_count;
    uint64_t set_layout_signatures[kMaxBoundDescriptorSets];
    uint32_t set_descriptor_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_count;
    uint32_t image_count;
    uint32_t buffer_count;
    uint32_t sampler_count;
    uint32_t input_attachment_count;
    uint32_t push_constant_bytes;
    bool complete;
    bool alive;
} PipelineLayoutRecord;

typedef struct {
    VkDescriptorSet handle;
    VkDescriptorPool pool;
    uint64_t layout_signature;
    uint64_t update_signature;
    uint64_t update_count;
    uint64_t last_update_call;
    uint64_t image_update_signature;
    uint64_t image_update_count;
    uint64_t last_image_update_call;
    uint64_t buffer_update_signature;
    uint64_t buffer_update_count;
    uint64_t last_buffer_update_call;
    uint32_t expected_image_count;
    uint32_t expected_buffer_count;
    bool class_complete;
    DescriptorImageBindingState images[kMaxTrackedImageBindings];
    bool occupied;
    bool alive;
} DescriptorSetRecord;

typedef struct {
    VkPipeline handle;
    uint64_t signature;
    uint64_t vertex_shader_hash;
    uint64_t fragment_shader_hash;
    VkPipelineLayout layout;
    uint64_t layout_signature;
    uint32_t set_layout_count;
    uint64_t set_layout_signatures[kMaxBoundDescriptorSets];
    uint32_t set_descriptor_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_count;
    uint32_t image_count;
    uint32_t buffer_count;
    uint32_t sampler_count;
    uint32_t input_attachment_count;
    uint32_t push_constant_bytes;
    VkRenderPass render_pass;
    uint32_t subpass;
    bool shader_hash_complete;
    bool layout_complete;
    bool alive;
} GraphicsPipelineRecord;

typedef struct {
    uint64_t generation;
    VkCommandBuffer command_buffer;
    VkFramebuffer framebuffer;
    uint32_t tracked_command_count;
    uint32_t draw_count;
    uint32_t indexed_draw_count;
    uint32_t distinct_pipeline_count;
    uint64_t pipeline_signatures[kMaxDrawPipelines];
    uint64_t draw_signature;
    uint64_t first_pipeline_signature;
    uint64_t last_pipeline_signature;
    bool pipeline_overflow;
    uint32_t bound_set_count;
    uint64_t descriptor_layout_signature;
    uint64_t descriptor_handle_signature;
    uint64_t descriptor_update_signature;
    uint64_t push_constant_signature;
    uint32_t push_constant_bytes;
    bool input_complete;
    uint64_t descriptor_set_layout_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_signatures[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_calls[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_calls[kMaxBoundDescriptorSets];
    uint64_t descriptor_image_update_counts[kMaxBoundDescriptorSets];
    uint64_t descriptor_buffer_update_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_expected_image_counts[kMaxBoundDescriptorSets];
    uint32_t descriptor_expected_buffer_counts[kMaxBoundDescriptorSets];
    bool descriptor_class_complete;
    DescriptorImageBindingState descriptor_images
        [kMaxBoundDescriptorSets][kMaxTrackedImageBindings];
} DrawSubmissionSummary;

typedef struct {
    VkImageView handle;
    uint64_t generation;
    VkImage image;
    VkFormat format;
    VkImageViewType view_type;
    uint32_t base_mip_level;
    uint32_t base_array_layer;
    bool alive;
} ImageViewRecord;

typedef struct {
    VkRenderPass handle;
    uint32_t attachment_count;
    VkFormat attachment_formats[kMaxRenderPassAttachments];
    VkAttachmentLoadOp attachment_load_ops[kMaxRenderPassAttachments];
    uint64_t first_generation;
    uint64_t last_generation;
    uint32_t link_count;
    bool alive;
} RenderPassRecord;

typedef struct {
    VkFramebuffer handle;
    VkRenderPass render_pass;
    uint64_t generation;
    uint32_t width;
    uint32_t height;
    bool alive;
} FramebufferRecord;

typedef struct {
    VkCommandBuffer handle;
    VkFramebuffer framebuffer;
    VkRenderPass render_pass;
    uint64_t generation;
    bool occupied;
    bool in_render_pass;
    uint32_t draw_count;
    uint32_t indexed_draw_count;
    uint32_t distinct_pipeline_count;
    uint64_t pipeline_signatures[kMaxDrawPipelines];
    uint64_t draw_signature;
    uint64_t first_pipeline_signature;
    uint64_t last_pipeline_signature;
    VkPipeline bound_pipeline;
    uint64_t bound_pipeline_signature;
    uint64_t bound_pipeline_layout_signature;
    uint32_t bound_pipeline_set_layout_count;
    uint64_t
        bound_pipeline_set_layout_signatures[kMaxBoundDescriptorSets];
    uint32_t
        bound_pipeline_set_descriptor_counts[kMaxBoundDescriptorSets];
    uint32_t bound_pipeline_push_constant_bytes;
    bool bound_pipeline_layout_complete;
    bool pipeline_overflow;
    VkDescriptorSet bound_sets[kMaxBoundDescriptorSets];
    uint32_t bound_set_count;
    uint64_t dynamic_offset_signature;
    uint64_t push_constant_signature;
    uint32_t push_constant_bytes;
    uint32_t push_constant_calls;
    uint64_t draw_descriptor_layout_signature;
    uint64_t draw_descriptor_handle_signature;
    uint64_t draw_descriptor_update_signature;
    uint64_t draw_push_constant_signature;
    uint32_t draw_push_constant_bytes;
    uint32_t draw_bound_set_count;
    bool draw_input_complete;
    uint64_t draw_set_layout_signatures[kMaxBoundDescriptorSets];
    uint64_t draw_set_image_update_signatures[kMaxBoundDescriptorSets];
    uint64_t draw_set_buffer_update_signatures[kMaxBoundDescriptorSets];
    uint64_t draw_set_image_update_calls[kMaxBoundDescriptorSets];
    uint64_t draw_set_buffer_update_calls[kMaxBoundDescriptorSets];
    uint64_t draw_set_image_update_counts[kMaxBoundDescriptorSets];
    uint64_t draw_set_buffer_update_counts[kMaxBoundDescriptorSets];
    uint32_t draw_set_expected_image_counts[kMaxBoundDescriptorSets];
    uint32_t draw_set_expected_buffer_counts[kMaxBoundDescriptorSets];
    bool draw_descriptor_class_complete;
    DescriptorImageBindingState draw_descriptor_images
        [kMaxBoundDescriptorSets][kMaxTrackedImageBindings];
} CommandBufferRecord;

static PFN_vkDeviceWaitIdle g_next_device_wait_idle;
static PFN_vkCreateSwapchainKHR g_next_create_swapchain;
static PFN_vkDestroySwapchainKHR g_next_destroy_swapchain;
static PFN_vkGetSwapchainImagesKHR g_next_get_swapchain_images;
static PFN_vkCreateImageView g_next_create_image_view;
static PFN_vkDestroyImageView g_next_destroy_image_view;
static PFN_vkCreateRenderPass g_next_create_render_pass;
static PFN_vkDestroyRenderPass g_next_destroy_render_pass;
static PFN_vkCreateFramebuffer g_next_create_framebuffer;
static PFN_vkDestroyFramebuffer g_next_destroy_framebuffer;
static PFN_vkAcquireNextImageKHR g_next_acquire_next_image;
static PFN_vkQueuePresentKHR g_next_queue_present;
static PFN_vkQueueSubmit g_next_queue_submit;
static PFN_vkCmdBeginRenderPass g_next_cmd_begin_render_pass;
static PFN_vkCmdEndRenderPass g_next_cmd_end_render_pass;
static PFN_vkCmdClearAttachments g_next_cmd_clear_attachments;
static PFN_vkBeginCommandBuffer g_next_begin_command_buffer;
static PFN_vkCreateShaderModule g_next_create_shader_module;
static PFN_vkDestroyShaderModule g_next_destroy_shader_module;
static PFN_vkCreateGraphicsPipelines g_next_create_graphics_pipelines;
static PFN_vkDestroyPipeline g_next_destroy_pipeline;
static PFN_vkCmdBindPipeline g_next_cmd_bind_pipeline;
static PFN_vkCmdDraw g_next_cmd_draw;
static PFN_vkCmdDrawIndexed g_next_cmd_draw_indexed;
static PFN_vkCreateDescriptorSetLayout g_next_create_descriptor_set_layout;
static PFN_vkDestroyDescriptorSetLayout g_next_destroy_descriptor_set_layout;
static PFN_vkCreatePipelineLayout g_next_create_pipeline_layout;
static PFN_vkDestroyPipelineLayout g_next_destroy_pipeline_layout;
static PFN_vkAllocateDescriptorSets g_next_allocate_descriptor_sets;
static PFN_vkFreeDescriptorSets g_next_free_descriptor_sets;
static PFN_vkResetDescriptorPool g_next_reset_descriptor_pool;
static PFN_vkDestroyDescriptorPool g_next_destroy_descriptor_pool;
static PFN_vkUpdateDescriptorSets g_next_update_descriptor_sets;
static PFN_vkCmdBindDescriptorSets g_next_cmd_bind_descriptor_sets;
static PFN_vkCmdPushConstants g_next_cmd_push_constants;

static Teso4m4LifecycleLogFunction g_logger;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static SwapchainRecord g_swapchains[kMaxSwapchains];
static ImageRecord g_images[kMaxSwapchainImages];
static ImageViewRecord g_image_views[kMaxImageViews];
static RenderPassRecord g_render_passes[kMaxRenderPasses];
static FramebufferRecord g_framebuffers[kMaxFramebuffers];
static CommandBufferRecord g_command_buffers[kMaxCommandBuffers];
static SignaledSemaphoreRecord g_signaled_semaphores[kMaxSignaledSemaphores];
static ShaderModuleRecord g_shader_modules[kMaxShaderModules];
static GraphicsPipelineRecord g_graphics_pipelines[kMaxGraphicsPipelines];
static DescriptorSetLayoutRecord
    g_descriptor_set_layouts[kMaxDescriptorSetLayouts];
static PipelineLayoutRecord g_pipeline_layouts[kMaxPipelineLayouts];
static DescriptorSetRecord g_descriptor_sets[kMaxDescriptorSets];
static uint64_t g_generation_counter;
static uint64_t g_wait_counter;
static uint64_t g_descriptor_update_call_counter;
static bool g_overflow_reported;
static bool g_enabled = true;
static atomic_bool g_startup_color_audit;
static atomic_bool g_startup_present_pixel_audit;
static atomic_bool g_startup_draw_audit;
static atomic_bool g_startup_input_audit;
static atomic_bool g_startup_compositor_audit;
static atomic_bool g_startup_compositor_neutralize;
static atomic_bool g_startup_color_audit_finished;
static atomic_uint g_startup_color_detail_count;
static Teso4m4PresentPixelSampler g_present_pixel_sampler;
static Teso4m4CompositorImageSampler g_compositor_image_sampler;
typedef enum {
    TESO4M4_COMPOSITOR_NEUTRALIZE_INACTIVE = 0,
    TESO4M4_COMPOSITOR_NEUTRALIZE_ARMED,
    TESO4M4_COMPOSITOR_NEUTRALIZE_SUPPRESSING,
    TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING,
    TESO4M4_COMPOSITOR_NEUTRALIZE_ABORTED,
} CompositorNeutralizeState;
static CompositorNeutralizeState g_compositor_neutralize_state;
static uint64_t g_compositor_placeholder_signature;
static uint32_t g_compositor_suppressed_draw_count;
static VkPipeline g_compositor_neutralize_test_pipeline;
static bool g_reset_has_run;

static void lifecycle_log(const char* format, ...) {
    if (!g_logger ||
        (atomic_load(&g_startup_color_audit) &&
         atomic_load(&g_startup_color_audit_finished))) {
        return;
    }
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    g_logger(message);
}

static void startup_color_detail_log(const char* format, ...) {
    const unsigned int ordinal =
        atomic_fetch_add(&g_startup_color_detail_count, 1);
    if (ordinal >= kStartupAuditDetailLimit) {
        if (ordinal == kStartupAuditDetailLimit) {
            lifecycle_log(
                "STARTUP_COLOR_DETAIL_LIMIT: retained=%u",
                kStartupAuditDetailLimit);
        }
        return;
    }
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    lifecycle_log("%s", message);
}

static void report_overflow(const char* resource) {
    if (!g_overflow_reported) {
        g_overflow_reported = true;
        lifecycle_log("LIFECYCLE_ERROR: tracking capacity exhausted resource=%s",
                      resource);
    }
}

static SwapchainRecord* find_swapchain(VkSwapchainKHR handle) {
    for (size_t index = 0; index < kMaxSwapchains; ++index) {
        if (g_swapchains[index].alive &&
            g_swapchains[index].handle == handle) {
            return &g_swapchains[index];
        }
    }
    return NULL;
}

static ImageRecord* find_image(VkImage handle) {
    for (size_t index = 0; index < kMaxSwapchainImages; ++index) {
        if (g_images[index].alive && g_images[index].handle == handle) {
            return &g_images[index];
        }
    }
    return NULL;
}

static ImageRecord* find_swapchain_image(
    VkSwapchainKHR swapchain,
    uint32_t image_index) {
    for (size_t index = 0; index < kMaxSwapchainImages; ++index) {
        if (g_images[index].alive &&
            g_images[index].swapchain == swapchain &&
            g_images[index].index == image_index) {
            return &g_images[index];
        }
    }
    return NULL;
}

static SignaledSemaphoreRecord* find_signaled_semaphore(
    VkSemaphore handle) {
    for (size_t index = 0; index < kMaxSignaledSemaphores; ++index) {
        if (g_signaled_semaphores[index].occupied &&
            g_signaled_semaphores[index].handle == handle) {
            return &g_signaled_semaphores[index];
        }
    }
    return NULL;
}

static void remember_signaled_semaphore(
    VkSemaphore handle,
    VkQueue queue,
    const DrawSubmissionSummary* summary) {
    SignaledSemaphoreRecord* existing = find_signaled_semaphore(handle);
    if (!existing) {
        for (size_t index = 0; index < kMaxSignaledSemaphores; ++index) {
            if (!g_signaled_semaphores[index].occupied) {
                existing = &g_signaled_semaphores[index];
                break;
            }
        }
    }
    if (!existing) {
        report_overflow("signaled-semaphore");
        return;
    }
    memset(existing, 0, sizeof(*existing));
    existing->handle = handle;
    existing->queue = queue;
    existing->occupied = true;
    if (summary) {
        existing->generation = summary->generation;
        existing->command_buffer = summary->command_buffer;
        existing->framebuffer = summary->framebuffer;
        existing->tracked_command_count = summary->tracked_command_count;
        existing->draw_count = summary->draw_count;
        existing->indexed_draw_count = summary->indexed_draw_count;
        existing->distinct_pipeline_count =
            summary->distinct_pipeline_count;
        memcpy(
            existing->pipeline_signatures, summary->pipeline_signatures,
            sizeof(existing->pipeline_signatures));
        existing->draw_signature = summary->draw_signature;
        existing->first_pipeline_signature =
            summary->first_pipeline_signature;
        existing->last_pipeline_signature =
            summary->last_pipeline_signature;
        existing->pipeline_overflow = summary->pipeline_overflow;
        existing->bound_set_count = summary->bound_set_count;
        existing->descriptor_layout_signature =
            summary->descriptor_layout_signature;
        existing->descriptor_handle_signature =
            summary->descriptor_handle_signature;
        existing->descriptor_update_signature =
            summary->descriptor_update_signature;
        existing->push_constant_signature =
            summary->push_constant_signature;
        existing->push_constant_bytes = summary->push_constant_bytes;
        existing->input_complete = summary->input_complete;
        memcpy(
            existing->descriptor_set_layout_signatures,
            summary->descriptor_set_layout_signatures,
            sizeof(existing->descriptor_set_layout_signatures));
        memcpy(
            existing->descriptor_image_update_signatures,
            summary->descriptor_image_update_signatures,
            sizeof(existing->descriptor_image_update_signatures));
        memcpy(
            existing->descriptor_buffer_update_signatures,
            summary->descriptor_buffer_update_signatures,
            sizeof(existing->descriptor_buffer_update_signatures));
        memcpy(
            existing->descriptor_image_update_calls,
            summary->descriptor_image_update_calls,
            sizeof(existing->descriptor_image_update_calls));
        memcpy(
            existing->descriptor_buffer_update_calls,
            summary->descriptor_buffer_update_calls,
            sizeof(existing->descriptor_buffer_update_calls));
        memcpy(
            existing->descriptor_image_update_counts,
            summary->descriptor_image_update_counts,
            sizeof(existing->descriptor_image_update_counts));
        memcpy(
            existing->descriptor_buffer_update_counts,
            summary->descriptor_buffer_update_counts,
            sizeof(existing->descriptor_buffer_update_counts));
        memcpy(
            existing->descriptor_expected_image_counts,
            summary->descriptor_expected_image_counts,
            sizeof(existing->descriptor_expected_image_counts));
        memcpy(
            existing->descriptor_expected_buffer_counts,
            summary->descriptor_expected_buffer_counts,
            sizeof(existing->descriptor_expected_buffer_counts));
        existing->descriptor_class_complete =
            summary->descriptor_class_complete;
        memcpy(
            existing->descriptor_images,
            summary->descriptor_images,
            sizeof(existing->descriptor_images));
    }
}

static void forget_signaled_semaphore(VkSemaphore handle) {
    SignaledSemaphoreRecord* existing = find_signaled_semaphore(handle);
    if (existing) {
        existing->occupied = false;
    }
}

static ImageViewRecord* find_image_view(VkImageView handle) {
    for (size_t index = 0; index < kMaxImageViews; ++index) {
        if (g_image_views[index].alive &&
            g_image_views[index].handle == handle) {
            return &g_image_views[index];
        }
    }
    return NULL;
}

static RenderPassRecord* find_render_pass(VkRenderPass handle) {
    for (size_t index = 0; index < kMaxRenderPasses; ++index) {
        if (g_render_passes[index].alive &&
            g_render_passes[index].handle == handle) {
            return &g_render_passes[index];
        }
    }
    return NULL;
}

static FramebufferRecord* find_framebuffer(VkFramebuffer handle) {
    for (size_t index = 0; index < kMaxFramebuffers; ++index) {
        if (g_framebuffers[index].alive &&
            g_framebuffers[index].handle == handle) {
            return &g_framebuffers[index];
        }
    }
    return NULL;
}

static SwapchainRecord* add_swapchain(
    VkSwapchainKHR handle,
    uint64_t generation,
    const VkSwapchainCreateInfoKHR* create_info) {
    for (size_t index = 0; index < kMaxSwapchains; ++index) {
        if (!g_swapchains[index].alive) {
            g_swapchains[index] = (SwapchainRecord){
                .handle = handle,
                .generation = generation,
                .format = create_info ? create_info->imageFormat : 0,
                .width = create_info ? create_info->imageExtent.width : 0,
                .height = create_info ? create_info->imageExtent.height : 0,
                .alive = true,
            };
            return &g_swapchains[index];
        }
    }
    report_overflow("swapchain");
    return NULL;
}

static void add_image(
    VkImage handle,
    VkSwapchainKHR swapchain,
    uint64_t generation,
    uint32_t image_index) {
    ImageRecord* existing = find_image(handle);
    if (existing) {
        existing->generation = generation;
        existing->swapchain = swapchain;
        existing->index = image_index;
        return;
    }
    for (size_t index = 0; index < kMaxSwapchainImages; ++index) {
        if (!g_images[index].alive) {
            g_images[index] = (ImageRecord){
                .handle = handle,
                .swapchain = swapchain,
                .generation = generation,
                .index = image_index,
                .alive = true,
            };
            return;
        }
    }
    report_overflow("swapchain-image");
}

static void add_image_view(
    VkImageView handle,
    uint64_t generation,
    const VkImageViewCreateInfo* create_info) {
    for (size_t index = 0; index < kMaxImageViews; ++index) {
        if (!g_image_views[index].alive) {
            g_image_views[index] = (ImageViewRecord){
                .handle = handle,
                .generation = generation,
                .image = create_info ? create_info->image : VK_NULL_HANDLE,
                .format = create_info ? create_info->format : VK_FORMAT_UNDEFINED,
                .view_type = create_info
                    ? create_info->viewType
                    : VK_IMAGE_VIEW_TYPE_2D,
                .base_mip_level = create_info
                    ? create_info->subresourceRange.baseMipLevel
                    : 0,
                .base_array_layer = create_info
                    ? create_info->subresourceRange.baseArrayLayer
                    : 0,
                .alive = true,
            };
            return;
        }
    }
    report_overflow("image-view");
}

static void add_render_pass(
    VkRenderPass handle,
    const VkRenderPassCreateInfo* create_info) {
    for (size_t index = 0; index < kMaxRenderPasses; ++index) {
        if (!g_render_passes[index].alive) {
            g_render_passes[index] = (RenderPassRecord){
                .handle = handle,
                .alive = true,
            };
            if (create_info) {
                const uint32_t count =
                    create_info->attachmentCount < kMaxRenderPassAttachments
                        ? create_info->attachmentCount
                        : kMaxRenderPassAttachments;
                g_render_passes[index].attachment_count = count;
                for (uint32_t attachment = 0; attachment < count;
                     ++attachment) {
                    g_render_passes[index].attachment_formats[attachment] =
                        create_info->pAttachments[attachment].format;
                    g_render_passes[index].attachment_load_ops[attachment] =
                        create_info->pAttachments[attachment].loadOp;
                }
            }
            return;
        }
    }
    report_overflow("render-pass");
}

static void add_framebuffer(
    VkFramebuffer handle,
    VkRenderPass render_pass,
    uint64_t generation,
    uint32_t width,
    uint32_t height) {
    for (size_t index = 0; index < kMaxFramebuffers; ++index) {
        if (!g_framebuffers[index].alive) {
            g_framebuffers[index] = (FramebufferRecord){
                .handle = handle,
                .render_pass = render_pass,
                .generation = generation,
                .width = width,
                .height = height,
                .alive = true,
            };
            return;
        }
    }
    report_overflow("framebuffer");
}

static CommandBufferRecord* find_command_buffer(VkCommandBuffer handle) {
    for (size_t index = 0; index < kMaxCommandBuffers; ++index) {
        if (g_command_buffers[index].occupied &&
            g_command_buffers[index].handle == handle) {
            return &g_command_buffers[index];
        }
    }
    return NULL;
}

static uint64_t hash_bytes(const void* bytes, size_t size) {
    const uint8_t* cursor = bytes;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t index = 0; cursor && index < size; ++index) {
        hash ^= cursor[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t hash_mix(uint64_t hash, uint64_t value) {
    for (size_t byte = 0; byte < sizeof(value); ++byte) {
        hash ^= (value >> (byte * 8)) & UINT64_C(0xff);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool descriptor_type_is_image(VkDescriptorType type) {
    return type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
}

static bool descriptor_type_is_image_class(VkDescriptorType type) {
    return descriptor_type_is_image(type) ||
           type == VK_DESCRIPTOR_TYPE_SAMPLER;
}

static bool descriptor_type_is_buffer(VkDescriptorType type) {
    return type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
           type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
}

static DescriptorSetLayoutRecord* find_descriptor_set_layout(
    VkDescriptorSetLayout handle) {
    for (size_t index = 0; index < kMaxDescriptorSetLayouts; ++index) {
        if (g_descriptor_set_layouts[index].alive &&
            g_descriptor_set_layouts[index].handle == handle) {
            return &g_descriptor_set_layouts[index];
        }
    }
    return NULL;
}

static DescriptorSetLayoutRecord* add_descriptor_set_layout(
    VkDescriptorSetLayout handle) {
    DescriptorSetLayoutRecord* existing = find_descriptor_set_layout(handle);
    if (existing) {
        return existing;
    }
    for (size_t index = 0; index < kMaxDescriptorSetLayouts; ++index) {
        if (!g_descriptor_set_layouts[index].alive) {
            g_descriptor_set_layouts[index] = (DescriptorSetLayoutRecord){
                .handle = handle,
                .alive = true,
            };
            return &g_descriptor_set_layouts[index];
        }
    }
    report_overflow("descriptor-set-layout");
    return NULL;
}

static PipelineLayoutRecord* find_pipeline_layout(VkPipelineLayout handle) {
    for (size_t index = 0; index < kMaxPipelineLayouts; ++index) {
        if (g_pipeline_layouts[index].alive &&
            g_pipeline_layouts[index].handle == handle) {
            return &g_pipeline_layouts[index];
        }
    }
    return NULL;
}

static PipelineLayoutRecord* add_pipeline_layout(VkPipelineLayout handle) {
    PipelineLayoutRecord* existing = find_pipeline_layout(handle);
    if (existing) {
        return existing;
    }
    for (size_t index = 0; index < kMaxPipelineLayouts; ++index) {
        if (!g_pipeline_layouts[index].alive) {
            g_pipeline_layouts[index] = (PipelineLayoutRecord){
                .handle = handle,
                .alive = true,
            };
            return &g_pipeline_layouts[index];
        }
    }
    report_overflow("pipeline-layout");
    return NULL;
}

static size_t descriptor_set_start(VkDescriptorSet handle) {
    uintptr_t value = (uintptr_t)handle;
    value ^= value >> 17;
    value *= UINT64_C(0xed5ad4bb);
    value ^= value >> 11;
    return (size_t)value & (kMaxDescriptorSets - 1);
}

static DescriptorSetRecord* find_descriptor_set(
    VkDescriptorSet handle,
    bool create) {
    const size_t start = descriptor_set_start(handle);
    DescriptorSetRecord* reusable = NULL;
    for (size_t probe = 0; probe < kMaxDescriptorSets; ++probe) {
        DescriptorSetRecord* record =
            &g_descriptor_sets[(start + probe) & (kMaxDescriptorSets - 1)];
        if (record->occupied && record->handle == handle) {
            return record;
        }
        if (!record->occupied) {
            if (!create) {
                return NULL;
            }
            reusable = record;
            break;
        }
        if (!record->alive && !reusable) {
            reusable = record;
        }
    }
    if (!create || !reusable) {
        if (create) {
            report_overflow("descriptor-set");
        }
        return NULL;
    }
    *reusable = (DescriptorSetRecord){
        .handle = handle,
        .occupied = true,
        .alive = true,
    };
    return reusable;
}

static uint64_t descriptor_write_signature(
    const VkWriteDescriptorSet* write) {
    uint64_t hash = UINT64_C(1469598103934665603);
    if (!write) {
        return 0;
    }
    hash = hash_mix(hash, write->dstBinding);
    hash = hash_mix(hash, write->dstArrayElement);
    hash = hash_mix(hash, write->descriptorCount);
    hash = hash_mix(hash, write->descriptorType);
    for (uint32_t index = 0; index < write->descriptorCount; ++index) {
        if ((descriptor_type_is_image(write->descriptorType) ||
             write->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) &&
            write->pImageInfo) {
            hash = hash_mix(
                hash, (uint64_t)(uintptr_t)write->pImageInfo[index].sampler);
            hash = hash_mix(
                hash, (uint64_t)(uintptr_t)write->pImageInfo[index].imageView);
            hash = hash_mix(hash, write->pImageInfo[index].imageLayout);
        } else if (descriptor_type_is_buffer(write->descriptorType) &&
                   write->pBufferInfo) {
            hash = hash_mix(
                hash, (uint64_t)(uintptr_t)write->pBufferInfo[index].buffer);
            hash = hash_mix(hash, write->pBufferInfo[index].offset);
            hash = hash_mix(hash, write->pBufferInfo[index].range);
        } else if (write->pTexelBufferView) {
            hash = hash_mix(
                hash,
                (uint64_t)(uintptr_t)write->pTexelBufferView[index]);
        } else {
            hash = hash_mix(hash, UINT64_MAX);
        }
    }
    return hash;
}

static void update_descriptor_image_bindings(
    DescriptorSetRecord* record,
    const VkWriteDescriptorSet* write,
    uint64_t update_call) {
    if (!record || !write || !descriptor_type_is_image(write->descriptorType) ||
        !write->pImageInfo) {
        return;
    }
    for (uint32_t element = 0; element < write->descriptorCount; ++element) {
        const uint32_t array_element = write->dstArrayElement + element;
        DescriptorImageBindingState* state = NULL;
        for (uint32_t index = 0; index < kMaxTrackedImageBindings; ++index) {
            DescriptorImageBindingState* candidate = &record->images[index];
            if (candidate->valid && candidate->binding == write->dstBinding &&
                candidate->array_element == array_element) {
                state = candidate;
                break;
            }
            if (!candidate->valid && !state) {
                state = candidate;
            }
        }
        if (!state) {
            record->class_complete = false;
            continue;
        }
        const VkDescriptorImageInfo* image = &write->pImageInfo[element];
        ImageViewRecord* view = find_image_view(image->imageView);
        uint64_t signature = UINT64_C(1469598103934665603);
        signature = hash_mix(signature, write->dstBinding);
        signature = hash_mix(signature, array_element);
        signature = hash_mix(signature, (uint64_t)(uintptr_t)image->sampler);
        signature = hash_mix(signature, (uint64_t)(uintptr_t)image->imageView);
        signature = hash_mix(signature, image->imageLayout);
        *state = (DescriptorImageBindingState){
            .binding = write->dstBinding,
            .array_element = array_element,
            .view = image->imageView,
            .sampler = image->sampler,
            .layout = image->imageLayout,
            .image = view ? view->image : VK_NULL_HANDLE,
            .format = view ? view->format : VK_FORMAT_UNDEFINED,
            .view_type = view ? view->view_type : VK_IMAGE_VIEW_TYPE_2D,
            .base_mip_level = view ? view->base_mip_level : 0,
            .base_array_layer = view ? view->base_array_layer : 0,
            .signature = signature,
            .update_call = update_call,
            .valid = view != NULL,
        };
        if (!view) {
            record->class_complete = false;
        }
    }
}

static ShaderModuleRecord* find_shader_module(VkShaderModule handle) {
    for (size_t index = 0; index < kMaxShaderModules; ++index) {
        if (g_shader_modules[index].alive &&
            g_shader_modules[index].handle == handle) {
            return &g_shader_modules[index];
        }
    }
    return NULL;
}

static ShaderModuleRecord* add_shader_module(VkShaderModule handle) {
    ShaderModuleRecord* existing = find_shader_module(handle);
    if (existing) {
        return existing;
    }
    for (size_t index = 0; index < kMaxShaderModules; ++index) {
        if (!g_shader_modules[index].alive) {
            g_shader_modules[index] = (ShaderModuleRecord){
                .handle = handle,
                .alive = true,
            };
            return &g_shader_modules[index];
        }
    }
    report_overflow("shader-module");
    return NULL;
}

static GraphicsPipelineRecord* find_graphics_pipeline(VkPipeline handle) {
    for (size_t index = 0; index < kMaxGraphicsPipelines; ++index) {
        if (g_graphics_pipelines[index].alive &&
            g_graphics_pipelines[index].handle == handle) {
            return &g_graphics_pipelines[index];
        }
    }
    return NULL;
}

static GraphicsPipelineRecord* add_graphics_pipeline(VkPipeline handle) {
    GraphicsPipelineRecord* existing = find_graphics_pipeline(handle);
    if (existing) {
        return existing;
    }
    for (size_t index = 0; index < kMaxGraphicsPipelines; ++index) {
        if (!g_graphics_pipelines[index].alive) {
            g_graphics_pipelines[index] = (GraphicsPipelineRecord){
                .handle = handle,
                .alive = true,
            };
            return &g_graphics_pipelines[index];
        }
    }
    report_overflow("graphics-pipeline");
    return NULL;
}

static void add_command_pipeline_signature(
    CommandBufferRecord* command,
    uint64_t signature) {
    if (!command || signature == 0) {
        return;
    }
    for (uint32_t index = 0;
         index < command->distinct_pipeline_count; ++index) {
        if (command->pipeline_signatures[index] == signature) {
            return;
        }
    }
    if (command->distinct_pipeline_count >= kMaxDrawPipelines) {
        command->pipeline_overflow = true;
        return;
    }
    command->pipeline_signatures[command->distinct_pipeline_count++] =
        signature;
}

static void merge_submission_command(
    DrawSubmissionSummary* summary,
    const CommandBufferRecord* command) {
    if (!summary || !command || command->generation == 0) {
        return;
    }
    ++summary->tracked_command_count;
    if (summary->tracked_command_count == 1) {
        summary->generation = command->generation;
        summary->command_buffer = command->handle;
        summary->framebuffer = command->framebuffer;
        summary->first_pipeline_signature =
            command->first_pipeline_signature;
        summary->bound_set_count = command->draw_bound_set_count;
        summary->descriptor_layout_signature =
            command->draw_descriptor_layout_signature;
        summary->descriptor_handle_signature =
            command->draw_descriptor_handle_signature;
        summary->descriptor_update_signature =
            command->draw_descriptor_update_signature;
        summary->push_constant_signature =
            command->draw_push_constant_signature;
        summary->push_constant_bytes = command->draw_push_constant_bytes;
        summary->input_complete = command->draw_input_complete;
        memcpy(
            summary->descriptor_set_layout_signatures,
            command->draw_set_layout_signatures,
            sizeof(summary->descriptor_set_layout_signatures));
        memcpy(
            summary->descriptor_image_update_signatures,
            command->draw_set_image_update_signatures,
            sizeof(summary->descriptor_image_update_signatures));
        memcpy(
            summary->descriptor_buffer_update_signatures,
            command->draw_set_buffer_update_signatures,
            sizeof(summary->descriptor_buffer_update_signatures));
        memcpy(
            summary->descriptor_image_update_calls,
            command->draw_set_image_update_calls,
            sizeof(summary->descriptor_image_update_calls));
        memcpy(
            summary->descriptor_buffer_update_calls,
            command->draw_set_buffer_update_calls,
            sizeof(summary->descriptor_buffer_update_calls));
        memcpy(
            summary->descriptor_image_update_counts,
            command->draw_set_image_update_counts,
            sizeof(summary->descriptor_image_update_counts));
        memcpy(
            summary->descriptor_buffer_update_counts,
            command->draw_set_buffer_update_counts,
            sizeof(summary->descriptor_buffer_update_counts));
        memcpy(
            summary->descriptor_expected_image_counts,
            command->draw_set_expected_image_counts,
            sizeof(summary->descriptor_expected_image_counts));
        memcpy(
            summary->descriptor_expected_buffer_counts,
            command->draw_set_expected_buffer_counts,
            sizeof(summary->descriptor_expected_buffer_counts));
        summary->descriptor_class_complete =
            command->draw_descriptor_class_complete;
        memcpy(
            summary->descriptor_images,
            command->draw_descriptor_images,
            sizeof(summary->descriptor_images));
    } else if (summary->generation != command->generation) {
        summary->generation = 0;
        summary->input_complete = false;
        summary->descriptor_class_complete = false;
    } else {
        summary->bound_set_count += command->draw_bound_set_count;
        summary->descriptor_layout_signature = hash_mix(
            summary->descriptor_layout_signature,
            command->draw_descriptor_layout_signature);
        summary->descriptor_handle_signature = hash_mix(
            summary->descriptor_handle_signature,
            command->draw_descriptor_handle_signature);
        summary->descriptor_update_signature = hash_mix(
            summary->descriptor_update_signature,
            command->draw_descriptor_update_signature);
        summary->push_constant_signature = hash_mix(
            summary->push_constant_signature,
            command->draw_push_constant_signature);
        summary->push_constant_bytes += command->draw_push_constant_bytes;
        summary->input_complete &= command->draw_input_complete;
        summary->descriptor_class_complete = false;
    }
    summary->draw_count += command->draw_count;
    summary->indexed_draw_count += command->indexed_draw_count;
    summary->last_pipeline_signature = command->last_pipeline_signature;
    summary->draw_signature = hash_mix(
        summary->draw_signature == 0
            ? UINT64_C(1469598103934665603)
            : summary->draw_signature,
        command->draw_signature);
    summary->pipeline_overflow |= command->pipeline_overflow;
    for (uint32_t pipeline = 0;
         pipeline < command->distinct_pipeline_count; ++pipeline) {
        bool found = false;
        for (uint32_t existing = 0;
             existing < summary->distinct_pipeline_count; ++existing) {
            if (summary->pipeline_signatures[existing] ==
                command->pipeline_signatures[pipeline]) {
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        if (summary->distinct_pipeline_count >= kMaxDrawPipelines) {
            summary->pipeline_overflow = true;
            continue;
        }
        summary->pipeline_signatures[
            summary->distinct_pipeline_count++] =
                command->pipeline_signatures[pipeline];
    }
}

static void merge_present_signal(
    DrawSubmissionSummary* summary,
    const SignaledSemaphoreRecord* signal) {
    if (!summary || !signal || signal->tracked_command_count == 0) {
        return;
    }
    if (summary->tracked_command_count == 0) {
        summary->generation = signal->generation;
        summary->command_buffer = signal->command_buffer;
        summary->framebuffer = signal->framebuffer;
        summary->first_pipeline_signature =
            signal->first_pipeline_signature;
        summary->bound_set_count = signal->bound_set_count;
        summary->descriptor_layout_signature =
            signal->descriptor_layout_signature;
        summary->descriptor_handle_signature =
            signal->descriptor_handle_signature;
        summary->descriptor_update_signature =
            signal->descriptor_update_signature;
        summary->push_constant_signature =
            signal->push_constant_signature;
        summary->push_constant_bytes = signal->push_constant_bytes;
        summary->input_complete = signal->input_complete;
        memcpy(
            summary->descriptor_set_layout_signatures,
            signal->descriptor_set_layout_signatures,
            sizeof(summary->descriptor_set_layout_signatures));
        memcpy(
            summary->descriptor_image_update_signatures,
            signal->descriptor_image_update_signatures,
            sizeof(summary->descriptor_image_update_signatures));
        memcpy(
            summary->descriptor_buffer_update_signatures,
            signal->descriptor_buffer_update_signatures,
            sizeof(summary->descriptor_buffer_update_signatures));
        memcpy(
            summary->descriptor_image_update_calls,
            signal->descriptor_image_update_calls,
            sizeof(summary->descriptor_image_update_calls));
        memcpy(
            summary->descriptor_buffer_update_calls,
            signal->descriptor_buffer_update_calls,
            sizeof(summary->descriptor_buffer_update_calls));
        memcpy(
            summary->descriptor_image_update_counts,
            signal->descriptor_image_update_counts,
            sizeof(summary->descriptor_image_update_counts));
        memcpy(
            summary->descriptor_buffer_update_counts,
            signal->descriptor_buffer_update_counts,
            sizeof(summary->descriptor_buffer_update_counts));
        memcpy(
            summary->descriptor_expected_image_counts,
            signal->descriptor_expected_image_counts,
            sizeof(summary->descriptor_expected_image_counts));
        memcpy(
            summary->descriptor_expected_buffer_counts,
            signal->descriptor_expected_buffer_counts,
            sizeof(summary->descriptor_expected_buffer_counts));
        summary->descriptor_class_complete =
            signal->descriptor_class_complete;
        memcpy(
            summary->descriptor_images,
            signal->descriptor_images,
            sizeof(summary->descriptor_images));
    } else if (summary->generation != signal->generation) {
        summary->generation = 0;
        summary->input_complete = false;
        summary->descriptor_class_complete = false;
    } else {
        summary->bound_set_count += signal->bound_set_count;
        summary->descriptor_layout_signature = hash_mix(
            summary->descriptor_layout_signature,
            signal->descriptor_layout_signature);
        summary->descriptor_handle_signature = hash_mix(
            summary->descriptor_handle_signature,
            signal->descriptor_handle_signature);
        summary->descriptor_update_signature = hash_mix(
            summary->descriptor_update_signature,
            signal->descriptor_update_signature);
        summary->push_constant_signature = hash_mix(
            summary->push_constant_signature,
            signal->push_constant_signature);
        summary->push_constant_bytes += signal->push_constant_bytes;
        summary->input_complete &= signal->input_complete;
        summary->descriptor_class_complete = false;
    }
    summary->tracked_command_count += signal->tracked_command_count;
    summary->draw_count += signal->draw_count;
    summary->indexed_draw_count += signal->indexed_draw_count;
    summary->last_pipeline_signature = signal->last_pipeline_signature;
    summary->draw_signature = hash_mix(
        summary->draw_signature == 0
            ? UINT64_C(1469598103934665603)
            : summary->draw_signature,
        signal->draw_signature);
    summary->pipeline_overflow |= signal->pipeline_overflow;
    for (uint32_t pipeline = 0;
         pipeline < signal->distinct_pipeline_count; ++pipeline) {
        bool found = false;
        for (uint32_t existing = 0;
             existing < summary->distinct_pipeline_count; ++existing) {
            if (summary->pipeline_signatures[existing] ==
                signal->pipeline_signatures[pipeline]) {
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        if (summary->distinct_pipeline_count >= kMaxDrawPipelines) {
            summary->pipeline_overflow = true;
            continue;
        }
        summary->pipeline_signatures[
            summary->distinct_pipeline_count++] =
                signal->pipeline_signatures[pipeline];
    }
}

static CommandBufferRecord* set_command_buffer(
    VkCommandBuffer handle,
    const FramebufferRecord* framebuffer) {
    CommandBufferRecord* record = find_command_buffer(handle);
    if (!record) {
        for (size_t index = 0; index < kMaxCommandBuffers; ++index) {
            if (!g_command_buffers[index].occupied) {
                record = &g_command_buffers[index];
                break;
            }
        }
    }
    if (!record) {
        report_overflow("command-buffer");
        return NULL;
    }
    if (!record->occupied) {
        *record = (CommandBufferRecord){
            .handle = handle,
            .occupied = true,
        };
    }
    record->framebuffer = framebuffer->handle;
    record->render_pass = framebuffer->render_pass;
    record->generation = framebuffer->generation;
    record->in_render_pass = true;
    return record;
}

static bool format_is_depth_or_stencil(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

static bool startup_audit_finished(void) {
    return atomic_load(&g_startup_color_audit) &&
           atomic_load(&g_startup_color_audit_finished);
}

static bool should_sample_present_pixel(
    uint64_t generation,
    uint32_t ordinal) {
    if (generation == 1) {
        return ordinal == 1;
    }
    if (generation != 2) {
        return false;
    }
    switch (ordinal) {
        case 1:
        case 10:
        case 20:
        case 30:
        case 40:
        case 50:
        case 60:
        case 70:
        case 80:
        case 90:
        case 100:
        case 110:
        case 120:
        case 130:
        case 140:
        case 150:
        case 160:
        case 170:
        case 180:
            return true;
        default:
            return false;
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_begin_command_buffer(
    VkCommandBuffer command_buffer,
    const VkCommandBufferBeginInfo* begin_info) {
    if (!g_next_begin_command_buffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_begin_command_buffer(command_buffer, begin_info);
    if (result == VK_SUCCESS && atomic_load(&g_startup_draw_audit) &&
        !startup_audit_finished()) {
        pthread_mutex_lock(&g_lock);
        CommandBufferRecord* record = find_command_buffer(command_buffer);
        if (!record) {
            for (size_t index = 0; index < kMaxCommandBuffers; ++index) {
                if (!g_command_buffers[index].occupied) {
                    record = &g_command_buffers[index];
                    break;
                }
            }
        }
        if (record) {
            memset(record, 0, sizeof(*record));
            record->handle = command_buffer;
            record->occupied = true;
        } else {
            report_overflow("command-buffer");
        }
        pthread_mutex_unlock(&g_lock);
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_descriptor_set_layout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkDescriptorSetLayout* set_layout) {
    if (!g_next_create_descriptor_set_layout) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_descriptor_set_layout(
        device, create_info, allocator, set_layout);
    if (result != VK_SUCCESS || !set_layout ||
        *set_layout == VK_NULL_HANDLE ||
        !atomic_load(&g_startup_input_audit) || startup_audit_finished()) {
        return result;
    }
    DescriptorSetLayoutRecord snapshot = {0};
    pthread_mutex_lock(&g_lock);
    DescriptorSetLayoutRecord* record =
        add_descriptor_set_layout(*set_layout);
    if (record) {
        record->signature = UINT64_C(1469598103934665603);
        record->binding_count = create_info ? create_info->bindingCount : 0;
        for (uint32_t index = 0;
             create_info && create_info->pBindings &&
             index < create_info->bindingCount; ++index) {
            const VkDescriptorSetLayoutBinding* binding =
                &create_info->pBindings[index];
            record->signature = hash_mix(record->signature, binding->binding);
            record->signature = hash_mix(
                record->signature, binding->descriptorType);
            record->signature = hash_mix(
                record->signature, binding->descriptorCount);
            record->signature = hash_mix(
                record->signature, binding->stageFlags);
            record->signature = hash_mix(
                record->signature, binding->pImmutableSamplers ? 1 : 0);
            record->descriptor_count += binding->descriptorCount;
            if (descriptor_type_is_image(binding->descriptorType)) {
                record->image_count += binding->descriptorCount;
            } else if (descriptor_type_is_buffer(binding->descriptorType)) {
                record->buffer_count += binding->descriptorCount;
            } else if (binding->descriptorType ==
                       VK_DESCRIPTOR_TYPE_SAMPLER) {
                record->sampler_count += binding->descriptorCount;
            } else if (binding->descriptorType ==
                       VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
                record->input_attachment_count += binding->descriptorCount;
            }
        }
        snapshot = *record;
    }
    pthread_mutex_unlock(&g_lock);
    lifecycle_log(
        "STARTUP_INPUT_SET_LAYOUT: layout=%p signature=%016" PRIx64
        " bindings=%u descriptors=%u images=%u buffers=%u samplers=%u"
        " input_attachments=%u result=%d",
        (void*)*set_layout, snapshot.signature, snapshot.binding_count,
        snapshot.descriptor_count, snapshot.image_count,
        snapshot.buffer_count, snapshot.sampler_count,
        snapshot.input_attachment_count, result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_descriptor_set_layout(
    VkDevice device,
    VkDescriptorSetLayout set_layout,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_descriptor_set_layout) {
        return;
    }
    g_next_destroy_descriptor_set_layout(device, set_layout, allocator);
    if (!atomic_load(&g_startup_input_audit)) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    DescriptorSetLayoutRecord* record =
        find_descriptor_set_layout(set_layout);
    if (record) {
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_pipeline_layout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkPipelineLayout* pipeline_layout) {
    if (!g_next_create_pipeline_layout) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_pipeline_layout(
        device, create_info, allocator, pipeline_layout);
    if (result != VK_SUCCESS || !pipeline_layout ||
        *pipeline_layout == VK_NULL_HANDLE ||
        !atomic_load(&g_startup_input_audit) || startup_audit_finished()) {
        return result;
    }
    PipelineLayoutRecord snapshot = {0};
    pthread_mutex_lock(&g_lock);
    PipelineLayoutRecord* record = add_pipeline_layout(*pipeline_layout);
    if (record) {
        record->signature = UINT64_C(1469598103934665603);
        record->set_layout_count = create_info ? create_info->setLayoutCount : 0;
        record->complete = true;
        if (record->set_layout_count > kMaxBoundDescriptorSets) {
            record->complete = false;
            report_overflow("pipeline-layout-set-count");
        }
        for (uint32_t index = 0;
             create_info && create_info->pSetLayouts &&
             index < create_info->setLayoutCount; ++index) {
            DescriptorSetLayoutRecord* set_layout =
                find_descriptor_set_layout(create_info->pSetLayouts[index]);
            if (!set_layout) {
                record->complete = false;
                record->signature = hash_mix(record->signature, 0);
                continue;
            }
            record->signature = hash_mix(
                record->signature, set_layout->signature);
            if (index < kMaxBoundDescriptorSets) {
                record->set_layout_signatures[index] =
                    set_layout->signature;
                record->set_descriptor_counts[index] =
                    set_layout->descriptor_count;
            }
            record->descriptor_count += set_layout->descriptor_count;
            record->image_count += set_layout->image_count;
            record->buffer_count += set_layout->buffer_count;
            record->sampler_count += set_layout->sampler_count;
            record->input_attachment_count +=
                set_layout->input_attachment_count;
        }
        for (uint32_t index = 0;
             create_info && create_info->pPushConstantRanges &&
             index < create_info->pushConstantRangeCount; ++index) {
            const VkPushConstantRange* range =
                &create_info->pPushConstantRanges[index];
            record->signature = hash_mix(record->signature, range->stageFlags);
            record->signature = hash_mix(record->signature, range->offset);
            record->signature = hash_mix(record->signature, range->size);
            const uint64_t end = (uint64_t)range->offset + range->size;
            if (end > record->push_constant_bytes) {
                record->push_constant_bytes =
                    end > UINT32_MAX ? UINT32_MAX : (uint32_t)end;
            }
        }
        snapshot = *record;
    }
    pthread_mutex_unlock(&g_lock);
    lifecycle_log(
        "STARTUP_INPUT_PIPELINE_LAYOUT: layout=%p signature=%016" PRIx64
        " set_layouts=%u descriptors=%u images=%u buffers=%u samplers=%u"
        " input_attachments=%u push_bytes=%u complete=%s result=%d",
        (void*)*pipeline_layout, snapshot.signature,
        snapshot.set_layout_count, snapshot.descriptor_count,
        snapshot.image_count, snapshot.buffer_count,
        snapshot.sampler_count, snapshot.input_attachment_count,
        snapshot.push_constant_bytes, snapshot.complete ? "yes" : "no",
        result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_pipeline_layout(
    VkDevice device,
    VkPipelineLayout pipeline_layout,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_pipeline_layout) {
        return;
    }
    g_next_destroy_pipeline_layout(device, pipeline_layout, allocator);
    if (!atomic_load(&g_startup_input_audit)) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    PipelineLayoutRecord* record = find_pipeline_layout(pipeline_layout);
    if (record) {
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_allocate_descriptor_sets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* allocate_info,
    VkDescriptorSet* descriptor_sets) {
    if (!g_next_allocate_descriptor_sets) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_allocate_descriptor_sets(
        device, allocate_info, descriptor_sets);
    if (result != VK_SUCCESS || !allocate_info || !descriptor_sets ||
        !atomic_load(&g_startup_input_audit) || startup_audit_finished()) {
        return result;
    }
    pthread_mutex_lock(&g_lock);
    for (uint32_t index = 0; index < allocate_info->descriptorSetCount;
         ++index) {
        DescriptorSetRecord* record =
            find_descriptor_set(descriptor_sets[index], true);
        DescriptorSetLayoutRecord* layout = find_descriptor_set_layout(
            allocate_info->pSetLayouts[index]);
        if (record) {
            record->pool = allocate_info->descriptorPool;
            record->layout_signature = layout ? layout->signature : 0;
            record->update_signature = 0;
            record->update_count = 0;
            record->last_update_call = 0;
            record->image_update_signature = 0;
            record->image_update_count = 0;
            record->last_image_update_call = 0;
            record->buffer_update_signature = 0;
            record->buffer_update_count = 0;
            record->last_buffer_update_call = 0;
            record->expected_image_count = layout
                ? layout->image_count + layout->sampler_count
                : 0;
            record->expected_buffer_count =
                layout ? layout->buffer_count : 0;
            record->class_complete = layout != NULL;
            memset(record->images, 0, sizeof(record->images));
            record->alive = true;
        }
    }
    pthread_mutex_unlock(&g_lock);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_free_descriptor_sets(
    VkDevice device,
    VkDescriptorPool pool,
    uint32_t descriptor_set_count,
    const VkDescriptorSet* descriptor_sets) {
    if (!g_next_free_descriptor_sets) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_free_descriptor_sets(
        device, pool, descriptor_set_count, descriptor_sets);
    if (result == VK_SUCCESS && descriptor_sets &&
        atomic_load(&g_startup_input_audit)) {
        pthread_mutex_lock(&g_lock);
        for (uint32_t index = 0; index < descriptor_set_count; ++index) {
            DescriptorSetRecord* record =
                find_descriptor_set(descriptor_sets[index], false);
            if (record) {
                record->alive = false;
            }
        }
        pthread_mutex_unlock(&g_lock);
    }
    return result;
}

static void forget_descriptor_pool(VkDescriptorPool pool) {
    for (size_t index = 0; index < kMaxDescriptorSets; ++index) {
        if (g_descriptor_sets[index].occupied &&
            g_descriptor_sets[index].pool == pool) {
            g_descriptor_sets[index].alive = false;
        }
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_reset_descriptor_pool(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorPoolResetFlags flags) {
    if (!g_next_reset_descriptor_pool) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_reset_descriptor_pool(device, pool, flags);
    if (result == VK_SUCCESS && atomic_load(&g_startup_input_audit)) {
        pthread_mutex_lock(&g_lock);
        forget_descriptor_pool(pool);
        pthread_mutex_unlock(&g_lock);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_descriptor_pool(
    VkDevice device,
    VkDescriptorPool pool,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_descriptor_pool) {
        return;
    }
    g_next_destroy_descriptor_pool(device, pool, allocator);
    if (atomic_load(&g_startup_input_audit)) {
        pthread_mutex_lock(&g_lock);
        forget_descriptor_pool(pool);
        pthread_mutex_unlock(&g_lock);
    }
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
    if (!atomic_load(&g_startup_input_audit) || startup_audit_finished()) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    const uint64_t update_call = ++g_descriptor_update_call_counter;
    for (uint32_t index = 0; writes && index < write_count; ++index) {
        DescriptorSetRecord* record =
            find_descriptor_set(writes[index].dstSet, false);
        if (record && record->alive) {
            const uint64_t write_signature =
                descriptor_write_signature(&writes[index]);
            if (record->last_update_call != update_call) {
                record->update_signature =
                    UINT64_C(1469598103934665603);
                record->update_count = 0;
                record->last_update_call = update_call;
            }
            record->update_signature = hash_mix(
                record->update_signature, write_signature);
            ++record->update_count;
            if (descriptor_type_is_image_class(
                    writes[index].descriptorType)) {
                if (record->last_image_update_call != update_call) {
                    record->image_update_signature =
                        UINT64_C(1469598103934665603);
                    record->image_update_count = 0;
                    record->last_image_update_call = update_call;
                }
                record->image_update_signature = hash_mix(
                    record->image_update_signature, write_signature);
                ++record->image_update_count;
                update_descriptor_image_bindings(
                    record, &writes[index], update_call);
            } else if (descriptor_type_is_buffer(
                           writes[index].descriptorType)) {
                if (record->last_buffer_update_call != update_call) {
                    record->buffer_update_signature =
                        UINT64_C(1469598103934665603);
                    record->buffer_update_count = 0;
                    record->last_buffer_update_call = update_call;
                }
                record->buffer_update_signature = hash_mix(
                    record->buffer_update_signature, write_signature);
                ++record->buffer_update_count;
            } else {
                record->class_complete = false;
            }
        }
    }
    for (uint32_t index = 0; copies && index < copy_count; ++index) {
        DescriptorSetRecord* source =
            find_descriptor_set(copies[index].srcSet, false);
        DescriptorSetRecord* destination =
            find_descriptor_set(copies[index].dstSet, false);
        if (destination && destination->alive) {
            if (destination->last_update_call != update_call) {
                destination->update_signature =
                    UINT64_C(1469598103934665603);
                destination->update_count = 0;
                destination->last_update_call = update_call;
            }
            uint64_t signature = UINT64_C(1469598103934665603);
            signature = hash_mix(
                signature, source ? source->update_signature : 0);
            signature = hash_mix(signature, copies[index].srcBinding);
            signature = hash_mix(signature, copies[index].srcArrayElement);
            signature = hash_mix(signature, copies[index].dstBinding);
            signature = hash_mix(signature, copies[index].dstArrayElement);
            signature = hash_mix(signature, copies[index].descriptorCount);
            destination->update_signature = hash_mix(
                destination->update_signature,
                signature);
            ++destination->update_count;
            destination->class_complete = false;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_bind_descriptor_sets(
    VkCommandBuffer command_buffer,
    VkPipelineBindPoint pipeline_bind_point,
    VkPipelineLayout layout,
    uint32_t first_set,
    uint32_t descriptor_set_count,
    const VkDescriptorSet* descriptor_sets,
    uint32_t dynamic_offset_count,
    const uint32_t* dynamic_offsets) {
    if (!g_next_cmd_bind_descriptor_sets) {
        return;
    }
    g_next_cmd_bind_descriptor_sets(
        command_buffer, pipeline_bind_point, layout, first_set,
        descriptor_set_count, descriptor_sets, dynamic_offset_count,
        dynamic_offsets);
    if (!atomic_load(&g_startup_input_audit) || startup_audit_finished() ||
        pipeline_bind_point != VK_PIPELINE_BIND_POINT_GRAPHICS) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    CommandBufferRecord* command = find_command_buffer(command_buffer);
    if (command) {
        for (uint32_t index = 0; descriptor_sets &&
             index < descriptor_set_count; ++index) {
            const uint32_t slot = first_set + index;
            if (slot >= kMaxBoundDescriptorSets) {
                command->draw_input_complete = false;
                report_overflow("bound-descriptor-set");
                continue;
            }
            command->bound_sets[slot] = descriptor_sets[index];
            if (slot + 1 > command->bound_set_count) {
                command->bound_set_count = slot + 1;
            }
        }
        for (uint32_t index = 0; dynamic_offsets &&
             index < dynamic_offset_count; ++index) {
            command->dynamic_offset_signature = hash_mix(
                command->dynamic_offset_signature == 0
                    ? UINT64_C(1469598103934665603)
                    : command->dynamic_offset_signature,
                dynamic_offsets[index]);
        }
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_push_constants(
    VkCommandBuffer command_buffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stage_flags,
    uint32_t offset,
    uint32_t size,
    const void* values) {
    if (!g_next_cmd_push_constants) {
        return;
    }
    g_next_cmd_push_constants(
        command_buffer, layout, stage_flags, offset, size, values);
    if (!atomic_load(&g_startup_input_audit) || startup_audit_finished()) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    CommandBufferRecord* command = find_command_buffer(command_buffer);
    if (command) {
        uint64_t signature = UINT64_C(1469598103934665603);
        signature = hash_mix(signature, (uint64_t)(uintptr_t)layout);
        signature = hash_mix(signature, stage_flags);
        signature = hash_mix(signature, offset);
        signature = hash_mix(signature, size);
        signature = hash_mix(signature, hash_bytes(values, size));
        command->push_constant_signature = hash_mix(
            command->push_constant_signature == 0
                ? UINT64_C(1469598103934665603)
                : command->push_constant_signature,
            signature);
        command->push_constant_bytes += size;
        ++command->push_constant_calls;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_shader_module(
    VkDevice device,
    const VkShaderModuleCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkShaderModule* shader_module) {
    if (!g_next_create_shader_module) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const uint64_t code_hash = create_info
        ? hash_bytes(create_info->pCode, create_info->codeSize)
        : 0;
    VkResult result = g_next_create_shader_module(
        device, create_info, allocator, shader_module);
    if (result == VK_SUCCESS && shader_module &&
        *shader_module != VK_NULL_HANDLE &&
        atomic_load(&g_startup_draw_audit) && !startup_audit_finished()) {
        pthread_mutex_lock(&g_lock);
        ShaderModuleRecord* record = add_shader_module(*shader_module);
        if (record) {
            record->code_hash = code_hash;
            record->code_size = create_info ? create_info->codeSize : 0;
        }
        pthread_mutex_unlock(&g_lock);
        lifecycle_log(
            "STARTUP_DRAW_SHADER: module=%p code_size=%zu code_hash=%016" PRIx64,
            (void*)*shader_module,
            create_info ? create_info->codeSize : 0, code_hash);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_shader_module(
    VkDevice device,
    VkShaderModule shader_module,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_shader_module) {
        return;
    }
    g_next_destroy_shader_module(device, shader_module, allocator);
    pthread_mutex_lock(&g_lock);
    ShaderModuleRecord* record = find_shader_module(shader_module);
    if (record) {
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static uint64_t pipeline_signature_locked(
    const VkGraphicsPipelineCreateInfo* info,
    uint64_t* vertex_shader_hash,
    uint64_t* fragment_shader_hash,
    bool* shader_hash_complete) {
    uint64_t hash = UINT64_C(1469598103934665603);
    *vertex_shader_hash = 0;
    *fragment_shader_hash = 0;
    *shader_hash_complete = info && info->stageCount > 0;
    if (!info) {
        return 0;
    }
    hash = hash_mix(hash, info->flags);
    hash = hash_mix(hash, info->stageCount);
    hash = hash_mix(hash, info->subpass);
    for (uint32_t stage_index = 0;
         stage_index < info->stageCount; ++stage_index) {
        const VkPipelineShaderStageCreateInfo* stage =
            &info->pStages[stage_index];
        ShaderModuleRecord* module = find_shader_module(stage->module);
        const uint64_t module_hash = module ? module->code_hash : 0;
        *shader_hash_complete &= module_hash != 0;
        if (stage->stage == VK_SHADER_STAGE_VERTEX_BIT) {
            *vertex_shader_hash = module_hash;
        } else if (stage->stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            *fragment_shader_hash = module_hash;
        }
        hash = hash_mix(hash, stage->stage);
        hash = hash_mix(hash, module_hash);
        if (stage->pName) {
            hash = hash_mix(hash, hash_bytes(stage->pName, strlen(stage->pName)));
        }
    }
    if (info->pInputAssemblyState) {
        hash = hash_mix(hash, info->pInputAssemblyState->topology);
        hash = hash_mix(hash, info->pInputAssemblyState->primitiveRestartEnable);
    }
    if (info->pRasterizationState) {
        hash = hash_mix(hash, info->pRasterizationState->polygonMode);
        hash = hash_mix(hash, info->pRasterizationState->cullMode);
        hash = hash_mix(hash, info->pRasterizationState->frontFace);
        hash = hash_mix(hash, info->pRasterizationState->rasterizerDiscardEnable);
    }
    if (info->pMultisampleState) {
        hash = hash_mix(hash, info->pMultisampleState->rasterizationSamples);
    }
    if (info->pDepthStencilState) {
        hash = hash_mix(hash, info->pDepthStencilState->depthTestEnable);
        hash = hash_mix(hash, info->pDepthStencilState->depthWriteEnable);
        hash = hash_mix(hash, info->pDepthStencilState->depthCompareOp);
        hash = hash_mix(hash, info->pDepthStencilState->stencilTestEnable);
    }
    if (info->pColorBlendState) {
        hash = hash_mix(hash, info->pColorBlendState->logicOpEnable);
        hash = hash_mix(hash, info->pColorBlendState->logicOp);
        hash = hash_mix(hash, info->pColorBlendState->attachmentCount);
        for (uint32_t attachment = 0;
             attachment < info->pColorBlendState->attachmentCount;
             ++attachment) {
            const VkPipelineColorBlendAttachmentState* blend =
                &info->pColorBlendState->pAttachments[attachment];
            hash = hash_mix(hash, blend->blendEnable);
            hash = hash_mix(hash, blend->srcColorBlendFactor);
            hash = hash_mix(hash, blend->dstColorBlendFactor);
            hash = hash_mix(hash, blend->colorBlendOp);
            hash = hash_mix(hash, blend->srcAlphaBlendFactor);
            hash = hash_mix(hash, blend->dstAlphaBlendFactor);
            hash = hash_mix(hash, blend->alphaBlendOp);
            hash = hash_mix(hash, blend->colorWriteMask);
        }
    }
    return hash;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_graphics_pipelines(
    VkDevice device,
    VkPipelineCache pipeline_cache,
    uint32_t create_info_count,
    const VkGraphicsPipelineCreateInfo* create_infos,
    const VkAllocationCallbacks* allocator,
    VkPipeline* pipelines) {
    if (!g_next_create_graphics_pipelines) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_create_graphics_pipelines(
        device, pipeline_cache, create_info_count, create_infos,
        allocator, pipelines);
    if (!atomic_load(&g_startup_draw_audit) || startup_audit_finished() ||
        !create_infos || !pipelines) {
        return result;
    }
    for (uint32_t index = 0; index < create_info_count; ++index) {
        if (pipelines[index] == VK_NULL_HANDLE) {
            continue;
        }
        uint64_t vertex_hash = 0;
        uint64_t fragment_hash = 0;
        uint64_t signature = 0;
        bool shader_hash_complete = false;
        PipelineLayoutRecord layout_snapshot = {0};
        pthread_mutex_lock(&g_lock);
        signature = pipeline_signature_locked(
            &create_infos[index], &vertex_hash, &fragment_hash,
            &shader_hash_complete);
        GraphicsPipelineRecord* record =
            add_graphics_pipeline(pipelines[index]);
        PipelineLayoutRecord* layout =
            find_pipeline_layout(create_infos[index].layout);
        if (layout) {
            layout_snapshot = *layout;
        }
        if (record) {
            record->signature = signature;
            record->vertex_shader_hash = vertex_hash;
            record->fragment_shader_hash = fragment_hash;
            record->layout = create_infos[index].layout;
            record->layout_signature = layout_snapshot.signature;
            record->set_layout_count = layout_snapshot.set_layout_count;
            memcpy(
                record->set_layout_signatures,
                layout_snapshot.set_layout_signatures,
                sizeof(record->set_layout_signatures));
            memcpy(
                record->set_descriptor_counts,
                layout_snapshot.set_descriptor_counts,
                sizeof(record->set_descriptor_counts));
            record->descriptor_count = layout_snapshot.descriptor_count;
            record->image_count = layout_snapshot.image_count;
            record->buffer_count = layout_snapshot.buffer_count;
            record->sampler_count = layout_snapshot.sampler_count;
            record->input_attachment_count =
                layout_snapshot.input_attachment_count;
            record->push_constant_bytes =
                layout_snapshot.push_constant_bytes;
            record->render_pass = create_infos[index].renderPass;
            record->subpass = create_infos[index].subpass;
            record->shader_hash_complete = shader_hash_complete;
            record->layout_complete = layout && layout->complete;
        }
        pthread_mutex_unlock(&g_lock);
        lifecycle_log(
            "STARTUP_DRAW_PIPELINE_CREATE: pipeline=%p signature=%016" PRIx64
            " vertex_hash=%016" PRIx64 " fragment_hash=%016" PRIx64
            " shader_hash_complete=%s stages=%u render_pass=%p subpass=%u"
            " result=%d",
            (void*)pipelines[index], signature, vertex_hash, fragment_hash,
            shader_hash_complete ? "yes" : "no",
            create_infos[index].stageCount,
            (void*)create_infos[index].renderPass,
            create_infos[index].subpass, result);
        if (atomic_load(&g_startup_input_audit)) {
            lifecycle_log(
                "STARTUP_INPUT_PIPELINE: pipeline=%p"
                " pipeline_signature=%016" PRIx64
                " layout_signature=%016" PRIx64
                " set_layouts=%u descriptors=%u images=%u buffers=%u"
                " samplers=%u input_attachments=%u push_bytes=%u"
                " layout_complete=%s",
                (void*)pipelines[index], signature,
                layout_snapshot.signature, layout_snapshot.set_layout_count,
                layout_snapshot.descriptor_count, layout_snapshot.image_count,
                layout_snapshot.buffer_count, layout_snapshot.sampler_count,
                layout_snapshot.input_attachment_count,
                layout_snapshot.push_constant_bytes,
                layout && layout->complete ? "yes" : "no");
        }
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_pipeline(
    VkDevice device,
    VkPipeline pipeline,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_pipeline) {
        return;
    }
    g_next_destroy_pipeline(device, pipeline, allocator);
    pthread_mutex_lock(&g_lock);
    GraphicsPipelineRecord* record = find_graphics_pipeline(pipeline);
    if (record) {
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_bind_pipeline(
    VkCommandBuffer command_buffer,
    VkPipelineBindPoint pipeline_bind_point,
    VkPipeline pipeline) {
    if (!g_next_cmd_bind_pipeline) {
        return;
    }
    g_next_cmd_bind_pipeline(command_buffer, pipeline_bind_point, pipeline);
    if (!atomic_load(&g_startup_draw_audit) || startup_audit_finished() ||
        pipeline_bind_point != VK_PIPELINE_BIND_POINT_GRAPHICS) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    CommandBufferRecord* command = find_command_buffer(command_buffer);
    GraphicsPipelineRecord* record = find_graphics_pipeline(pipeline);
    if (command && command->in_render_pass) {
        command->bound_pipeline = pipeline;
        command->bound_pipeline_signature = record ? record->signature : 0;
        command->bound_pipeline_layout_signature =
            record ? record->layout_signature : 0;
        command->bound_pipeline_set_layout_count =
            record ? record->set_layout_count : 0;
        command->bound_pipeline_push_constant_bytes =
            record ? record->push_constant_bytes : 0;
        if (record) {
            memcpy(
                command->bound_pipeline_set_layout_signatures,
                record->set_layout_signatures,
                sizeof(command->bound_pipeline_set_layout_signatures));
            memcpy(
                command->bound_pipeline_set_descriptor_counts,
                record->set_descriptor_counts,
                sizeof(command->bound_pipeline_set_descriptor_counts));
        } else {
            memset(
                command->bound_pipeline_set_layout_signatures, 0,
                sizeof(command->bound_pipeline_set_layout_signatures));
            memset(
                command->bound_pipeline_set_descriptor_counts, 0,
                sizeof(command->bound_pipeline_set_descriptor_counts));
        }
        command->bound_pipeline_layout_complete =
            record && record->layout_complete;
    }
    pthread_mutex_unlock(&g_lock);
}

static uint32_t next_present_ordinal_for_generation_locked(
    uint64_t generation) {
    uint32_t present_count = 0;
    for (size_t index = 0; index < kMaxSwapchains; ++index) {
        if (g_swapchains[index].alive &&
            g_swapchains[index].generation == generation &&
            g_swapchains[index].present_count > present_count) {
            present_count = g_swapchains[index].present_count;
        }
    }
    return present_count + 1;
}

static bool compositor_descriptor_signature_locked(
    const CommandBufferRecord* command,
    bool test_override,
    uint64_t* signature_out) {
    if (!command || !signature_out ||
        !command->bound_pipeline_layout_complete) {
        return false;
    }
    const uint32_t required_set_count =
        command->bound_pipeline_set_layout_count;
    if (required_set_count == 0 ||
        required_set_count > kMaxBoundDescriptorSets ||
        (!test_override && required_set_count != 2) ||
        (!test_override &&
         command->bound_pipeline_push_constant_bytes != 0)) {
        return false;
    }
    uint64_t signature = UINT64_C(1469598103934665603);
    for (uint32_t slot = 0; slot < required_set_count; ++slot) {
        DescriptorSetRecord* set =
            find_descriptor_set(command->bound_sets[slot], false);
        if (!set || !set->alive || set->layout_signature == 0 ||
            set->layout_signature !=
                command->bound_pipeline_set_layout_signatures[slot] ||
            (command->bound_pipeline_set_descriptor_counts[slot] != 0 &&
             set->update_signature == 0) ||
            !set->class_complete ||
            (set->expected_image_count != 0 &&
             set->image_update_signature == 0) ||
            (set->expected_buffer_count != 0 &&
             set->buffer_update_signature == 0)) {
            return false;
        }
        if (!test_override) {
            if (set->layout_signature !=
                    kCompositorSetLayoutSignatures[slot] ||
                set->expected_image_count != (slot == 0 ? 0u : 2u) ||
                set->expected_buffer_count != 3) {
                return false;
            }
        }
        signature = hash_mix(signature, slot);
        signature = hash_mix(signature, set->update_signature);
        signature = hash_mix(signature, set->update_count);
    }
    *signature_out = signature;
    return true;
}

static void compositor_neutralize_latch_locked(
    CompositorNeutralizeState state,
    const char* reason,
    const CommandBufferRecord* command,
    uint32_t ordinal,
    uint64_t descriptor_signature) {
    g_compositor_neutralize_state = state;
    startup_color_detail_log(
        "STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: action=%s reason=%s"
        " generation=%" PRIu64 " ordinal=%u pipeline=%016" PRIx64
        " descriptor_update_signature=%016" PRIx64
        " suppressed_draws=%u",
        state == TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING
            ? "forward"
            : "abort",
        reason,
        command ? command->generation : 0,
        ordinal,
        command ? command->bound_pipeline_signature : 0,
        descriptor_signature,
        g_compositor_suppressed_draw_count);
}

static bool should_suppress_compositor_draw_locked(
    VkCommandBuffer command_buffer,
    bool indexed,
    uint32_t* width_out,
    uint32_t* height_out) {
    if (!atomic_load(&g_startup_compositor_neutralize) ||
        g_compositor_neutralize_state ==
            TESO4M4_COMPOSITOR_NEUTRALIZE_INACTIVE ||
        g_compositor_neutralize_state ==
            TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING ||
        g_compositor_neutralize_state ==
            TESO4M4_COMPOSITOR_NEUTRALIZE_ABORTED) {
        return false;
    }
    CommandBufferRecord* command = find_command_buffer(command_buffer);
    if (!command || !command->in_render_pass) {
        return false;
    }
    const bool test_override =
        g_compositor_neutralize_test_pipeline != VK_NULL_HANDLE;
    const bool target = test_override
        ? command->bound_pipeline == g_compositor_neutralize_test_pipeline
        : command->bound_pipeline_signature ==
              kCompositorPipelineSignature &&
          command->bound_pipeline_layout_signature ==
              kCompositorPipelineLayoutSignature;
    if (!target) {
        return false;
    }
    const uint32_t ordinal =
        next_present_ordinal_for_generation_locked(command->generation);
    if (!test_override &&
        (command->generation != 2 ||
         ordinal < kCompositorNeutralizeFirstPresent)) {
        return false;
    }
    if (!indexed) {
        compositor_neutralize_latch_locked(
            TESO4M4_COMPOSITOR_NEUTRALIZE_ABORTED,
            "unexpected-non-indexed-target", command, ordinal, 0);
        return false;
    }
    if (!test_override && ordinal > kCompositorNeutralizeLastPresent) {
        if (g_compositor_neutralize_state ==
            TESO4M4_COMPOSITOR_NEUTRALIZE_SUPPRESSING) {
            compositor_neutralize_latch_locked(
                TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING,
                "present-deadline", command, ordinal,
                g_compositor_placeholder_signature);
        }
        return false;
    }
    FramebufferRecord* framebuffer = find_framebuffer(command->framebuffer);
    uint64_t descriptor_signature = 0;
    if (!g_next_cmd_clear_attachments || !framebuffer ||
        framebuffer->width == 0 || framebuffer->height == 0 ||
        !compositor_descriptor_signature_locked(
            command, test_override, &descriptor_signature)) {
        compositor_neutralize_latch_locked(
            TESO4M4_COMPOSITOR_NEUTRALIZE_ABORTED,
            "incomplete-target-state", command, ordinal,
            descriptor_signature);
        return false;
    }
    /* The first complete target state is the bounded placeholder baseline. */
    if (g_compositor_neutralize_state ==
        TESO4M4_COMPOSITOR_NEUTRALIZE_ARMED) {
        g_compositor_placeholder_signature = descriptor_signature;
        g_compositor_neutralize_state =
            TESO4M4_COMPOSITOR_NEUTRALIZE_SUPPRESSING;
    } else if (descriptor_signature !=
               g_compositor_placeholder_signature) {
        compositor_neutralize_latch_locked(
            TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING,
            "descriptor-transition", command, ordinal,
            descriptor_signature);
        return false;
    }
    if (g_compositor_suppressed_draw_count >=
            kCompositorNeutralizeMaxSuppressedDraws ||
        (!test_override &&
         ordinal >= kCompositorNeutralizeLastPresent)) {
        compositor_neutralize_latch_locked(
            TESO4M4_COMPOSITOR_NEUTRALIZE_FORWARDING,
            "suppression-deadline", command, ordinal,
            descriptor_signature);
        return false;
    }
    ++g_compositor_suppressed_draw_count;
    *width_out = framebuffer->width;
    *height_out = framebuffer->height;
    startup_color_detail_log(
        "STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS: generation=%" PRIu64
        " ordinal=%u pipeline=%016" PRIx64
        " descriptor_update_signature=%016" PRIx64 " draw=%u",
        command->generation,
        ordinal,
        command->bound_pipeline_signature,
        descriptor_signature,
        g_compositor_suppressed_draw_count);
    return true;
}

static void record_draw_locked(
    VkCommandBuffer command_buffer,
    bool indexed,
    uint32_t count,
    uint32_t instance_count,
    uint32_t first,
    int32_t vertex_offset,
    uint32_t first_instance) {
    CommandBufferRecord* command = find_command_buffer(command_buffer);
    if (!command || !command->in_render_pass || command->generation == 0) {
        return;
    }
    ++command->draw_count;
    command->indexed_draw_count += indexed ? 1 : 0;
    if (command->first_pipeline_signature == 0) {
        command->first_pipeline_signature =
            command->bound_pipeline_signature;
    }
    command->last_pipeline_signature = command->bound_pipeline_signature;
    add_command_pipeline_signature(
        command, command->bound_pipeline_signature);
    uint64_t signature = command->draw_signature == 0
        ? UINT64_C(1469598103934665603)
        : command->draw_signature;
    signature = hash_mix(signature, indexed ? 1 : 0);
    signature = hash_mix(signature, command->bound_pipeline_signature);
    signature = hash_mix(signature, count);
    signature = hash_mix(signature, instance_count);
    signature = hash_mix(signature, first);
    signature = hash_mix(signature, (uint32_t)vertex_offset);
    signature = hash_mix(signature, first_instance);
    command->draw_signature = signature;
    if (atomic_load(&g_startup_input_audit)) {
        memset(
            command->draw_set_layout_signatures, 0,
            sizeof(command->draw_set_layout_signatures));
        memset(
            command->draw_set_image_update_signatures, 0,
            sizeof(command->draw_set_image_update_signatures));
        memset(
            command->draw_set_buffer_update_signatures, 0,
            sizeof(command->draw_set_buffer_update_signatures));
        memset(
            command->draw_set_image_update_calls, 0,
            sizeof(command->draw_set_image_update_calls));
        memset(
            command->draw_set_buffer_update_calls, 0,
            sizeof(command->draw_set_buffer_update_calls));
        memset(
            command->draw_set_image_update_counts, 0,
            sizeof(command->draw_set_image_update_counts));
        memset(
            command->draw_set_buffer_update_counts, 0,
            sizeof(command->draw_set_buffer_update_counts));
        memset(
            command->draw_set_expected_image_counts, 0,
            sizeof(command->draw_set_expected_image_counts));
        memset(
            command->draw_set_expected_buffer_counts, 0,
            sizeof(command->draw_set_expected_buffer_counts));
        memset(
            command->draw_descriptor_images, 0,
            sizeof(command->draw_descriptor_images));
        uint64_t layout_signature = UINT64_C(1469598103934665603);
        uint64_t handle_signature = UINT64_C(1469598103934665603);
        uint64_t update_signature = UINT64_C(1469598103934665603);
        bool complete = command->bound_pipeline_layout_complete;
        uint32_t bound_count = 0;
        const uint32_t required_set_count =
            command->bound_pipeline_set_layout_count;
        if (required_set_count > kMaxBoundDescriptorSets) {
            complete = false;
        }
        const uint32_t tracked_set_count =
            required_set_count < kMaxBoundDescriptorSets
                ? required_set_count
                : kMaxBoundDescriptorSets;
        for (uint32_t slot = 0; slot < tracked_set_count; ++slot) {
            DescriptorSetRecord* set =
                find_descriptor_set(command->bound_sets[slot], false);
            const bool known = set && set->alive;
            const uint64_t set_layout = known ? set->layout_signature : 0;
            const uint64_t set_update = known ? set->update_signature : 0;
            command->draw_set_layout_signatures[slot] = set_layout;
            command->draw_set_image_update_signatures[slot] =
                known ? set->image_update_signature : 0;
            command->draw_set_buffer_update_signatures[slot] =
                known ? set->buffer_update_signature : 0;
            command->draw_set_image_update_calls[slot] =
                known ? set->last_image_update_call : 0;
            command->draw_set_buffer_update_calls[slot] =
                known ? set->last_buffer_update_call : 0;
            command->draw_set_image_update_counts[slot] =
                known ? set->image_update_count : 0;
            command->draw_set_buffer_update_counts[slot] =
                known ? set->buffer_update_count : 0;
            command->draw_set_expected_image_counts[slot] =
                known ? set->expected_image_count : 0;
            command->draw_set_expected_buffer_counts[slot] =
                known ? set->expected_buffer_count : 0;
            if (known) {
                memcpy(
                    command->draw_descriptor_images[slot], set->images,
                    sizeof(command->draw_descriptor_images[slot]));
            }
            layout_signature = hash_mix(layout_signature, slot);
            layout_signature = hash_mix(layout_signature, set_layout);
            handle_signature = hash_mix(handle_signature, slot);
            handle_signature = hash_mix(
                handle_signature,
                (uint64_t)(uintptr_t)command->bound_sets[slot]);
            update_signature = hash_mix(update_signature, slot);
            update_signature = hash_mix(update_signature, set_update);
            update_signature = hash_mix(
                update_signature, known ? set->update_count : 0);
            complete &=
                known && set_layout != 0 &&
                set_layout ==
                    command->bound_pipeline_set_layout_signatures[slot] &&
                (command->bound_pipeline_set_descriptor_counts[slot] == 0 ||
                 set_update != 0);
            complete &= known && set->class_complete &&
                (set->expected_image_count == 0 ||
                 set->image_update_signature != 0) &&
                (set->expected_buffer_count == 0 ||
                 set->buffer_update_signature != 0);
            uint32_t tracked_images = 0;
            for (uint32_t image_index = 0;
                 known && image_index < kMaxTrackedImageBindings;
                 ++image_index) {
                tracked_images += set->images[image_index].valid ? 1u : 0u;
            }
            complete &= !known ||
                tracked_images == set->expected_image_count;
            ++bound_count;
        }
        complete &=
            command->bound_pipeline_push_constant_bytes == 0 ||
            command->push_constant_signature != 0;
        handle_signature = hash_mix(
            handle_signature, command->dynamic_offset_signature);
        command->draw_descriptor_layout_signature = hash_mix(
            command->bound_pipeline_layout_signature, layout_signature);
        command->draw_descriptor_handle_signature = handle_signature;
        command->draw_descriptor_update_signature = update_signature;
        command->draw_push_constant_signature =
            command->push_constant_signature;
        command->draw_push_constant_bytes = command->push_constant_bytes;
        command->draw_bound_set_count = bound_count;
        command->draw_input_complete = complete;
        command->draw_descriptor_class_complete = complete;
    }
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_draw(
    VkCommandBuffer command_buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) {
    if (!g_next_cmd_draw) {
        return;
    }
    if (atomic_load(&g_startup_compositor_neutralize) &&
        !startup_audit_finished()) {
        uint32_t ignored_width = 0;
        uint32_t ignored_height = 0;
        pthread_mutex_lock(&g_lock);
        (void)should_suppress_compositor_draw_locked(
            command_buffer, false, &ignored_width, &ignored_height);
        pthread_mutex_unlock(&g_lock);
    }
    g_next_cmd_draw(
        command_buffer, vertex_count, instance_count,
        first_vertex, first_instance);
    if (atomic_load(&g_startup_draw_audit) && !startup_audit_finished()) {
        pthread_mutex_lock(&g_lock);
        record_draw_locked(
            command_buffer, false, vertex_count, instance_count,
            first_vertex, 0, first_instance);
        pthread_mutex_unlock(&g_lock);
    }
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_draw_indexed(
    VkCommandBuffer command_buffer,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance) {
    if (!g_next_cmd_draw_indexed) {
        return;
    }
    if (atomic_load(&g_startup_compositor_neutralize) &&
        !startup_audit_finished()) {
        uint32_t width = 0;
        uint32_t height = 0;
        pthread_mutex_lock(&g_lock);
        const bool suppress = should_suppress_compositor_draw_locked(
            command_buffer, true, &width, &height);
        pthread_mutex_unlock(&g_lock);
        if (suppress) {
            const VkClearAttachment attachment = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .colorAttachment = 0,
                .clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}},
            };
            const VkClearRect rect = {
                .rect = {{0, 0}, {width, height}},
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            g_next_cmd_clear_attachments(
                command_buffer, 1, &attachment, 1, &rect);
            return;
        }
    }
    g_next_cmd_draw_indexed(
        command_buffer, index_count, instance_count, first_index,
        vertex_offset, first_instance);
    if (atomic_load(&g_startup_draw_audit) && !startup_audit_finished()) {
        pthread_mutex_lock(&g_lock);
        record_draw_locked(
            command_buffer, true, index_count, instance_count,
            first_index, vertex_offset, first_instance);
        pthread_mutex_unlock(&g_lock);
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_device_wait_idle(
    VkDevice device) {
    if (!g_next_device_wait_idle) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_device_wait_idle(device);
    }
    VkResult result = g_next_device_wait_idle(device);
    pthread_mutex_lock(&g_lock);
    const uint64_t call = ++g_wait_counter;
    pthread_mutex_unlock(&g_lock);
    lifecycle_log("DEVICE_WAIT_IDLE: call=%" PRIu64 " device=%p result=%d",
                  call, (void*)device, result);
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
    if (startup_audit_finished()) {
        return g_next_create_swapchain(
            device, create_info, allocator, swapchain);
    }
    VkResult result =
        g_next_create_swapchain(device, create_info, allocator, swapchain);

    uint64_t generation = 0;
    uint64_t old_generation = 0;
    if (result == VK_SUCCESS && swapchain &&
        *swapchain != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        SwapchainRecord* old_record =
            create_info ? find_swapchain(create_info->oldSwapchain) : NULL;
        old_generation = old_record ? old_record->generation : 0;
        generation = ++g_generation_counter;
        add_swapchain(*swapchain, generation, create_info);
        pthread_mutex_unlock(&g_lock);
    }

    lifecycle_log(
        "SWAPCHAIN_CREATE: device=%p old=%p old_generation=%" PRIu64
        " extent=%ux%u min_images=%u format=%d color_space=%d present_mode=%d"
        " result=%d swapchain=%p generation=%" PRIu64,
        (void*)device,
        create_info ? (void*)create_info->oldSwapchain : NULL,
        old_generation,
        create_info ? create_info->imageExtent.width : 0,
        create_info ? create_info->imageExtent.height : 0,
        create_info ? create_info->minImageCount : 0,
        create_info ? create_info->imageFormat : 0,
        create_info ? create_info->imageColorSpace : 0,
        create_info ? create_info->presentMode : 0,
        result,
        swapchain ? (void*)*swapchain : NULL,
        generation);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_swapchain(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_swapchain) {
        lifecycle_log("LIFECYCLE_ERROR: destroy swapchain target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_destroy_swapchain(device, swapchain, allocator);
        return;
    }
    g_next_destroy_swapchain(device, swapchain, allocator);

    uint64_t generation = 0;
    uint32_t live_images = 0;
    uint32_t live_views = 0;
    uint32_t live_framebuffers = 0;
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = find_swapchain(swapchain);
    if (record) {
        generation = record->generation;
        record->alive = false;
    }
    for (size_t index = 0; index < kMaxSwapchainImages; ++index) {
        if (g_images[index].alive &&
            g_images[index].generation == generation) {
            ++live_images;
            g_images[index].alive = false;
        }
    }
    for (size_t index = 0; index < kMaxImageViews; ++index) {
        if (g_image_views[index].alive &&
            g_image_views[index].generation == generation) {
            ++live_views;
        }
    }
    for (size_t index = 0; index < kMaxFramebuffers; ++index) {
        if (g_framebuffers[index].alive &&
            g_framebuffers[index].generation == generation) {
            ++live_framebuffers;
        }
    }
    pthread_mutex_unlock(&g_lock);
    lifecycle_log(
        "SWAPCHAIN_DESTROY: device=%p swapchain=%p generation=%" PRIu64
        " live_images=%u live_views=%u live_framebuffers=%u",
        (void*)device, (void*)swapchain, generation, live_images, live_views,
        live_framebuffers);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_get_swapchain_images(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* image_count,
    VkImage* images) {
    if (!g_next_get_swapchain_images) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_get_swapchain_images(
            device, swapchain, image_count, images);
    }
    const uint32_t capacity = image_count ? *image_count : 0;
    VkResult result =
        g_next_get_swapchain_images(device, swapchain, image_count, images);

    uint64_t generation = 0;
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = find_swapchain(swapchain);
    generation = record ? record->generation : 0;
    if (generation != 0 && images && image_count &&
        (result == VK_SUCCESS || result == VK_INCOMPLETE)) {
        for (uint32_t index = 0; index < *image_count; ++index) {
            add_image(images[index], swapchain, generation, index);
        }
    }
    pthread_mutex_unlock(&g_lock);
    lifecycle_log(
        "SWAPCHAIN_IMAGES: device=%p swapchain=%p generation=%" PRIu64
        " query=%s capacity=%u count=%u result=%d",
        (void*)device, (void*)swapchain, generation,
        images ? "data" : "count", capacity, image_count ? *image_count : 0,
        result);
    if (images && image_count) {
        for (uint32_t index = 0; index < *image_count; ++index) {
            lifecycle_log(
                "SWAPCHAIN_IMAGE: swapchain=%p generation=%" PRIu64
                " index=%u image=%p",
                (void*)swapchain, generation, index, (void*)images[index]);
        }
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_image_view(
    VkDevice device,
    const VkImageViewCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkImageView* image_view) {
    if (!g_next_create_image_view) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_create_image_view(
            device, create_info, allocator, image_view);
    }
    VkResult result =
        g_next_create_image_view(device, create_info, allocator, image_view);
    uint64_t generation = 0;
    if (result == VK_SUCCESS && create_info && image_view &&
        *image_view != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        ImageRecord* image = find_image(create_info->image);
        if (image) {
            generation = image->generation;
        }
        if (image || atomic_load(&g_startup_input_audit)) {
            add_image_view(*image_view, generation, create_info);
        }
        pthread_mutex_unlock(&g_lock);
    }
    if (generation != 0) {
        lifecycle_log(
            "SWAPCHAIN_IMAGE_VIEW_CREATE: device=%p generation=%" PRIu64
            " image=%p view=%p view_type=%d format=%d"
            " components=%d,%d,%d,%d aspect=0x%x"
            " base_mip=%u level_count=%u base_layer=%u layer_count=%u"
            " result=%d",
            (void*)device, generation, (void*)create_info->image,
            (void*)*image_view, create_info->viewType, create_info->format,
            create_info->components.r, create_info->components.g,
            create_info->components.b, create_info->components.a,
            create_info->subresourceRange.aspectMask,
            create_info->subresourceRange.baseMipLevel,
            create_info->subresourceRange.levelCount,
            create_info->subresourceRange.baseArrayLayer,
            create_info->subresourceRange.layerCount, result);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_image_view(
    VkDevice device,
    VkImageView image_view,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_image_view) {
        lifecycle_log("LIFECYCLE_ERROR: destroy image view target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_destroy_image_view(device, image_view, allocator);
        return;
    }
    g_next_destroy_image_view(device, image_view, allocator);
    uint64_t generation = 0;
    pthread_mutex_lock(&g_lock);
    ImageViewRecord* record = find_image_view(image_view);
    if (record) {
        generation = record->generation;
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
    if (generation != 0) {
        lifecycle_log(
            "SWAPCHAIN_IMAGE_VIEW_DESTROY: device=%p generation=%" PRIu64
            " view=%p",
            (void*)device, generation, (void*)image_view);
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_render_pass(
    VkDevice device,
    const VkRenderPassCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkRenderPass* render_pass) {
    if (!g_next_create_render_pass) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_create_render_pass(
            device, create_info, allocator, render_pass);
    }
    VkResult result =
        g_next_create_render_pass(device, create_info, allocator, render_pass);
    if (result == VK_SUCCESS && render_pass &&
        *render_pass != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        add_render_pass(*render_pass, create_info);
        pthread_mutex_unlock(&g_lock);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_render_pass(
    VkDevice device,
    VkRenderPass render_pass,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_render_pass) {
        lifecycle_log("LIFECYCLE_ERROR: destroy render pass target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_destroy_render_pass(device, render_pass, allocator);
        return;
    }
    g_next_destroy_render_pass(device, render_pass, allocator);
    uint64_t first_generation = 0;
    uint64_t last_generation = 0;
    uint32_t link_count = 0;
    pthread_mutex_lock(&g_lock);
    RenderPassRecord* record = find_render_pass(render_pass);
    if (record) {
        first_generation = record->first_generation;
        last_generation = record->last_generation;
        link_count = record->link_count;
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
    if (first_generation != 0) {
        lifecycle_log(
            "SWAPCHAIN_RENDER_PASS_DESTROY: device=%p first_generation=%" PRIu64
            " last_generation=%" PRIu64 " links=%u render_pass=%p",
            (void*)device, first_generation, last_generation, link_count,
            (void*)render_pass);
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_create_framebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* create_info,
    const VkAllocationCallbacks* allocator,
    VkFramebuffer* framebuffer) {
    if (!g_next_create_framebuffer) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_create_framebuffer(
            device, create_info, allocator, framebuffer);
    }
    VkResult result =
        g_next_create_framebuffer(device, create_info, allocator, framebuffer);
    uint64_t generation = 0;
    uint32_t tracked_attachments = 0;
    bool mixed_generations = false;
    if (result == VK_SUCCESS && create_info && framebuffer &&
        *framebuffer != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        for (uint32_t index = 0; index < create_info->attachmentCount; ++index) {
            ImageViewRecord* view =
                find_image_view(create_info->pAttachments[index]);
            if (!view) {
                continue;
            }
            ++tracked_attachments;
            if (generation == 0) {
                generation = view->generation;
            } else if (generation != view->generation) {
                mixed_generations = true;
            }
        }
        if (generation != 0) {
            add_framebuffer(
                *framebuffer, create_info->renderPass, generation,
                create_info->width, create_info->height);
            RenderPassRecord* render_pass =
                find_render_pass(create_info->renderPass);
            if (render_pass) {
                if (render_pass->first_generation == 0) {
                    render_pass->first_generation = generation;
                }
                render_pass->last_generation = generation;
                ++render_pass->link_count;
            }
        }
        pthread_mutex_unlock(&g_lock);
    }
    if (generation != 0) {
        lifecycle_log(
            "SWAPCHAIN_FRAMEBUFFER_CREATE: device=%p generation=%" PRIu64
            " framebuffer=%p render_pass=%p attachments=%u tracked=%u"
            " mixed_generations=%s extent=%ux%u result=%d",
            (void*)device, generation, (void*)*framebuffer,
            (void*)create_info->renderPass, create_info->attachmentCount,
            tracked_attachments, mixed_generations ? "yes" : "no",
            create_info->width, create_info->height, result);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_framebuffer(
    VkDevice device,
    VkFramebuffer framebuffer,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_framebuffer) {
        lifecycle_log("LIFECYCLE_ERROR: destroy framebuffer target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_destroy_framebuffer(device, framebuffer, allocator);
        return;
    }
    g_next_destroy_framebuffer(device, framebuffer, allocator);
    uint64_t generation = 0;
    pthread_mutex_lock(&g_lock);
    FramebufferRecord* record = find_framebuffer(framebuffer);
    if (record) {
        generation = record->generation;
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
    if (generation != 0) {
        lifecycle_log(
            "SWAPCHAIN_FRAMEBUFFER_DESTROY: device=%p generation=%" PRIu64
            " framebuffer=%p",
            (void*)device, generation, (void*)framebuffer);
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_acquire_next_image(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* image_index) {
    if (!g_next_acquire_next_image) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_acquire_next_image(
            device, swapchain, timeout, semaphore, fence, image_index);
    }
    VkResult result = g_next_acquire_next_image(
        device, swapchain, timeout, semaphore, fence, image_index);
    uint64_t generation = 0;
    uint32_t ordinal = 0;
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = find_swapchain(swapchain);
    if (record) {
        generation = record->generation;
        ordinal = ++record->acquire_count;
    }
    pthread_mutex_unlock(&g_lock);
    if (ordinal <= kFirstPresentationLimit ||
        (!atomic_load(&g_startup_color_audit) && result != VK_SUCCESS)) {
        lifecycle_log(
            "SWAPCHAIN_ACQUIRE: device=%p swapchain=%p generation=%" PRIu64
            " ordinal=%u image_index=%u result=%d",
            (void*)device, (void*)swapchain, generation, ordinal,
            image_index ? *image_index : UINT32_MAX, result);
    }
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
    if (startup_audit_finished()) {
        return g_next_queue_submit(queue, submit_count, submits, fence);
    }
    VkResult result =
        g_next_queue_submit(queue, submit_count, submits, fence);
    if (result == VK_SUCCESS && submits) {
        pthread_mutex_lock(&g_lock);
        for (uint32_t submit_index = 0; submit_index < submit_count;
             ++submit_index) {
            DrawSubmissionSummary draw_summary = {0};
            for (uint32_t command_index = 0;
                 command_index < submits[submit_index].commandBufferCount;
                 ++command_index) {
                CommandBufferRecord* command = find_command_buffer(
                    submits[submit_index].pCommandBuffers[command_index]);
                if (command) {
                    merge_submission_command(&draw_summary, command);
                }
            }
            for (uint32_t wait_index = 0;
                 wait_index < submits[submit_index].waitSemaphoreCount;
                 ++wait_index) {
                forget_signaled_semaphore(
                    submits[submit_index].pWaitSemaphores[wait_index]);
            }
            for (uint32_t signal_index = 0;
                 signal_index < submits[submit_index].signalSemaphoreCount;
                 ++signal_index) {
                remember_signaled_semaphore(
                    submits[submit_index].pSignalSemaphores[signal_index],
                    queue, &draw_summary);
            }
        }
        pthread_mutex_unlock(&g_lock);
    }
    if (!atomic_load(&g_startup_color_audit) ||
        atomic_load(&g_startup_color_audit_finished) || !submits) {
        return result;
    }
    for (uint32_t submit_index = 0; submit_index < submit_count;
         ++submit_index) {
        for (uint32_t command_index = 0;
             command_index < submits[submit_index].commandBufferCount;
             ++command_index) {
            const VkCommandBuffer handle =
                submits[submit_index].pCommandBuffers[command_index];
            CommandBufferRecord command = {0};
            bool tracked = false;
            pthread_mutex_lock(&g_lock);
            CommandBufferRecord* record = find_command_buffer(handle);
            if (record && record->generation >= 1 &&
                record->generation <= kStartupAuditGenerationLimit) {
                command = *record;
                tracked = true;
            }
            pthread_mutex_unlock(&g_lock);
            if (tracked) {
                startup_color_detail_log(
                    "STARTUP_COLOR_SUBMIT: generation=%" PRIu64 " queue=%p"
                    " submit=%u command_index=%u command_buffer=%p"
                    " framebuffer=%p signal_count=%u result=%d",
                    command.generation, (void*)queue, submit_index,
                    command_index, (void*)handle,
                    (void*)command.framebuffer,
                    submits[submit_index].signalSemaphoreCount, result);
            }
        }
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_queue_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    if (!g_next_queue_present) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (startup_audit_finished()) {
        return g_next_queue_present(queue, present_info);
    }
    if (atomic_load(&g_startup_present_pixel_audit) && present_info) {
        for (uint32_t index = 0; index < present_info->swapchainCount;
             ++index) {
            const VkSwapchainKHR swapchain = present_info->pSwapchains[index];
            const uint32_t image_index = present_info->pImageIndices
                ? present_info->pImageIndices[index]
                : UINT32_MAX;
            SwapchainRecord swapchain_snapshot = {0};
            ImageRecord image_snapshot = {0};
            bool found_swapchain = false;
            bool found_image = false;
            bool same_queue_ready = true;
            uint32_t matched_signals = 0;
            DrawSubmissionSummary present_draw = {0};
            Teso4m4PresentPixelSampler sampler = NULL;
            Teso4m4CompositorImageSampler compositor_sampler = NULL;
            pthread_mutex_lock(&g_lock);
            SwapchainRecord* record = find_swapchain(swapchain);
            if (record) {
                swapchain_snapshot = *record;
                found_swapchain = true;
            }
            ImageRecord* image = find_swapchain_image(
                swapchain, image_index);
            if (image) {
                image_snapshot = *image;
                found_image = true;
            }
            for (uint32_t wait_index = 0;
                 wait_index < present_info->waitSemaphoreCount;
                 ++wait_index) {
                SignaledSemaphoreRecord* signal = find_signaled_semaphore(
                    present_info->pWaitSemaphores[wait_index]);
                if (!signal || signal->queue != queue) {
                    same_queue_ready = false;
                    break;
                }
                ++matched_signals;
                merge_present_signal(&present_draw, signal);
            }
            sampler = g_present_pixel_sampler;
            compositor_sampler = g_compositor_image_sampler;
            pthread_mutex_unlock(&g_lock);

            const uint32_t ordinal = found_swapchain
                ? swapchain_snapshot.present_count + 1
                : 0;
            if (!should_sample_present_pixel(
                    swapchain_snapshot.generation, ordinal)) {
                continue;
            }
            if (!found_image || !same_queue_ready || !sampler) {
                lifecycle_log(
                    "STARTUP_PRESENT_PIXEL_SKIP: generation=%" PRIu64
                    " ordinal=%u image_index=%u image=%s"
                    " synchronization=%s sampler=%s",
                    swapchain_snapshot.generation, ordinal, image_index,
                    found_image ? "tracked" : "missing",
                    same_queue_ready ? "same-queue" : "unconfirmed",
                    sampler ? "ready" : "missing");
                continue;
            }
            if (atomic_load(&g_startup_draw_audit)) {
                lifecycle_log(
                    "STARTUP_PRESENT_DRAW_SUMMARY: generation=%" PRIu64
                    " ordinal=%u image_index=%u wait_count=%u"
                    " matched_signals=%u tracked_commands=%u"
                    " draw_count=%u indexed_draw_count=%u"
                    " distinct_pipelines=%u pipeline_overflow=%s"
                    " draw_signature=%016" PRIx64
                    " first_pipeline=%016" PRIx64
                    " last_pipeline=%016" PRIx64
                    " command_buffer=%p framebuffer=%p",
                    swapchain_snapshot.generation, ordinal, image_index,
                    present_info->waitSemaphoreCount, matched_signals,
                    present_draw.tracked_command_count,
                    present_draw.draw_count,
                    present_draw.indexed_draw_count,
                    present_draw.distinct_pipeline_count,
                    present_draw.pipeline_overflow ? "yes" : "no",
                    present_draw.draw_signature,
                    present_draw.first_pipeline_signature,
                    present_draw.last_pipeline_signature,
                    (void*)present_draw.command_buffer,
                    (void*)present_draw.framebuffer);
                for (uint32_t pipeline_index = 0;
                     pipeline_index < present_draw.distinct_pipeline_count;
                     ++pipeline_index) {
                    GraphicsPipelineRecord pipeline_snapshot = {0};
                    bool found_pipeline = false;
                    pthread_mutex_lock(&g_lock);
                    for (size_t pipeline = 0;
                         pipeline < kMaxGraphicsPipelines; ++pipeline) {
                        if (g_graphics_pipelines[pipeline].alive &&
                            g_graphics_pipelines[pipeline].signature ==
                                present_draw.pipeline_signatures[
                                    pipeline_index]) {
                            pipeline_snapshot = g_graphics_pipelines[pipeline];
                            found_pipeline = true;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&g_lock);
                    lifecycle_log(
                        "STARTUP_PRESENT_DRAW_PIPELINE: generation=%" PRIu64
                        " ordinal=%u pipeline_index=%u signature=%016" PRIx64
                        " vertex_hash=%016" PRIx64
                        " fragment_hash=%016" PRIx64
                        " shader_hash_complete=%s pipeline_state=%s",
                        swapchain_snapshot.generation, ordinal,
                        pipeline_index,
                        present_draw.pipeline_signatures[pipeline_index],
                        pipeline_snapshot.vertex_shader_hash,
                        pipeline_snapshot.fragment_shader_hash,
                        found_pipeline && pipeline_snapshot.shader_hash_complete
                            ? "yes" : "no",
                        found_pipeline ? "tracked" : "missing");
                    if (atomic_load(&g_startup_input_audit)) {
                        lifecycle_log(
                            "STARTUP_PRESENT_INPUT_PIPELINE: generation=%" PRIu64
                            " ordinal=%u pipeline_index=%u"
                            " pipeline_signature=%016" PRIx64
                            " layout_signature=%016" PRIx64
                            " set_layouts=%u descriptors=%u images=%u"
                            " buffers=%u samplers=%u input_attachments=%u"
                            " push_bytes=%u layout_complete=%s",
                            swapchain_snapshot.generation, ordinal,
                            pipeline_index,
                            present_draw.pipeline_signatures[pipeline_index],
                            pipeline_snapshot.layout_signature,
                            pipeline_snapshot.set_layout_count,
                            pipeline_snapshot.descriptor_count,
                            pipeline_snapshot.image_count,
                            pipeline_snapshot.buffer_count,
                            pipeline_snapshot.sampler_count,
                            pipeline_snapshot.input_attachment_count,
                            pipeline_snapshot.push_constant_bytes,
                            found_pipeline && pipeline_snapshot.layout_complete
                                ? "yes" : "no");
                    }
                }
                if (atomic_load(&g_startup_input_audit)) {
                    lifecycle_log(
                        "STARTUP_PRESENT_DRAW_INPUT: generation=%" PRIu64
                        " ordinal=%u bound_sets=%u"
                        " descriptor_layout_signature=%016" PRIx64
                        " descriptor_handle_signature=%016" PRIx64
                        " descriptor_update_signature=%016" PRIx64
                        " push_signature=%016" PRIx64
                        " push_bytes=%u input_complete=%s",
                        swapchain_snapshot.generation, ordinal,
                        present_draw.bound_set_count,
                        present_draw.descriptor_layout_signature,
                        present_draw.descriptor_handle_signature,
                        present_draw.descriptor_update_signature,
                        present_draw.push_constant_signature,
                        present_draw.push_constant_bytes,
                        present_draw.input_complete ? "yes" : "no");
                    const uint32_t logged_set_count =
                        present_draw.bound_set_count <
                                kMaxBoundDescriptorSets
                            ? present_draw.bound_set_count
                            : kMaxBoundDescriptorSets;
                    for (uint32_t slot = 0; slot < logged_set_count;
                         ++slot) {
                        lifecycle_log(
                            "STARTUP_PRESENT_DESCRIPTOR_CLASS:"
                            " generation=%" PRIu64 " ordinal=%u slot=%u"
                            " layout_signature=%016" PRIx64
                            " expected_images=%u expected_buffers=%u"
                            " image_update_signature=%016" PRIx64
                            " image_update_writes=%" PRIu64
                            " image_update_call=%" PRIu64
                            " buffer_update_signature=%016" PRIx64
                            " buffer_update_writes=%" PRIu64
                            " buffer_update_call=%" PRIu64
                            " class_complete=%s",
                            swapchain_snapshot.generation, ordinal, slot,
                            present_draw
                                .descriptor_set_layout_signatures[slot],
                            present_draw
                                .descriptor_expected_image_counts[slot],
                            present_draw
                                .descriptor_expected_buffer_counts[slot],
                            present_draw
                                .descriptor_image_update_signatures[slot],
                            present_draw
                                .descriptor_image_update_counts[slot],
                            present_draw
                                .descriptor_image_update_calls[slot],
                            present_draw
                                .descriptor_buffer_update_signatures[slot],
                            present_draw
                                .descriptor_buffer_update_counts[slot],
                            present_draw
                                .descriptor_buffer_update_calls[slot],
                            present_draw.descriptor_class_complete
                                ? "yes"
                                : "no");
                    }
                }
            }
            if (!sampler(
                    queue, image_snapshot.handle, swapchain_snapshot.format,
                    swapchain_snapshot.width, swapchain_snapshot.height,
                    swapchain_snapshot.generation, ordinal, image_index)) {
                lifecycle_log(
                    "STARTUP_PRESENT_PIXEL_SKIP: generation=%" PRIu64
                    " ordinal=%u image_index=%u image=tracked"
                    " synchronization=same-queue sampler=failed",
                    swapchain_snapshot.generation, ordinal, image_index);
            }
            if (atomic_load(&g_startup_compositor_audit)) {
                for (uint32_t set_slot = 0;
                     set_slot < present_draw.bound_set_count &&
                     set_slot < kMaxBoundDescriptorSets;
                     ++set_slot) {
                    uint32_t ordered[kMaxTrackedImageBindings] = {0};
                    uint32_t ordered_count = 0;
                    for (uint32_t state_index = 0;
                         state_index < kMaxTrackedImageBindings;
                         ++state_index) {
                        if (present_draw.descriptor_images
                                [set_slot][state_index].valid) {
                            ordered[ordered_count++] = state_index;
                        }
                    }
                    for (uint32_t left = 0; left < ordered_count; ++left) {
                        for (uint32_t right = left + 1;
                             right < ordered_count; ++right) {
                            const DescriptorImageBindingState* a =
                                &present_draw.descriptor_images
                                    [set_slot][ordered[left]];
                            const DescriptorImageBindingState* b =
                                &present_draw.descriptor_images
                                    [set_slot][ordered[right]];
                            if (b->binding < a->binding ||
                                (b->binding == a->binding &&
                                 b->array_element < a->array_element)) {
                                const uint32_t temporary = ordered[left];
                                ordered[left] = ordered[right];
                                ordered[right] = temporary;
                            }
                        }
                    }
                    for (uint32_t image_ordinal = 0;
                         image_ordinal < ordered_count;
                         ++image_ordinal) {
                        const DescriptorImageBindingState* state =
                            &present_draw.descriptor_images
                                [set_slot][ordered[image_ordinal]];
                        lifecycle_log(
                            "STARTUP_PRESENT_COMPOSITOR_IMAGE:"
                            " generation=%" PRIu64 " ordinal=%u"
                            " set_slot=%u image_ordinal=%u binding=%u"
                            " array_element=%u signature=%016" PRIx64
                            " update_call=%" PRIu64 " view=%p image=%p"
                            " format=%d view_type=%d mip=%u layer=%u"
                            " layout=%d",
                            swapchain_snapshot.generation, ordinal,
                            set_slot, image_ordinal, state->binding,
                            state->array_element, state->signature,
                            state->update_call, (void*)state->view,
                            (void*)state->image, state->format,
                            state->view_type, state->base_mip_level,
                            state->base_array_layer, state->layout);
                        if (!compositor_sampler ||
                            !compositor_sampler(
                                queue, state->image, state->format,
                                state->view_type, state->base_mip_level,
                                state->base_array_layer,
                                swapchain_snapshot.generation, ordinal,
                                set_slot, state->binding,
                                state->array_element, image_ordinal)) {
                            lifecycle_log(
                                "STARTUP_PRESENT_COMPOSITOR_IMAGE_SKIP:"
                                " generation=%" PRIu64 " ordinal=%u"
                                " set_slot=%u image_ordinal=%u binding=%u"
                                " sampler=%s",
                                swapchain_snapshot.generation, ordinal,
                                set_slot, image_ordinal, state->binding,
                                compositor_sampler ? "failed" : "missing");
                        }
                    }
                }
            }
        }
    }
    VkResult result = g_next_queue_present(queue, present_info);
    if (!present_info) {
        lifecycle_log("SWAPCHAIN_PRESENT: queue=%p info=NULL result=%d",
                      (void*)queue, result);
        return result;
    }
    pthread_mutex_lock(&g_lock);
    for (uint32_t wait_index = 0;
         wait_index < present_info->waitSemaphoreCount; ++wait_index) {
        SignaledSemaphoreRecord* signal = find_signaled_semaphore(
            present_info->pWaitSemaphores[wait_index]);
        if (signal) {
            signal->occupied = false;
        }
    }
    pthread_mutex_unlock(&g_lock);
    bool finish_startup_audit = false;
    for (uint32_t index = 0; index < present_info->swapchainCount; ++index) {
        const VkSwapchainKHR swapchain = present_info->pSwapchains[index];
        uint64_t generation = 0;
        uint32_t ordinal = 0;
        pthread_mutex_lock(&g_lock);
        SwapchainRecord* record = find_swapchain(swapchain);
        if (record) {
            generation = record->generation;
            ordinal = ++record->present_count;
        }
        pthread_mutex_unlock(&g_lock);
        const VkResult item_result =
            present_info->pResults ? present_info->pResults[index] : result;
        const bool startup_audit = atomic_load(&g_startup_color_audit);
        if (ordinal <= kFirstPresentationLimit ||
            (!startup_audit &&
             (result != VK_SUCCESS || item_result != VK_SUCCESS))) {
            lifecycle_log(
                "SWAPCHAIN_PRESENT: queue=%p swapchain=%p generation=%" PRIu64
                " ordinal=%u image_index=%u result=%d item_result=%d",
                (void*)queue, (void*)swapchain, generation, ordinal,
                present_info->pImageIndices ?
                    present_info->pImageIndices[index] : UINT32_MAX,
                result, item_result);
        }
        if (startup_audit &&
            generation == kStartupAuditGenerationLimit &&
            ordinal >= kStartupAuditGeneration2PresentLimit) {
            finish_startup_audit = true;
        }
    }
    if (finish_startup_audit) {
        lifecycle_log(
            "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit"
            " generation=%u ordinal=%u",
            kStartupAuditGenerationLimit,
            kStartupAuditGeneration2PresentLimit);
        atomic_store(&g_startup_color_audit_finished, true);
    }
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_begin_render_pass(
    VkCommandBuffer command_buffer,
    const VkRenderPassBeginInfo* begin_info,
    VkSubpassContents contents) {
    if (!g_next_cmd_begin_render_pass) {
        lifecycle_log("LIFECYCLE_ERROR: begin render pass target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_cmd_begin_render_pass(command_buffer, begin_info, contents);
        return;
    }
    g_next_cmd_begin_render_pass(command_buffer, begin_info, contents);
    if (!atomic_load(&g_startup_color_audit) ||
        atomic_load(&g_startup_color_audit_finished) || !begin_info) {
        return;
    }

    FramebufferRecord framebuffer = {0};
    RenderPassRecord render_pass = {0};
    bool tracked = false;
    pthread_mutex_lock(&g_lock);
    FramebufferRecord* framebuffer_record =
        find_framebuffer(begin_info->framebuffer);
    if (framebuffer_record && framebuffer_record->generation >= 1 &&
        framebuffer_record->generation <= kStartupAuditGenerationLimit) {
        framebuffer = *framebuffer_record;
        RenderPassRecord* render_pass_record =
            find_render_pass(begin_info->renderPass);
        if (render_pass_record) {
            render_pass = *render_pass_record;
        }
        set_command_buffer(command_buffer, framebuffer_record);
        tracked = true;
    }
    pthread_mutex_unlock(&g_lock);
    if (!tracked) {
        return;
    }

    startup_color_detail_log(
        "STARTUP_COLOR_BEGIN: generation=%" PRIu64
        " command_buffer=%p framebuffer=%p"
        " render_pass=%p framebuffer_extent=%ux%u"
        " render_area=%d,%d,%ux%u clear_value_count=%u contents=%d",
        framebuffer.generation, (void*)command_buffer,
        (void*)framebuffer.handle,
        (void*)begin_info->renderPass, framebuffer.width, framebuffer.height,
        begin_info->renderArea.offset.x, begin_info->renderArea.offset.y,
        begin_info->renderArea.extent.width,
        begin_info->renderArea.extent.height, begin_info->clearValueCount,
        contents);
    const uint32_t clear_count =
        begin_info->clearValueCount < render_pass.attachment_count
            ? begin_info->clearValueCount
            : render_pass.attachment_count;
    for (uint32_t attachment = 0;
         begin_info->pClearValues && attachment < clear_count; ++attachment) {
        const VkClearValue* value = &begin_info->pClearValues[attachment];
        if (format_is_depth_or_stencil(
                render_pass.attachment_formats[attachment])) {
            startup_color_detail_log(
                "STARTUP_COLOR_BEGIN_CLEAR: generation=%" PRIu64
                " attachment=%u"
                " framebuffer=%p format=%d load_op=%d"
                " depth=%.9g depth_hex=%a stencil=%u",
                framebuffer.generation, attachment,
                (void*)framebuffer.handle,
                render_pass.attachment_formats[attachment],
                render_pass.attachment_load_ops[attachment],
                value->depthStencil.depth,
                (double)value->depthStencil.depth,
                value->depthStencil.stencil);
        } else {
            startup_color_detail_log(
                "STARTUP_COLOR_BEGIN_CLEAR: generation=%" PRIu64
                " attachment=%u"
                " framebuffer=%p format=%d load_op=%d"
                " rgba=%.9g,%.9g,%.9g,%.9g"
                " rgba_hex=%a,%a,%a,%a",
                framebuffer.generation, attachment,
                (void*)framebuffer.handle,
                render_pass.attachment_formats[attachment],
                render_pass.attachment_load_ops[attachment],
                value->color.float32[0], value->color.float32[1],
                value->color.float32[2], value->color.float32[3],
                (double)value->color.float32[0],
                (double)value->color.float32[1],
                (double)value->color.float32[2],
                (double)value->color.float32[3]);
        }
    }
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_end_render_pass(
    VkCommandBuffer command_buffer) {
    if (!g_next_cmd_end_render_pass) {
        lifecycle_log("LIFECYCLE_ERROR: end render pass target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_cmd_end_render_pass(command_buffer);
        return;
    }
    g_next_cmd_end_render_pass(command_buffer);
    if (!atomic_load(&g_startup_color_audit) ||
        atomic_load(&g_startup_color_audit_finished)) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    CommandBufferRecord* record = find_command_buffer(command_buffer);
    if (record) {
        record->in_render_pass = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR void VKAPI_CALL traced_cmd_clear_attachments(
    VkCommandBuffer command_buffer,
    uint32_t attachment_count,
    const VkClearAttachment* attachments,
    uint32_t rect_count,
    const VkClearRect* rects) {
    if (!g_next_cmd_clear_attachments) {
        lifecycle_log("LIFECYCLE_ERROR: clear attachments target is unset");
        return;
    }
    if (startup_audit_finished()) {
        g_next_cmd_clear_attachments(
            command_buffer, attachment_count, attachments, rect_count, rects);
        return;
    }
    g_next_cmd_clear_attachments(
        command_buffer, attachment_count, attachments, rect_count, rects);
    if (!atomic_load(&g_startup_color_audit) ||
        atomic_load(&g_startup_color_audit_finished)) {
        return;
    }

    CommandBufferRecord command = {0};
    bool tracked = false;
    pthread_mutex_lock(&g_lock);
    CommandBufferRecord* record = find_command_buffer(command_buffer);
    if (record && record->generation >= 1 &&
        record->generation <= kStartupAuditGenerationLimit &&
        record->in_render_pass) {
        command = *record;
        tracked = true;
    }
    pthread_mutex_unlock(&g_lock);
    if (!tracked) {
        return;
    }

    const uint32_t logged_attachments =
        attachment_count < kMaxRenderPassAttachments
            ? attachment_count
            : kMaxRenderPassAttachments;
    for (uint32_t index = 0;
         attachments && index < logged_attachments; ++index) {
        const VkClearAttachment* attachment = &attachments[index];
        if (attachment->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
            startup_color_detail_log(
                "STARTUP_COLOR_CLEAR: generation=%" PRIu64
                " command_buffer=%p"
                " framebuffer=%p attachment=%u aspect=0x%x"
                " color_attachment=%u rgba=%.9g,%.9g,%.9g,%.9g"
                " rgba_hex=%a,%a,%a,%a rect_count=%u",
                command.generation, (void*)command_buffer,
                (void*)command.framebuffer, index,
                attachment->aspectMask, attachment->colorAttachment,
                attachment->clearValue.color.float32[0],
                attachment->clearValue.color.float32[1],
                attachment->clearValue.color.float32[2],
                attachment->clearValue.color.float32[3],
                (double)attachment->clearValue.color.float32[0],
                (double)attachment->clearValue.color.float32[1],
                (double)attachment->clearValue.color.float32[2],
                (double)attachment->clearValue.color.float32[3], rect_count);
        } else {
            startup_color_detail_log(
                "STARTUP_COLOR_CLEAR: generation=%" PRIu64
                " command_buffer=%p"
                " framebuffer=%p attachment=%u aspect=0x%x"
                " depth=%.9g depth_hex=%a stencil=%u rect_count=%u",
                command.generation, (void*)command_buffer,
                (void*)command.framebuffer, index,
                attachment->aspectMask,
                attachment->clearValue.depthStencil.depth,
                (double)attachment->clearValue.depthStencil.depth,
                attachment->clearValue.depthStencil.stencil, rect_count);
        }
    }
    const uint32_t logged_rects =
        rect_count < kMaxRenderPassAttachments
            ? rect_count
            : kMaxRenderPassAttachments;
    for (uint32_t index = 0; rects && index < logged_rects; ++index) {
        startup_color_detail_log(
            "STARTUP_COLOR_CLEAR_RECT: generation=%" PRIu64
            " command_buffer=%p"
            " framebuffer=%p rect=%u area=%d,%d,%ux%u"
            " base_layer=%u layer_count=%u",
            command.generation, (void*)command_buffer,
            (void*)command.framebuffer, index,
            rects[index].rect.offset.x, rects[index].rect.offset.y,
            rects[index].rect.extent.width, rects[index].rect.extent.height,
            rects[index].baseArrayLayer, rects[index].layerCount);
    }
    if (attachment_count > logged_attachments || rect_count > logged_rects) {
        startup_color_detail_log(
            "STARTUP_COLOR_TRUNCATED: generation=%" PRIu64
            " attachments=%u/%u"
            " rects=%u/%u",
            command.generation, logged_attachments, attachment_count,
            logged_rects, rect_count);
    }
}

void teso4m4_lifecycle_reset(void) {
    pthread_mutex_lock(&g_lock);
    const bool clear_runtime_tables = g_reset_has_run;
    g_reset_has_run = true;
    g_next_device_wait_idle = NULL;
    g_next_create_swapchain = NULL;
    g_next_destroy_swapchain = NULL;
    g_next_get_swapchain_images = NULL;
    g_next_create_image_view = NULL;
    g_next_destroy_image_view = NULL;
    g_next_create_render_pass = NULL;
    g_next_destroy_render_pass = NULL;
    g_next_create_framebuffer = NULL;
    g_next_destroy_framebuffer = NULL;
    g_next_acquire_next_image = NULL;
    g_next_queue_present = NULL;
    g_next_queue_submit = NULL;
    g_next_cmd_begin_render_pass = NULL;
    g_next_cmd_end_render_pass = NULL;
    g_next_cmd_clear_attachments = NULL;
    g_next_begin_command_buffer = NULL;
    g_next_create_shader_module = NULL;
    g_next_destroy_shader_module = NULL;
    g_next_create_graphics_pipelines = NULL;
    g_next_destroy_pipeline = NULL;
    g_next_cmd_bind_pipeline = NULL;
    g_next_cmd_draw = NULL;
    g_next_cmd_draw_indexed = NULL;
    g_next_create_descriptor_set_layout = NULL;
    g_next_destroy_descriptor_set_layout = NULL;
    g_next_create_pipeline_layout = NULL;
    g_next_destroy_pipeline_layout = NULL;
    g_next_allocate_descriptor_sets = NULL;
    g_next_free_descriptor_sets = NULL;
    g_next_reset_descriptor_pool = NULL;
    g_next_destroy_descriptor_pool = NULL;
    g_next_update_descriptor_sets = NULL;
    g_next_cmd_bind_descriptor_sets = NULL;
    g_next_cmd_push_constants = NULL;
    g_logger = NULL;
    if (clear_runtime_tables) {
        memset(g_swapchains, 0, sizeof(g_swapchains));
        memset(g_images, 0, sizeof(g_images));
        memset(g_image_views, 0, sizeof(g_image_views));
        memset(g_render_passes, 0, sizeof(g_render_passes));
        memset(g_framebuffers, 0, sizeof(g_framebuffers));
        memset(g_command_buffers, 0, sizeof(g_command_buffers));
        memset(g_signaled_semaphores, 0, sizeof(g_signaled_semaphores));
        memset(g_shader_modules, 0, sizeof(g_shader_modules));
        memset(g_graphics_pipelines, 0, sizeof(g_graphics_pipelines));
        memset(g_descriptor_set_layouts, 0, sizeof(g_descriptor_set_layouts));
        memset(g_pipeline_layouts, 0, sizeof(g_pipeline_layouts));
        memset(g_descriptor_sets, 0, sizeof(g_descriptor_sets));
    }
    g_generation_counter = 0;
    g_wait_counter = 0;
    g_descriptor_update_call_counter = 0;
    g_overflow_reported = false;
    g_enabled = true;
    atomic_store(&g_startup_color_audit, false);
    atomic_store(&g_startup_present_pixel_audit, false);
    atomic_store(&g_startup_draw_audit, false);
    atomic_store(&g_startup_input_audit, false);
    atomic_store(&g_startup_compositor_audit, false);
    atomic_store(&g_startup_compositor_neutralize, false);
    atomic_store(&g_startup_color_audit_finished, false);
    atomic_store(&g_startup_color_detail_count, 0);
    g_present_pixel_sampler = NULL;
    g_compositor_image_sampler = NULL;
    g_compositor_neutralize_state =
        TESO4M4_COMPOSITOR_NEUTRALIZE_INACTIVE;
    g_compositor_placeholder_signature = 0;
    g_compositor_suppressed_draw_count = 0;
    g_compositor_neutralize_test_pipeline = VK_NULL_HANDLE;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_logger(Teso4m4LifecycleLogFunction logger) {
    pthread_mutex_lock(&g_lock);
    g_logger = logger;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_enabled(bool enabled) {
    pthread_mutex_lock(&g_lock);
    g_enabled = enabled;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_startup_color_audit(bool enabled) {
    atomic_store(&g_startup_color_audit_finished, false);
    atomic_store(&g_startup_color_detail_count, 0);
    atomic_store(&g_startup_color_audit, enabled);
    if (enabled) {
        lifecycle_log(
            "STARTUP_COLOR_AUDIT_BEGIN: generation_limit=2"
            " generation_2_present_limit=180");
    }
}

void teso4m4_lifecycle_set_startup_present_pixel_audit(bool enabled) {
    atomic_store(&g_startup_present_pixel_audit, enabled);
    if (enabled) {
        lifecycle_log(
            "STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1"
            " generation_2_samples=1,10,20,30,40,50,60,70,80,90,100,"
            "110,120,130,140,150,160,170,180");
    }
}

void teso4m4_lifecycle_set_startup_draw_audit(bool enabled) {
    atomic_store(&g_startup_draw_audit, enabled);
    if (enabled) {
        lifecycle_log(
            "STARTUP_DRAW_AUDIT_BEGIN: generation_limit=2"
            " generation_2_present_limit=180"
            " max_distinct_pipelines_per_submit=8");
    }
}

void teso4m4_lifecycle_set_startup_input_audit(bool enabled) {
    atomic_store(&g_startup_input_audit, enabled);
    if (enabled) {
        lifecycle_log(
            "STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2"
            " generation_2_present_limit=180"
            " max_descriptor_set_layouts=2048 max_pipeline_layouts=2048"
            " max_descriptor_sets=131072 max_bound_sets=16");
    }
}

void teso4m4_lifecycle_set_startup_compositor_audit(bool enabled) {
    atomic_store(&g_startup_compositor_audit, enabled);
    if (enabled) {
        lifecycle_log(
            "STARTUP_COMPOSITOR_AUDIT_BEGIN: image_bindings_per_set=2"
            " sampled_subresources=base-mip-base-layer");
    }
}

void teso4m4_lifecycle_set_startup_compositor_neutralize(bool enabled) {
    pthread_mutex_lock(&g_lock);
    atomic_store(&g_startup_compositor_neutralize, enabled);
    g_compositor_neutralize_state = enabled
        ? TESO4M4_COMPOSITOR_NEUTRALIZE_ARMED
        : TESO4M4_COMPOSITOR_NEUTRALIZE_INACTIVE;
    g_compositor_placeholder_signature = 0;
    g_compositor_suppressed_draw_count = 0;
    pthread_mutex_unlock(&g_lock);
    if (enabled) {
        lifecycle_log(
            "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2"
            " first_present=60 last_present=150"
            " max_suppressed_draws=96 fallback=forward");
    }
}

void teso4m4_lifecycle_set_compositor_neutralize_test_pipeline(
    VkPipeline pipeline) {
    pthread_mutex_lock(&g_lock);
    g_compositor_neutralize_test_pipeline = pipeline;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_present_pixel_sampler(
    Teso4m4PresentPixelSampler sampler) {
    pthread_mutex_lock(&g_lock);
    g_present_pixel_sampler = sampler;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_compositor_image_sampler(
    Teso4m4CompositorImageSampler sampler) {
    pthread_mutex_lock(&g_lock);
    g_compositor_image_sampler = sampler;
    pthread_mutex_unlock(&g_lock);
}

bool teso4m4_lifecycle_startup_window_open(void) {
    return atomic_load(&g_startup_color_audit) &&
           !atomic_load(&g_startup_color_audit_finished);
}

PFN_vkVoidFunction teso4m4_lifecycle_intercept(
    const char* name,
    PFN_vkVoidFunction next_function) {
    if (!name || !next_function) {
        return next_function;
    }
    PFN_vkVoidFunction returned = next_function;
    pthread_mutex_lock(&g_lock);
    if (!g_enabled) {
        pthread_mutex_unlock(&g_lock);
        return next_function;
    }
    if (strcmp(name, "vkDeviceWaitIdle") == 0) {
        g_next_device_wait_idle = (PFN_vkDeviceWaitIdle)next_function;
        returned = (PFN_vkVoidFunction)&traced_device_wait_idle;
    } else if (strcmp(name, "vkCreateSwapchainKHR") == 0) {
        g_next_create_swapchain = (PFN_vkCreateSwapchainKHR)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_swapchain;
    } else if (strcmp(name, "vkDestroySwapchainKHR") == 0) {
        g_next_destroy_swapchain = (PFN_vkDestroySwapchainKHR)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_swapchain;
    } else if (strcmp(name, "vkGetSwapchainImagesKHR") == 0) {
        g_next_get_swapchain_images =
            (PFN_vkGetSwapchainImagesKHR)next_function;
        returned = (PFN_vkVoidFunction)&traced_get_swapchain_images;
    } else if (strcmp(name, "vkCreateImageView") == 0) {
        g_next_create_image_view = (PFN_vkCreateImageView)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_image_view;
    } else if (strcmp(name, "vkDestroyImageView") == 0) {
        g_next_destroy_image_view = (PFN_vkDestroyImageView)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_image_view;
    } else if (strcmp(name, "vkCreateRenderPass") == 0) {
        g_next_create_render_pass = (PFN_vkCreateRenderPass)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_render_pass;
    } else if (strcmp(name, "vkDestroyRenderPass") == 0) {
        g_next_destroy_render_pass = (PFN_vkDestroyRenderPass)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_render_pass;
    } else if (strcmp(name, "vkCreateFramebuffer") == 0) {
        g_next_create_framebuffer = (PFN_vkCreateFramebuffer)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_framebuffer;
    } else if (strcmp(name, "vkDestroyFramebuffer") == 0) {
        g_next_destroy_framebuffer =
            (PFN_vkDestroyFramebuffer)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_framebuffer;
    } else if (strcmp(name, "vkAcquireNextImageKHR") == 0) {
        g_next_acquire_next_image =
            (PFN_vkAcquireNextImageKHR)next_function;
        returned = (PFN_vkVoidFunction)&traced_acquire_next_image;
    } else if (strcmp(name, "vkQueuePresentKHR") == 0) {
        g_next_queue_present = (PFN_vkQueuePresentKHR)next_function;
        returned = (PFN_vkVoidFunction)&traced_queue_present;
    } else if (strcmp(name, "vkQueueSubmit") == 0) {
        g_next_queue_submit = (PFN_vkQueueSubmit)next_function;
        returned = (PFN_vkVoidFunction)&traced_queue_submit;
    } else if (strcmp(name, "vkCmdBeginRenderPass") == 0) {
        g_next_cmd_begin_render_pass =
            (PFN_vkCmdBeginRenderPass)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_begin_render_pass;
    } else if (strcmp(name, "vkCmdEndRenderPass") == 0) {
        g_next_cmd_end_render_pass = (PFN_vkCmdEndRenderPass)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_end_render_pass;
    } else if (strcmp(name, "vkCmdClearAttachments") == 0) {
        g_next_cmd_clear_attachments =
            (PFN_vkCmdClearAttachments)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_clear_attachments;
    } else if (strcmp(name, "vkBeginCommandBuffer") == 0) {
        g_next_begin_command_buffer =
            (PFN_vkBeginCommandBuffer)next_function;
        returned = (PFN_vkVoidFunction)&traced_begin_command_buffer;
    } else if (strcmp(name, "vkCreateShaderModule") == 0) {
        g_next_create_shader_module =
            (PFN_vkCreateShaderModule)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_shader_module;
    } else if (strcmp(name, "vkDestroyShaderModule") == 0) {
        g_next_destroy_shader_module =
            (PFN_vkDestroyShaderModule)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_shader_module;
    } else if (strcmp(name, "vkCreateGraphicsPipelines") == 0) {
        g_next_create_graphics_pipelines =
            (PFN_vkCreateGraphicsPipelines)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_graphics_pipelines;
    } else if (strcmp(name, "vkDestroyPipeline") == 0) {
        g_next_destroy_pipeline = (PFN_vkDestroyPipeline)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_pipeline;
    } else if (strcmp(name, "vkCmdBindPipeline") == 0) {
        g_next_cmd_bind_pipeline = (PFN_vkCmdBindPipeline)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_bind_pipeline;
    } else if (strcmp(name, "vkCmdDraw") == 0) {
        g_next_cmd_draw = (PFN_vkCmdDraw)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_draw;
    } else if (strcmp(name, "vkCmdDrawIndexed") == 0) {
        g_next_cmd_draw_indexed = (PFN_vkCmdDrawIndexed)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_draw_indexed;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkCreateDescriptorSetLayout") == 0) {
        g_next_create_descriptor_set_layout =
            (PFN_vkCreateDescriptorSetLayout)next_function;
        returned =
            (PFN_vkVoidFunction)&traced_create_descriptor_set_layout;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkDestroyDescriptorSetLayout") == 0) {
        g_next_destroy_descriptor_set_layout =
            (PFN_vkDestroyDescriptorSetLayout)next_function;
        returned =
            (PFN_vkVoidFunction)&traced_destroy_descriptor_set_layout;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkCreatePipelineLayout") == 0) {
        g_next_create_pipeline_layout =
            (PFN_vkCreatePipelineLayout)next_function;
        returned = (PFN_vkVoidFunction)&traced_create_pipeline_layout;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkDestroyPipelineLayout") == 0) {
        g_next_destroy_pipeline_layout =
            (PFN_vkDestroyPipelineLayout)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_pipeline_layout;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkAllocateDescriptorSets") == 0) {
        g_next_allocate_descriptor_sets =
            (PFN_vkAllocateDescriptorSets)next_function;
        returned = (PFN_vkVoidFunction)&traced_allocate_descriptor_sets;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkFreeDescriptorSets") == 0) {
        g_next_free_descriptor_sets =
            (PFN_vkFreeDescriptorSets)next_function;
        returned = (PFN_vkVoidFunction)&traced_free_descriptor_sets;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkResetDescriptorPool") == 0) {
        g_next_reset_descriptor_pool =
            (PFN_vkResetDescriptorPool)next_function;
        returned = (PFN_vkVoidFunction)&traced_reset_descriptor_pool;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkDestroyDescriptorPool") == 0) {
        g_next_destroy_descriptor_pool =
            (PFN_vkDestroyDescriptorPool)next_function;
        returned = (PFN_vkVoidFunction)&traced_destroy_descriptor_pool;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkUpdateDescriptorSets") == 0) {
        g_next_update_descriptor_sets =
            (PFN_vkUpdateDescriptorSets)next_function;
        returned = (PFN_vkVoidFunction)&traced_update_descriptor_sets;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkCmdBindDescriptorSets") == 0) {
        g_next_cmd_bind_descriptor_sets =
            (PFN_vkCmdBindDescriptorSets)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_bind_descriptor_sets;
    } else if (atomic_load(&g_startup_input_audit) &&
               strcmp(name, "vkCmdPushConstants") == 0) {
        g_next_cmd_push_constants =
            (PFN_vkCmdPushConstants)next_function;
        returned = (PFN_vkVoidFunction)&traced_cmd_push_constants;
    }
    pthread_mutex_unlock(&g_lock);
    return returned;
}
