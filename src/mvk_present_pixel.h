#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "mvk_lifecycle.h"

#if defined(__GNUC__)
#define TESO4M4_PRESENT_PIXEL_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_PRESENT_PIXEL_HIDDEN
#endif

TESO4M4_PRESENT_PIXEL_HIDDEN void teso4m4_present_pixel_reset(void);
TESO4M4_PRESENT_PIXEL_HIDDEN bool teso4m4_present_pixel_configure(
    PFN_vkVoidFunction get_mtl_texture,
    PFN_vkQueueWaitIdle queue_wait_idle,
    Teso4m4LifecycleLogFunction logger);
TESO4M4_PRESENT_PIXEL_HIDDEN bool teso4m4_present_pixel_sample(
    VkQueue queue,
    VkImage image,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint64_t generation,
    uint32_t ordinal,
    uint32_t image_index);
