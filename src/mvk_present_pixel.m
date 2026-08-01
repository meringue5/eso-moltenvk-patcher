#import <Metal/Metal.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mvk_present_pixel.h"

typedef void (VKAPI_PTR *PFN_teso4m4GetMTLTextureMVK)(
    VkImage image,
    id<MTLTexture>* texture);

enum {
    kSampleCount = 5,
    kBytesPerRow = 256,
};

static PFN_teso4m4GetMTLTextureMVK g_get_mtl_texture;
static PFN_vkQueueWaitIdle g_queue_wait_idle;
static Teso4m4LifecycleLogFunction g_logger;

static void pixel_log(const char* format, ...) {
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

static bool format_is_bgra(VkFormat format) {
    return format == VK_FORMAT_B8G8R8A8_UNORM ||
           format == VK_FORMAT_B8G8R8A8_SRGB;
}

static bool format_is_rgba(VkFormat format) {
    return format == VK_FORMAT_R8G8B8A8_UNORM ||
           format == VK_FORMAT_R8G8B8A8_SRGB;
}

void teso4m4_present_pixel_reset(void) {
    g_get_mtl_texture = NULL;
    g_queue_wait_idle = NULL;
    g_logger = NULL;
}

bool teso4m4_present_pixel_configure(
    PFN_vkVoidFunction get_mtl_texture,
    PFN_vkQueueWaitIdle queue_wait_idle,
    Teso4m4LifecycleLogFunction logger) {
    teso4m4_present_pixel_reset();
    if (!get_mtl_texture || !queue_wait_idle || !logger) {
        return false;
    }
    g_get_mtl_texture =
        (PFN_teso4m4GetMTLTextureMVK)get_mtl_texture;
    g_queue_wait_idle = queue_wait_idle;
    g_logger = logger;
    return true;
}

bool teso4m4_present_pixel_sample(
    VkQueue queue,
    VkImage image,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint64_t generation,
    uint32_t ordinal,
    uint32_t image_index) {
    @autoreleasepool {
        if (!g_get_mtl_texture || !g_queue_wait_idle || !g_logger ||
            !queue || !image || width == 0 || height == 0) {
            return false;
        }
        if (!format_is_bgra(format) && !format_is_rgba(format)) {
            pixel_log(
                "STARTUP_PRESENT_PIXEL_ERROR: generation=%" PRIu64
                " ordinal=%u image_index=%u reason=unsupported-format"
                " format=%d",
                generation, ordinal, image_index, format);
            return false;
        }

        const VkResult wait_result = g_queue_wait_idle(queue);
        if (wait_result != VK_SUCCESS) {
            pixel_log(
                "STARTUP_PRESENT_PIXEL_ERROR: generation=%" PRIu64
                " ordinal=%u image_index=%u reason=queue-wait-idle"
                " result=%d",
                generation, ordinal, image_index, wait_result);
            return false;
        }

        id<MTLTexture> texture = nil;
        g_get_mtl_texture(image, &texture);
        if (!texture || texture.width == 0 || texture.height == 0) {
            pixel_log(
                "STARTUP_PRESENT_PIXEL_ERROR: generation=%" PRIu64
                " ordinal=%u image_index=%u reason=texture-unavailable",
                generation, ordinal, image_index);
            return false;
        }

        const uint32_t texture_width = (uint32_t)texture.width;
        const uint32_t texture_height = (uint32_t)texture.height;
        const uint32_t xs[kSampleCount] = {
            texture_width / 2,
            texture_width / 4,
            (texture_width * 3) / 4,
            texture_width / 4,
            (texture_width * 3) / 4,
        };
        const uint32_t ys[kSampleCount] = {
            texture_height / 2,
            texture_height / 4,
            texture_height / 4,
            (texture_height * 3) / 4,
            (texture_height * 3) / 4,
        };

        id<MTLCommandQueue> metal_queue = [texture.device newCommandQueue];
        id<MTLBuffer> buffer = [texture.device
            newBufferWithLength:kSampleCount * kBytesPerRow
                        options:MTLResourceStorageModeShared];
        id<MTLCommandBuffer> command = [metal_queue commandBuffer];
        id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
        if (!metal_queue || !buffer || !command || !blit) {
            pixel_log(
                "STARTUP_PRESENT_PIXEL_ERROR: generation=%" PRIu64
                " ordinal=%u image_index=%u reason=metal-allocation",
                generation, ordinal, image_index);
            return false;
        }
        for (uint32_t sample = 0; sample < kSampleCount; ++sample) {
            [blit copyFromTexture:texture
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(xs[sample], ys[sample], 0)
                       sourceSize:MTLSizeMake(1, 1, 1)
                         toBuffer:buffer
                destinationOffset:sample * kBytesPerRow
           destinationBytesPerRow:kBytesPerRow
         destinationBytesPerImage:kBytesPerRow];
        }
        [blit endEncoding];
        [command commit];
        [command waitUntilCompleted];
        if (command.status != MTLCommandBufferStatusCompleted) {
            pixel_log(
                "STARTUP_PRESENT_PIXEL_ERROR: generation=%" PRIu64
                " ordinal=%u image_index=%u reason=metal-readback"
                " status=%ld error=%s",
                generation, ordinal, image_index, (long)command.status,
                command.error.localizedDescription.UTF8String ?: "none");
            return false;
        }

        const uint8_t* bytes = (const uint8_t*)buffer.contents;
        uint32_t exact_magenta = 0;
        uint32_t near_magenta = 0;
        uint32_t black = 0;
        for (uint32_t sample = 0; sample < kSampleCount; ++sample) {
            const uint8_t* raw = bytes + sample * kBytesPerRow;
            const uint8_t r = format_is_bgra(format) ? raw[2] : raw[0];
            const uint8_t g = raw[1];
            const uint8_t b = format_is_bgra(format) ? raw[0] : raw[2];
            const uint8_t a = raw[3];
            const bool exact = r == 255 && g == 0 && b == 255;
            const bool near = r >= 240 && g <= 15 && b >= 240;
            const bool is_black = r <= 3 && g <= 3 && b <= 3;
            exact_magenta += exact ? 1u : 0u;
            near_magenta += near ? 1u : 0u;
            black += is_black ? 1u : 0u;
            pixel_log(
                "STARTUP_PRESENT_PIXEL: generation=%" PRIu64
                " ordinal=%u image_index=%u sample=%u x=%u y=%u"
                " format=%d raw=%u,%u,%u,%u rgba=%u,%u,%u,%u",
                generation, ordinal, image_index, sample, xs[sample],
                ys[sample], format, raw[0], raw[1], raw[2], raw[3],
                r, g, b, a);
        }
        pixel_log(
            "STARTUP_PRESENT_PIXEL_SUMMARY: generation=%" PRIu64
            " ordinal=%u image_index=%u requested_extent=%ux%u"
            " texture_extent=%ux%u format=%d samples=%u"
            " exact_magenta=%u near_magenta=%u black=%u",
            generation, ordinal, image_index, width, height,
            texture_width, texture_height, format, kSampleCount,
            exact_magenta, near_magenta, black);
        return true;
    }
}
