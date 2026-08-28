#include "mvk_swapchain_experiment.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
    kMaxSurfaceRecords = 8,
    kMaxSwapchainRecords = 32,
    kMaxSamples = 65536,
    kWarmupPresentsPerSwapchain = 300,
    kFirstCheckpointSamples = 600,
    kCheckpointIntervalSamples = 3600,
    kDurationHistogramWidthUs = 10,
    kIntervalHistogramWidthUs = 100,
    kHistogramBins = 10001,
};

typedef struct {
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR capabilities;
    bool valid;
} SurfaceRecord;

typedef struct {
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;
    uint32_t requested_images;
    uint32_t effective_images;
    uint32_t returned_images;
    uint64_t present_count;
    uint64_t last_present_start_ns;
    bool promoted;
    bool alive;
} SwapchainRecord;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static Teso4m4SwapchainExperimentMode g_mode;
static Teso4m4SwapchainLogFunction g_logger;
static Teso4m4SwapchainStartupWindowFunction g_startup_window_open;
static SurfaceRecord g_surfaces[kMaxSurfaceRecords];
static SwapchainRecord g_swapchains[kMaxSwapchainRecords];
static uint64_t g_acquire_samples[kMaxSamples];
static uint64_t g_present_samples[kMaxSamples];
static uint64_t g_interval_samples[kMaxSamples];
static uint64_t g_acquire_histogram[kHistogramBins];
static uint64_t g_present_histogram[kHistogramBins];
static uint64_t g_interval_histogram[kHistogramBins];
static size_t g_acquire_sample_count;
static size_t g_present_sample_count;
static size_t g_interval_sample_count;
static uint64_t g_create_count;
static uint64_t g_promoted_count;
static uint64_t g_forwarded_count;
static uint64_t g_capability_miss_count;
static uint64_t g_returned_two_count;
static uint64_t g_returned_three_count;
static uint64_t g_returned_count_mismatch_count;
static uint64_t g_acquire_error_count;
static uint64_t g_present_error_count;

static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    g_next_get_surface_capabilities;
static PFN_vkCreateSwapchainKHR g_next_create_swapchain;
static PFN_vkDestroySwapchainKHR g_next_destroy_swapchain;
static PFN_vkGetSwapchainImagesKHR g_next_get_swapchain_images;
static PFN_vkAcquireNextImageKHR g_next_acquire_next_image;
static PFN_vkQueuePresentKHR g_next_queue_present;

static void experiment_log(const char* format, ...) {
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

static uint64_t monotonic_ns(void) {
    struct timespec value = {0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &value) != 0) {
        return 0;
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
           (uint64_t)value.tv_nsec;
}

static SurfaceRecord* find_surface(VkSurfaceKHR surface) {
    for (size_t index = 0; index < kMaxSurfaceRecords; ++index) {
        if (g_surfaces[index].valid && g_surfaces[index].surface == surface) {
            return &g_surfaces[index];
        }
    }
    return NULL;
}

static void remember_surface(
    VkSurfaceKHR surface,
    const VkSurfaceCapabilitiesKHR* capabilities) {
    SurfaceRecord* record = find_surface(surface);
    if (!record) {
        for (size_t index = 0; index < kMaxSurfaceRecords; ++index) {
            if (!g_surfaces[index].valid) {
                record = &g_surfaces[index];
                break;
            }
        }
    }
    if (record) {
        record->surface = surface;
        record->capabilities = *capabilities;
        record->valid = true;
    }
}

static SwapchainRecord* find_swapchain(VkSwapchainKHR swapchain) {
    for (size_t index = 0; index < kMaxSwapchainRecords; ++index) {
        if (g_swapchains[index].alive &&
            g_swapchains[index].swapchain == swapchain) {
            return &g_swapchains[index];
        }
    }
    return NULL;
}

static SwapchainRecord* remember_swapchain(
    VkSwapchainKHR swapchain,
    const VkSwapchainCreateInfoKHR* requested,
    const VkSwapchainCreateInfoKHR* effective,
    bool promoted) {
    for (size_t index = 0; index < kMaxSwapchainRecords; ++index) {
        if (!g_swapchains[index].alive) {
            g_swapchains[index] = (SwapchainRecord){
                .swapchain = swapchain,
                .surface = effective->surface,
                .requested_images = requested->minImageCount,
                .effective_images = effective->minImageCount,
                .promoted = promoted,
                .alive = true,
            };
            return &g_swapchains[index];
        }
    }
    return NULL;
}

static void add_sample(
    uint64_t* samples,
    uint64_t* histogram,
    uint64_t histogram_width_us,
    size_t* count,
    uint64_t value) {
    if (value != 0 && *count < kMaxSamples) {
        samples[(*count)++] = value;
        const uint64_t value_us = value / 1000;
        uint64_t bin = value_us / histogram_width_us;
        if (bin >= kHistogramBins) {
            bin = kHistogramBins - 1;
        }
        ++histogram[bin];
    }
}

static int compare_u64(const void* left, const void* right) {
    const uint64_t a = *(const uint64_t*)left;
    const uint64_t b = *(const uint64_t*)right;
    return (a > b) - (a < b);
}

static uint64_t percentile(uint64_t* samples, size_t count, size_t percent) {
    if (count == 0) {
        return 0;
    }
    const size_t index = ((count - 1) * percent) / 100;
    return samples[index];
}

static uint64_t percentile_permille(
    uint64_t* samples, size_t count, size_t permille) {
    if (count == 0) {
        return 0;
    }
    const size_t index = ((count - 1) * permille) / 1000;
    return samples[index];
}

static uint64_t histogram_percentile_us(
    const uint64_t* histogram,
    size_t count,
    uint64_t width_us,
    size_t permille) {
    if (count == 0) {
        return 0;
    }
    const uint64_t target =
        ((uint64_t)count * (uint64_t)permille + 999) / 1000;
    uint64_t cumulative = 0;
    for (size_t index = 0; index < kHistogramBins; ++index) {
        cumulative += histogram[index];
        if (cumulative >= target) {
            return (uint64_t)(index + 1) * width_us;
        }
    }
    return (uint64_t)kHistogramBins * width_us;
}

static const char* experiment_mode_name(void) {
    return g_mode == TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER
        ? "triple" : "control";
}

static void log_checkpoint_locked(void) {
    experiment_log(
        "SWAPCHAIN_EXPERIMENT_CHECKPOINT: mode=%s creates=%" PRIu64
        " promoted=%" PRIu64 " forwarded=%" PRIu64
        " capability_misses=%" PRIu64
        " returned_two=%" PRIu64 " returned_three=%" PRIu64
        " returned_count_mismatches=%" PRIu64
        " acquire_samples=%zu acquire_p50_us=%" PRIu64
        " acquire_p95_us=%" PRIu64 " acquire_p99_us=%" PRIu64
        " acquire_p999_us=%" PRIu64 " acquire_max_us=0"
        " present_samples=%zu present_p50_us=%" PRIu64
        " present_p95_us=%" PRIu64 " present_p99_us=%" PRIu64
        " present_p999_us=%" PRIu64
        " interval_samples=%zu interval_p50_us=%" PRIu64
        " interval_p95_us=%" PRIu64 " interval_p99_us=%" PRIu64
        " interval_p999_us=%" PRIu64
        " acquire_errors=%" PRIu64 " present_errors=%" PRIu64,
        experiment_mode_name(), g_create_count, g_promoted_count,
        g_forwarded_count, g_capability_miss_count, g_returned_two_count,
        g_returned_three_count, g_returned_count_mismatch_count,
        g_acquire_sample_count,
        histogram_percentile_us(
            g_acquire_histogram, g_acquire_sample_count,
            kDurationHistogramWidthUs, 500),
        histogram_percentile_us(
            g_acquire_histogram, g_acquire_sample_count,
            kDurationHistogramWidthUs, 950),
        histogram_percentile_us(
            g_acquire_histogram, g_acquire_sample_count,
            kDurationHistogramWidthUs, 990),
        histogram_percentile_us(
            g_acquire_histogram, g_acquire_sample_count,
            kDurationHistogramWidthUs, 999),
        g_present_sample_count,
        histogram_percentile_us(
            g_present_histogram, g_present_sample_count,
            kDurationHistogramWidthUs, 500),
        histogram_percentile_us(
            g_present_histogram, g_present_sample_count,
            kDurationHistogramWidthUs, 950),
        histogram_percentile_us(
            g_present_histogram, g_present_sample_count,
            kDurationHistogramWidthUs, 990),
        histogram_percentile_us(
            g_present_histogram, g_present_sample_count,
            kDurationHistogramWidthUs, 999),
        g_interval_sample_count,
        histogram_percentile_us(
            g_interval_histogram, g_interval_sample_count,
            kIntervalHistogramWidthUs, 500),
        histogram_percentile_us(
            g_interval_histogram, g_interval_sample_count,
            kIntervalHistogramWidthUs, 950),
        histogram_percentile_us(
            g_interval_histogram, g_interval_sample_count,
            kIntervalHistogramWidthUs, 990),
        histogram_percentile_us(
            g_interval_histogram, g_interval_sample_count,
            kIntervalHistogramWidthUs, 999),
        g_acquire_error_count, g_present_error_count);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_get_surface_capabilities(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* capabilities) {
    if (!g_next_get_surface_capabilities) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const VkResult result = g_next_get_surface_capabilities(
        physical_device, surface, capabilities);
    if (result == VK_SUCCESS && capabilities) {
        pthread_mutex_lock(&g_lock);
        remember_surface(surface, capabilities);
        pthread_mutex_unlock(&g_lock);
        experiment_log(
            "SWAPCHAIN_CAPABILITIES: min_images=%u max_images=%u "
            "current_extent=%ux%u",
            capabilities->minImageCount, capabilities->maxImageCount,
            capabilities->currentExtent.width,
            capabilities->currentExtent.height);
    }
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
    VkSwapchainCreateInfoKHR candidate = {0};
    const VkSwapchainCreateInfoKHR* effective = create_info;
    bool promoted = false;
    const char* reason = "control";
    pthread_mutex_lock(&g_lock);
    ++g_create_count;
    SurfaceRecord* surface = create_info ? find_surface(create_info->surface) : NULL;
    if (g_mode == TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER &&
        create_info && create_info->minImageCount == 2 && surface &&
        surface->capabilities.minImageCount <= 3 &&
        (surface->capabilities.maxImageCount == 0 ||
         surface->capabilities.maxImageCount >= 3)) {
        candidate = *create_info;
        candidate.minImageCount = 3;
        effective = &candidate;
        promoted = true;
        ++g_promoted_count;
        reason = "supported";
    } else if (g_mode == TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER) {
        ++g_forwarded_count;
        if (!surface) {
            ++g_capability_miss_count;
            reason = "capabilities-unobserved";
        } else if (!create_info || create_info->minImageCount != 2) {
            reason = "unexpected-request";
        } else {
            reason = "unsupported";
        }
    } else {
        ++g_forwarded_count;
    }
    pthread_mutex_unlock(&g_lock);

    const VkResult result = g_next_create_swapchain(
        device, effective, allocator, swapchain);
    if (result == VK_SUCCESS && create_info && effective && swapchain &&
        *swapchain != VK_NULL_HANDLE) {
        pthread_mutex_lock(&g_lock);
        (void)remember_swapchain(*swapchain, create_info, effective, promoted);
        pthread_mutex_unlock(&g_lock);
    }
    experiment_log(
        "SWAPCHAIN_BUFFER_POLICY: requested=%u effective=%u action=%s "
        "reason=%s result=%d",
        create_info ? create_info->minImageCount : 0,
        effective ? effective->minImageCount : 0,
        promoted ? "promote" : "forward", reason, result);
    return result;
}

static VKAPI_ATTR void VKAPI_CALL traced_destroy_swapchain(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* allocator) {
    if (!g_next_destroy_swapchain) {
        return;
    }
    g_next_destroy_swapchain(device, swapchain, allocator);
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = find_swapchain(swapchain);
    if (record) {
        record->alive = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_get_swapchain_images(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* image_count,
    VkImage* images) {
    if (!g_next_get_swapchain_images) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const VkResult result = g_next_get_swapchain_images(
        device, swapchain, image_count, images);
    if (image_count && (result == VK_SUCCESS || result == VK_INCOMPLETE)) {
        uint32_t effective_images = 0;
        pthread_mutex_lock(&g_lock);
        SwapchainRecord* record = find_swapchain(swapchain);
        if (record) {
            record->returned_images = *image_count;
            effective_images = record->effective_images;
            if (!images) {
                if (*image_count == 2) {
                    ++g_returned_two_count;
                }
                if (*image_count == 3) {
                    ++g_returned_three_count;
                }
                if (*image_count < effective_images) {
                    ++g_returned_count_mismatch_count;
                }
            }
        }
        pthread_mutex_unlock(&g_lock);
        if (!images) {
            experiment_log(
                "SWAPCHAIN_BUFFER_COUNT: effective=%u returned=%u result=%d",
                effective_images, *image_count, result);
        }
    }
    return result;
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
    if (g_startup_window_open && g_startup_window_open()) {
        return g_next_acquire_next_image(
            device, swapchain, timeout, semaphore, fence, image_index);
    }
    const uint64_t start = monotonic_ns();
    const VkResult result = g_next_acquire_next_image(
        device, swapchain, timeout, semaphore, fence, image_index);
    const uint64_t end = monotonic_ns();
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = find_swapchain(swapchain);
    if (record && record->present_count >= kWarmupPresentsPerSwapchain) {
        add_sample(
            g_acquire_samples, g_acquire_histogram,
            kDurationHistogramWidthUs, &g_acquire_sample_count, end - start);
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        ++g_acquire_error_count;
    }
    pthread_mutex_unlock(&g_lock);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL traced_queue_present(
    VkQueue queue,
    const VkPresentInfoKHR* present_info) {
    if (!g_next_queue_present) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (g_startup_window_open && g_startup_window_open()) {
        return g_next_queue_present(queue, present_info);
    }
    const uint64_t start = monotonic_ns();
    const VkResult result = g_next_queue_present(queue, present_info);
    const uint64_t end = monotonic_ns();
    pthread_mutex_lock(&g_lock);
    SwapchainRecord* record = NULL;
    if (present_info && present_info->swapchainCount > 0) {
        record = find_swapchain(present_info->pSwapchains[0]);
    }
    if (record) {
        ++record->present_count;
        if (record->present_count > kWarmupPresentsPerSwapchain) {
            add_sample(
                g_present_samples, g_present_histogram,
                kDurationHistogramWidthUs, &g_present_sample_count,
                end - start);
            if (record->last_present_start_ns != 0) {
                add_sample(
                    g_interval_samples, g_interval_histogram,
                    kIntervalHistogramWidthUs, &g_interval_sample_count,
                    start - record->last_present_start_ns);
            }
            if (g_interval_sample_count == kFirstCheckpointSamples ||
                (g_interval_sample_count > kFirstCheckpointSamples &&
                 g_interval_sample_count % kCheckpointIntervalSamples == 0)) {
                log_checkpoint_locked();
            }
        }
        record->last_present_start_ns = start;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        ++g_present_error_count;
    }
    pthread_mutex_unlock(&g_lock);
    return result;
}

void teso4m4_swapchain_experiment_reset(void) {
    pthread_mutex_lock(&g_lock);
    g_mode = TESO4M4_SWAPCHAIN_EXPERIMENT_DISABLED;
    g_logger = NULL;
    g_startup_window_open = NULL;
    memset(g_surfaces, 0, sizeof(g_surfaces));
    memset(g_swapchains, 0, sizeof(g_swapchains));
    g_acquire_sample_count = 0;
    g_present_sample_count = 0;
    g_interval_sample_count = 0;
    memset(g_acquire_histogram, 0, sizeof(g_acquire_histogram));
    memset(g_present_histogram, 0, sizeof(g_present_histogram));
    memset(g_interval_histogram, 0, sizeof(g_interval_histogram));
    g_create_count = 0;
    g_promoted_count = 0;
    g_forwarded_count = 0;
    g_capability_miss_count = 0;
    g_returned_two_count = 0;
    g_returned_three_count = 0;
    g_returned_count_mismatch_count = 0;
    g_acquire_error_count = 0;
    g_present_error_count = 0;
    g_next_get_surface_capabilities = NULL;
    g_next_create_swapchain = NULL;
    g_next_destroy_swapchain = NULL;
    g_next_get_swapchain_images = NULL;
    g_next_acquire_next_image = NULL;
    g_next_queue_present = NULL;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_swapchain_experiment_configure(
    Teso4m4SwapchainExperimentMode mode,
    Teso4m4SwapchainLogFunction logger) {
    pthread_mutex_lock(&g_lock);
    g_mode = mode;
    g_logger = logger;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_swapchain_experiment_set_startup_window_function(
    Teso4m4SwapchainStartupWindowFunction startup_window_open) {
    pthread_mutex_lock(&g_lock);
    g_startup_window_open = startup_window_open;
    pthread_mutex_unlock(&g_lock);
}

PFN_vkVoidFunction teso4m4_swapchain_experiment_intercept(
    const char* name,
    PFN_vkVoidFunction next_function) {
    if (!name || !next_function ||
        g_mode == TESO4M4_SWAPCHAIN_EXPERIMENT_DISABLED) {
        return next_function;
    }
    if (strcmp(name, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") == 0) {
        g_next_get_surface_capabilities =
            (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)next_function;
        return (PFN_vkVoidFunction)&traced_get_surface_capabilities;
    }
    if (strcmp(name, "vkCreateSwapchainKHR") == 0) {
        g_next_create_swapchain = (PFN_vkCreateSwapchainKHR)next_function;
        return (PFN_vkVoidFunction)&traced_create_swapchain;
    }
    if (strcmp(name, "vkDestroySwapchainKHR") == 0) {
        g_next_destroy_swapchain = (PFN_vkDestroySwapchainKHR)next_function;
        return (PFN_vkVoidFunction)&traced_destroy_swapchain;
    }
    if (strcmp(name, "vkGetSwapchainImagesKHR") == 0) {
        g_next_get_swapchain_images =
            (PFN_vkGetSwapchainImagesKHR)next_function;
        return (PFN_vkVoidFunction)&traced_get_swapchain_images;
    }
    if (strcmp(name, "vkAcquireNextImageKHR") == 0) {
        g_next_acquire_next_image = (PFN_vkAcquireNextImageKHR)next_function;
        return (PFN_vkVoidFunction)&traced_acquire_next_image;
    }
    if (strcmp(name, "vkQueuePresentKHR") == 0) {
        g_next_queue_present = (PFN_vkQueuePresentKHR)next_function;
        return (PFN_vkVoidFunction)&traced_queue_present;
    }
    return next_function;
}

void teso4m4_swapchain_experiment_log_summary(void) {
    pthread_mutex_lock(&g_lock);
    if (g_mode == TESO4M4_SWAPCHAIN_EXPERIMENT_DISABLED || !g_logger) {
        pthread_mutex_unlock(&g_lock);
        return;
    }
    qsort(g_acquire_samples, g_acquire_sample_count, sizeof(uint64_t), compare_u64);
    qsort(g_present_samples, g_present_sample_count, sizeof(uint64_t), compare_u64);
    qsort(g_interval_samples, g_interval_sample_count, sizeof(uint64_t), compare_u64);
    const char* mode = experiment_mode_name();
    experiment_log(
        "SWAPCHAIN_EXPERIMENT_SUMMARY: mode=%s creates=%" PRIu64
        " promoted=%" PRIu64 " forwarded=%" PRIu64
        " capability_misses=%" PRIu64
        " returned_two=%" PRIu64 " returned_three=%" PRIu64
        " returned_count_mismatches=%" PRIu64
        " acquire_samples=%zu acquire_p50_us=%" PRIu64
        " acquire_p95_us=%" PRIu64 " acquire_p99_us=%" PRIu64
        " acquire_p999_us=%" PRIu64
        " acquire_max_us=%" PRIu64
        " present_samples=%zu present_p50_us=%" PRIu64
        " present_p95_us=%" PRIu64 " present_p99_us=%" PRIu64
        " present_p999_us=%" PRIu64
        " interval_samples=%zu interval_p50_us=%" PRIu64
        " interval_p95_us=%" PRIu64 " interval_p99_us=%" PRIu64
        " interval_p999_us=%" PRIu64
        " acquire_errors=%" PRIu64 " present_errors=%" PRIu64,
        mode, g_create_count, g_promoted_count, g_forwarded_count,
        g_capability_miss_count, g_returned_two_count, g_returned_three_count,
        g_returned_count_mismatch_count, g_acquire_sample_count,
        percentile(g_acquire_samples, g_acquire_sample_count, 50) / 1000,
        percentile(g_acquire_samples, g_acquire_sample_count, 95) / 1000,
        percentile(g_acquire_samples, g_acquire_sample_count, 99) / 1000,
        percentile_permille(
            g_acquire_samples, g_acquire_sample_count, 999) / 1000,
        g_acquire_sample_count ? g_acquire_samples[g_acquire_sample_count - 1] / 1000 : 0,
        g_present_sample_count,
        percentile(g_present_samples, g_present_sample_count, 50) / 1000,
        percentile(g_present_samples, g_present_sample_count, 95) / 1000,
        percentile(g_present_samples, g_present_sample_count, 99) / 1000,
        percentile_permille(
            g_present_samples, g_present_sample_count, 999) / 1000,
        g_interval_sample_count,
        percentile(g_interval_samples, g_interval_sample_count, 50) / 1000,
        percentile(g_interval_samples, g_interval_sample_count, 95) / 1000,
        percentile(g_interval_samples, g_interval_sample_count, 99) / 1000,
        percentile_permille(
            g_interval_samples, g_interval_sample_count, 999) / 1000,
        g_acquire_error_count, g_present_error_count);
    pthread_mutex_unlock(&g_lock);
}
