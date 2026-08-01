#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#define TESO4M4_LIFECYCLE_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_LIFECYCLE_HIDDEN
#endif

typedef void (*Teso4m4LifecycleLogFunction)(const char* message);
typedef bool (*Teso4m4PresentPixelSampler)(
    VkQueue queue,
    VkImage image,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint64_t generation,
    uint32_t ordinal,
    uint32_t image_index);
typedef bool (*Teso4m4CompositorImageSampler)(
    VkQueue queue,
    VkImage image,
    VkFormat format,
    VkImageViewType view_type,
    uint32_t mip_level,
    uint32_t array_layer,
    uint64_t generation,
    uint32_t ordinal,
    uint32_t set_slot,
    uint32_t binding,
    uint32_t array_element,
    uint32_t image_ordinal);

TESO4M4_LIFECYCLE_HIDDEN void teso4m4_lifecycle_reset(void);
TESO4M4_LIFECYCLE_HIDDEN void teso4m4_lifecycle_set_logger(
    Teso4m4LifecycleLogFunction logger);
TESO4M4_LIFECYCLE_HIDDEN void teso4m4_lifecycle_set_enabled(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_color_audit(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_present_pixel_audit(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_draw_audit(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_input_audit(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_compositor_audit(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_startup_compositor_neutralize(bool enabled);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_compositor_neutralize_test_pipeline(
    VkPipeline pipeline);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_compositor_neutralize_test_ordinal(
    uint32_t ordinal);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_present_pixel_sampler(
    Teso4m4PresentPixelSampler sampler);
TESO4M4_LIFECYCLE_HIDDEN void
teso4m4_lifecycle_set_compositor_image_sampler(
    Teso4m4CompositorImageSampler sampler);
TESO4M4_LIFECYCLE_HIDDEN bool
teso4m4_lifecycle_startup_window_open(void);
TESO4M4_LIFECYCLE_HIDDEN PFN_vkVoidFunction
teso4m4_lifecycle_intercept(
    const char* name,
    PFN_vkVoidFunction next_function);
