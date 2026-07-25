#include "mvk_lifecycle.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
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
    kFirstPresentationLimit = 8,
};

typedef struct {
    VkSwapchainKHR handle;
    uint64_t generation;
    uint32_t acquire_count;
    uint32_t present_count;
    bool alive;
} SwapchainRecord;

typedef struct {
    VkImage handle;
    uint64_t generation;
    bool alive;
} ImageRecord;

typedef struct {
    VkImageView handle;
    uint64_t generation;
    bool alive;
} ImageViewRecord;

typedef struct {
    VkRenderPass handle;
    uint64_t first_generation;
    uint64_t last_generation;
    uint32_t link_count;
    bool alive;
} RenderPassRecord;

typedef struct {
    VkFramebuffer handle;
    uint64_t generation;
    bool alive;
} FramebufferRecord;

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

static Teso4m4LifecycleLogFunction g_logger;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static SwapchainRecord g_swapchains[kMaxSwapchains];
static ImageRecord g_images[kMaxSwapchainImages];
static ImageViewRecord g_image_views[kMaxImageViews];
static RenderPassRecord g_render_passes[kMaxRenderPasses];
static FramebufferRecord g_framebuffers[kMaxFramebuffers];
static uint64_t g_generation_counter;
static uint64_t g_wait_counter;
static bool g_overflow_reported;

static void lifecycle_log(const char* format, ...) {
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
    uint64_t generation) {
    for (size_t index = 0; index < kMaxSwapchains; ++index) {
        if (!g_swapchains[index].alive) {
            g_swapchains[index] = (SwapchainRecord){
                .handle = handle,
                .generation = generation,
                .alive = true,
            };
            return &g_swapchains[index];
        }
    }
    report_overflow("swapchain");
    return NULL;
}

static void add_image(VkImage handle, uint64_t generation) {
    ImageRecord* existing = find_image(handle);
    if (existing) {
        existing->generation = generation;
        return;
    }
    for (size_t index = 0; index < kMaxSwapchainImages; ++index) {
        if (!g_images[index].alive) {
            g_images[index] = (ImageRecord){
                .handle = handle,
                .generation = generation,
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

static void add_render_pass(VkRenderPass handle) {
    for (size_t index = 0; index < kMaxRenderPasses; ++index) {
        if (!g_render_passes[index].alive) {
            g_render_passes[index] = (RenderPassRecord){
                .handle = handle,
                .alive = true,
            };
            return;
        }
    }
    report_overflow("render-pass");
}

static void add_framebuffer(VkFramebuffer handle, uint64_t generation) {
    for (size_t index = 0; index < kMaxFramebuffers; ++index) {
        if (!g_framebuffers[index].alive) {
            g_framebuffers[index] = (FramebufferRecord){
                .handle = handle,
                .generation = generation,
                .alive = true,
            };
            return;
        }
    }
    report_overflow("framebuffer");
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_device_wait_idle(
    VkDevice device) {
    if (!g_next_device_wait_idle) {
        return VK_ERROR_INITIALIZATION_FAILED;
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
        add_swapchain(*swapchain, generation);
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
            add_image(images[index], generation);
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
    VkResult result =
        g_next_create_render_pass(device, create_info, allocator, render_pass);
    if (result == VK_SUCCESS && render_pass &&
        *render_pass != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        add_render_pass(*render_pass);
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
            add_framebuffer(*framebuffer, generation);
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
    if (ordinal <= kFirstPresentationLimit || result != VK_SUCCESS) {
        lifecycle_log(
            "SWAPCHAIN_ACQUIRE: device=%p swapchain=%p generation=%" PRIu64
            " ordinal=%u image_index=%u result=%d",
            (void*)device, (void*)swapchain, generation, ordinal,
            image_index ? *image_index : UINT32_MAX, result);
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_queue_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    if (!g_next_queue_present) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult result = g_next_queue_present(queue, present_info);
    if (!present_info) {
        lifecycle_log("SWAPCHAIN_PRESENT: queue=%p info=NULL result=%d",
                      (void*)queue, result);
        return result;
    }
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
        if (ordinal <= kFirstPresentationLimit ||
            result != VK_SUCCESS || item_result != VK_SUCCESS) {
            lifecycle_log(
                "SWAPCHAIN_PRESENT: queue=%p swapchain=%p generation=%" PRIu64
                " ordinal=%u image_index=%u result=%d item_result=%d",
                (void*)queue, (void*)swapchain, generation, ordinal,
                present_info->pImageIndices ?
                    present_info->pImageIndices[index] : UINT32_MAX,
                result, item_result);
        }
    }
    return result;
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
    g_logger = NULL;
    memset(g_swapchains, 0, sizeof(g_swapchains));
    memset(g_images, 0, sizeof(g_images));
    memset(g_image_views, 0, sizeof(g_image_views));
    memset(g_render_passes, 0, sizeof(g_render_passes));
    memset(g_framebuffers, 0, sizeof(g_framebuffers));
    g_generation_counter = 0;
    g_wait_counter = 0;
    g_overflow_reported = false;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_lifecycle_set_logger(Teso4m4LifecycleLogFunction logger) {
    pthread_mutex_lock(&g_lock);
    g_logger = logger;
    pthread_mutex_unlock(&g_lock);
}

PFN_vkVoidFunction teso4m4_lifecycle_intercept(
    const char* name,
    PFN_vkVoidFunction next_function) {
    if (!name || !next_function) {
        return next_function;
    }
    PFN_vkVoidFunction returned = next_function;
    pthread_mutex_lock(&g_lock);
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
    }
    pthread_mutex_unlock(&g_lock);
    return returned;
}
