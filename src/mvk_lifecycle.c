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
    kMaxImageViews = 512,
    kMaxRenderPasses = 512,
    kMaxFramebuffers = 512,
    kMaxCommandBuffers = 512,
    kMaxSignaledSemaphores = 512,
    kMaxRenderPassAttachments = 16,
    kFirstPresentationLimit = 8,
    kStartupAuditGenerationLimit = 2,
    kStartupAuditGeneration2PresentLimit = 180,
    kStartupAuditDetailLimit = 2048,
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
    VkSemaphore handle;
    VkQueue queue;
    bool occupied;
} SignaledSemaphoreRecord;

typedef struct {
    VkImageView handle;
    uint64_t generation;
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

static Teso4m4LifecycleLogFunction g_logger;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static SwapchainRecord g_swapchains[kMaxSwapchains];
static ImageRecord g_images[kMaxSwapchainImages];
static ImageViewRecord g_image_views[kMaxImageViews];
static RenderPassRecord g_render_passes[kMaxRenderPasses];
static FramebufferRecord g_framebuffers[kMaxFramebuffers];
static CommandBufferRecord g_command_buffers[kMaxCommandBuffers];
static SignaledSemaphoreRecord g_signaled_semaphores[kMaxSignaledSemaphores];
static uint64_t g_generation_counter;
static uint64_t g_wait_counter;
static bool g_overflow_reported;
static bool g_enabled = true;
static atomic_bool g_startup_color_audit;
static atomic_bool g_startup_present_pixel_audit;
static atomic_bool g_startup_color_audit_finished;
static atomic_uint g_startup_color_detail_count;
static Teso4m4PresentPixelSampler g_present_pixel_sampler;

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

static void remember_signaled_semaphore(VkSemaphore handle, VkQueue queue) {
    SignaledSemaphoreRecord* existing = find_signaled_semaphore(handle);
    if (existing) {
        existing->queue = queue;
        return;
    }
    for (size_t index = 0; index < kMaxSignaledSemaphores; ++index) {
        if (!g_signaled_semaphores[index].occupied) {
            g_signaled_semaphores[index] = (SignaledSemaphoreRecord){
                .handle = handle,
                .queue = queue,
                .occupied = true,
            };
            return;
        }
    }
    report_overflow("signaled-semaphore");
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

static void add_image_view(VkImageView handle, uint64_t generation) {
    for (size_t index = 0; index < kMaxImageViews; ++index) {
        if (!g_image_views[index].alive) {
            g_image_views[index] = (ImageViewRecord){
                .handle = handle,
                .generation = generation,
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
    *record = (CommandBufferRecord){
        .handle = handle,
        .framebuffer = framebuffer->handle,
        .render_pass = framebuffer->render_pass,
        .generation = framebuffer->generation,
        .occupied = true,
        .in_render_pass = true,
    };
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
            add_image_view(*image_view, generation);
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
                    queue);
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
            Teso4m4PresentPixelSampler sampler = NULL;
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
            }
            sampler = g_present_pixel_sampler;
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
    g_logger = NULL;
    memset(g_swapchains, 0, sizeof(g_swapchains));
    memset(g_images, 0, sizeof(g_images));
    memset(g_image_views, 0, sizeof(g_image_views));
    memset(g_render_passes, 0, sizeof(g_render_passes));
    memset(g_framebuffers, 0, sizeof(g_framebuffers));
    memset(g_command_buffers, 0, sizeof(g_command_buffers));
    memset(g_signaled_semaphores, 0, sizeof(g_signaled_semaphores));
    g_generation_counter = 0;
    g_wait_counter = 0;
    g_overflow_reported = false;
    g_enabled = true;
    atomic_store(&g_startup_color_audit, false);
    atomic_store(&g_startup_present_pixel_audit, false);
    atomic_store(&g_startup_color_audit_finished, false);
    atomic_store(&g_startup_color_detail_count, 0);
    g_present_pixel_sampler = NULL;
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

void teso4m4_lifecycle_set_present_pixel_sampler(
    Teso4m4PresentPixelSampler sampler) {
    pthread_mutex_lock(&g_lock);
    g_present_pixel_sampler = sampler;
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
    }
    pthread_mutex_unlock(&g_lock);
    return returned;
}
