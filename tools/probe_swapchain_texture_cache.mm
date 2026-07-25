#define VK_USE_PLATFORM_MACOS_MVK 1

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <vulkan/vulkan.h>

#ifndef TESO4M4_PROBE_IDENTITY_VIEW
#define TESO4M4_PROBE_IDENTITY_VIEW 0
#endif

extern "C" void VKAPI_CALL vkGetMTLTextureMVK(
    VkImage image, id<MTLTexture>* mtl_texture);

static bool vk_ok(VkResult result, const char* operation) {
    if (result == VK_SUCCESS) {
        return true;
    }
    fprintf(stderr, "%s failed: %d\n", operation, result);
    return false;
}

static uint8_t to_unorm8(float value) {
    return (uint8_t)lroundf(value * 255.0f);
}

static void sort3(uint8_t values[3]) {
    if (values[0] > values[1]) {
        const uint8_t temporary = values[0];
        values[0] = values[1];
        values[1] = temporary;
    }
    if (values[1] > values[2]) {
        const uint8_t temporary = values[1];
        values[1] = values[2];
        values[2] = temporary;
    }
    if (values[0] > values[1]) {
        const uint8_t temporary = values[0];
        values[0] = values[1];
        values[1] = temporary;
    }
}

static bool close_byte(uint8_t actual, uint8_t expected) {
    const int difference = (int)actual - (int)expected;
    return difference >= -3 && difference <= 3;
}

static bool pixel_matches_clear(const uint8_t pixel[4], float variable) {
    uint8_t actual[3] = {pixel[0], pixel[1], pixel[2]};
    uint8_t expected[3] = {
        to_unorm8(variable),
        to_unorm8(0.25f),
        to_unorm8(0.75f),
    };
    sort3(actual);
    sort3(expected);
    return close_byte(actual[0], expected[0]) &&
           close_byte(actual[1], expected[1]) &&
           close_byte(actual[2], expected[2]) &&
           close_byte(pixel[3], 255);
}

static bool read_first_pixel(id<MTLTexture> texture, uint8_t pixel[4]) {
    if (!texture || texture.width == 0 || texture.height == 0) {
        fprintf(stderr, "current drawable texture is unavailable\n");
        return false;
    }
    id<MTLDevice> device = texture.device;
    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLBuffer> buffer =
        [device newBufferWithLength:256 options:MTLResourceStorageModeShared];
    if (!queue || !buffer) {
        fprintf(stderr, "Metal readback allocation failed\n");
        return false;
    }
    id<MTLCommandBuffer> command = [queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    [blit copyFromTexture:texture
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(1, 1, 1)
                 toBuffer:buffer
        destinationOffset:0
   destinationBytesPerRow:256
 destinationBytesPerImage:256];
    [blit endEncoding];
    [command commit];
    [command waitUntilCompleted];
    if (command.status != MTLCommandBufferStatusCompleted) {
        fprintf(stderr, "Metal readback failed: %s\n",
                command.error.localizedDescription.UTF8String);
        return false;
    }
    const uint8_t* bytes = (const uint8_t*)buffer.contents;
    pixel[0] = bytes[0];
    pixel[1] = bytes[1];
    pixel[2] = bytes[2];
    pixel[3] = bytes[3];
    return true;
}

static VkCompositeAlphaFlagBitsKHR choose_composite_alpha(
    VkCompositeAlphaFlagsKHR supported) {
    const VkCompositeAlphaFlagBitsKHR choices[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (size_t index = 0; index < sizeof(choices) / sizeof(choices[0]);
         ++index) {
        if (supported & choices[index]) {
            return choices[index];
        }
    }
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

int main(void) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 64, 64)];
        CAMetalLayer* layer = [CAMetalLayer layer];
        layer.drawableSize = CGSizeMake(64, 64);
        layer.framebufferOnly = NO;
        [view setWantsLayer:YES];
        [view setLayer:layer];
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(20, 20, 64, 64)
                      styleMask:NSWindowStyleMaskBorderless
                        backing:NSBackingStoreBuffered
                          defer:NO];
        [window setContentView:view];
        [window orderFrontRegardless];

        const char* instance_extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
        };
        const VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "teso4m4 swapchain texture-cache probe",
            .apiVersion = VK_API_VERSION_1_0,
        };
        const VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &application_info,
            .enabledExtensionCount = 2,
            .ppEnabledExtensionNames = instance_extensions,
        };
        VkInstance instance = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateInstance(&instance_info, NULL, &instance),
                   "vkCreateInstance")) {
            return 1;
        }

        const VkMacOSSurfaceCreateInfoMVK surface_info = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .pView = (__bridge void*)view,
        };
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateMacOSSurfaceMVK(
                       instance, &surface_info, NULL, &surface),
                   "vkCreateMacOSSurfaceMVK")) {
            return 1;
        }

        uint32_t physical_count = 0;
        if (!vk_ok(vkEnumeratePhysicalDevices(
                       instance, &physical_count, NULL),
                   "vkEnumeratePhysicalDevices(count)") ||
            physical_count == 0) {
            return 1;
        }
        VkPhysicalDevice* physical_devices =
            (VkPhysicalDevice*)calloc(physical_count,
                                      sizeof(*physical_devices));
        if (!physical_devices ||
            !vk_ok(vkEnumeratePhysicalDevices(
                       instance, &physical_count, physical_devices),
                   "vkEnumeratePhysicalDevices(data)")) {
            return 1;
        }
        const VkPhysicalDevice physical_device = physical_devices[0];
        free(physical_devices);

        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &family_count, NULL);
        VkQueueFamilyProperties* families =
            (VkQueueFamilyProperties*)calloc(family_count,
                                             sizeof(*families));
        if (!families) {
            return 1;
        }
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &family_count, families);
        uint32_t family_index = UINT32_MAX;
        for (uint32_t index = 0; index < family_count; ++index) {
            VkBool32 present_supported = VK_FALSE;
            if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    physical_device, index, surface, &present_supported) ==
                    VK_SUCCESS &&
                present_supported) {
                family_index = index;
                break;
            }
        }
        free(families);
        if (family_index == UINT32_MAX) {
            fprintf(stderr, "no graphics/present queue family\n");
            return 1;
        }

        const float queue_priority = 1.0f;
        const VkDeviceQueueCreateInfo queue_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        const char* device_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        const VkDeviceCreateInfo device_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = device_extensions,
        };
        VkDevice device = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateDevice(
                       physical_device, &device_info, NULL, &device),
                   "vkCreateDevice")) {
            return 1;
        }
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, family_index, 0, &queue);

        VkSurfaceCapabilitiesKHR capabilities = {};
        if (!vk_ok(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                       physical_device, surface, &capabilities),
                   "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")) {
            return 1;
        }
        if ((capabilities.supportedUsageFlags &
             (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) !=
            (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
            fprintf(stderr, "surface lacks color-attachment/transfer-src usage\n");
            return 2;
        }

        uint32_t format_count = 0;
        if (!vk_ok(vkGetPhysicalDeviceSurfaceFormatsKHR(
                       physical_device, surface, &format_count, NULL),
                   "vkGetPhysicalDeviceSurfaceFormatsKHR(count)")) {
            return 1;
        }
        VkSurfaceFormatKHR* formats =
            (VkSurfaceFormatKHR*)calloc(format_count, sizeof(*formats));
        if (!formats ||
            !vk_ok(vkGetPhysicalDeviceSurfaceFormatsKHR(
                       physical_device, surface, &format_count, formats),
                   "vkGetPhysicalDeviceSurfaceFormatsKHR(data)")) {
            return 1;
        }
        VkSurfaceFormatKHR surface_format = {};
        bool found_format = false;
        for (uint32_t index = 0; index < format_count; ++index) {
            if (formats[index].format == VK_FORMAT_B8G8R8A8_UNORM &&
                formats[index].colorSpace ==
                    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surface_format = formats[index];
                found_format = true;
                break;
            }
        }
        free(formats);
        if (!found_format) {
            fprintf(stderr, "required B8G8R8A8_UNORM surface format missing\n");
            return 2;
        }

        uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount != 0 &&
            image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }
        const VkSwapchainCreateInfoKHR swapchain_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = {64, 64},
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha =
                choose_composite_alpha(capabilities.supportedCompositeAlpha),
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
        };
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateSwapchainKHR(
                       device, &swapchain_info, NULL, &swapchain),
                   "vkCreateSwapchainKHR")) {
            return 1;
        }

        uint32_t swapchain_image_count = 0;
        if (!vk_ok(vkGetSwapchainImagesKHR(
                       device, swapchain, &swapchain_image_count, NULL),
                   "vkGetSwapchainImagesKHR(count)")) {
            return 1;
        }
        VkImage* images =
            (VkImage*)calloc(swapchain_image_count, sizeof(*images));
        VkImageView* image_views =
            (VkImageView*)calloc(swapchain_image_count,
                                 sizeof(*image_views));
        VkFramebuffer* framebuffers =
            (VkFramebuffer*)calloc(swapchain_image_count,
                                   sizeof(*framebuffers));
        VkCommandBuffer* commands =
            (VkCommandBuffer*)calloc(swapchain_image_count,
                                     sizeof(*commands));
        uintptr_t* prior_texture =
            (uintptr_t*)calloc(swapchain_image_count,
                               sizeof(*prior_texture));
        if (!images || !image_views || !framebuffers || !commands ||
            !prior_texture ||
            !vk_ok(vkGetSwapchainImagesKHR(
                       device, swapchain, &swapchain_image_count, images),
                   "vkGetSwapchainImagesKHR(data)")) {
            return 1;
        }

        for (uint32_t index = 0; index < swapchain_image_count; ++index) {
#if TESO4M4_PROBE_IDENTITY_VIEW
            const VkComponentMapping components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            };
#else
            const VkComponentMapping components = {
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_A,
            };
#endif
            const VkImageViewCreateInfo image_view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = images[index],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surface_format.format,
                .components = components,
                .subresourceRange = {
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    0,
                    1,
                    0,
                    1,
                },
            };
            if (!vk_ok(vkCreateImageView(
                           device, &image_view_info, NULL,
                           &image_views[index]),
                       "vkCreateImageView")) {
                return 1;
            }
        }

        const VkAttachmentDescription attachment = {
            .format = surface_format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        const VkAttachmentReference color_reference = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_reference,
        };
        const VkRenderPassCreateInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
        };
        VkRenderPass render_pass = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateRenderPass(
                       device, &render_pass_info, NULL, &render_pass),
                   "vkCreateRenderPass")) {
            return 1;
        }
        for (uint32_t index = 0; index < swapchain_image_count; ++index) {
            const VkFramebufferCreateInfo framebuffer_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass,
                .attachmentCount = 1,
                .pAttachments = &image_views[index],
                .width = 64,
                .height = 64,
                .layers = 1,
            };
            if (!vk_ok(vkCreateFramebuffer(
                           device, &framebuffer_info, NULL,
                           &framebuffers[index]),
                       "vkCreateFramebuffer")) {
                return 1;
            }
        }

        const VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = family_index,
        };
        VkCommandPool command_pool = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateCommandPool(
                       device, &pool_info, NULL, &command_pool),
                   "vkCreateCommandPool")) {
            return 1;
        }
        const VkCommandBufferAllocateInfo command_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = swapchain_image_count,
        };
        if (!vk_ok(vkAllocateCommandBuffers(
                       device, &command_info, commands),
                   "vkAllocateCommandBuffers")) {
            return 1;
        }

        const VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };
        VkSemaphore acquired = VK_NULL_HANDLE;
        VkSemaphore rendered = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        if (!vk_ok(vkCreateSemaphore(
                       device, &semaphore_info, NULL, &acquired),
                   "vkCreateSemaphore(acquired)") ||
            !vk_ok(vkCreateSemaphore(
                       device, &semaphore_info, NULL, &rendered),
                   "vkCreateSemaphore(rendered)") ||
            !vk_ok(vkCreateFence(device, &fence_info, NULL, &fence),
                   "vkCreateFence")) {
            return 1;
        }

        const float variables[] = {
            0.05f, 0.12f, 0.19f, 0.32f, 0.39f, 0.46f,
            0.53f, 0.60f, 0.67f, 0.82f, 0.89f, 0.96f,
        };
        uint32_t replacement_events = 0;
        bool stale_detected = false;
        for (uint32_t frame = 0;
             frame < sizeof(variables) / sizeof(variables[0]);
             ++frame) {
            uint32_t index = 0;
            VkResult acquire_result = vkAcquireNextImageKHR(
                device, swapchain, UINT64_MAX, acquired,
                VK_NULL_HANDLE, &index);
            if (acquire_result != VK_SUCCESS &&
                acquire_result != VK_SUBOPTIMAL_KHR) {
                fprintf(stderr, "vkAcquireNextImageKHR failed: %d\n",
                        acquire_result);
                return 1;
            }
            if (!vk_ok(vkResetCommandBuffer(commands[index], 0),
                       "vkResetCommandBuffer")) {
                return 1;
            }
            const VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            if (!vk_ok(vkBeginCommandBuffer(
                           commands[index], &begin_info),
                       "vkBeginCommandBuffer")) {
                return 1;
            }
            const VkClearValue clear = {
                .color = {{variables[frame], 0.25f, 0.75f, 1.0f}},
            };
            const VkRenderPassBeginInfo pass_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = render_pass,
                .framebuffer = framebuffers[index],
                .renderArea = {{0, 0}, {64, 64}},
                .clearValueCount = 1,
                .pClearValues = &clear,
            };
            vkCmdBeginRenderPass(
                commands[index], &pass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(commands[index]);
            if (!vk_ok(vkEndCommandBuffer(commands[index]),
                       "vkEndCommandBuffer")) {
                return 1;
            }
            const VkPipelineStageFlags wait_stage =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            const VkSubmitInfo submit = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &acquired,
                .pWaitDstStageMask = &wait_stage,
                .commandBufferCount = 1,
                .pCommandBuffers = &commands[index],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &rendered,
            };
            if (!vk_ok(vkQueueSubmit(queue, 1, &submit, fence),
                       "vkQueueSubmit") ||
                !vk_ok(vkWaitForFences(
                           device, 1, &fence, VK_TRUE, UINT64_MAX),
                       "vkWaitForFences")) {
                return 1;
            }

            id<MTLTexture> current_texture = nil;
            vkGetMTLTextureMVK(images[index], &current_texture);
            const uintptr_t current_identity =
                (uintptr_t)(__bridge void*)current_texture;
            const bool replaced =
                prior_texture[index] != 0 &&
                prior_texture[index] != current_identity;
            if (replaced) {
                ++replacement_events;
            }
            prior_texture[index] = current_identity;

            uint8_t pixel[4] = {};
            if (!read_first_pixel(current_texture, pixel)) {
                return 1;
            }
            const bool matches =
                pixel_matches_clear(pixel, variables[frame]);
            printf(
                "frame=%u image=%u texture=%p replaced=%s "
                "pixel=%u,%u,%u,%u expected-variable=%u match=%s\n",
                frame, index, (void*)current_identity,
                replaced ? "yes" : "no", pixel[0], pixel[1],
                pixel[2], pixel[3], to_unorm8(variables[frame]),
                matches ? "yes" : "NO");
            if (replaced && !matches) {
                stale_detected = true;
            }

            const VkPresentInfoKHR present_info = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &rendered,
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &index,
            };
            VkResult present_result =
                vkQueuePresentKHR(queue, &present_info);
            if (present_result != VK_SUCCESS &&
                present_result != VK_SUBOPTIMAL_KHR) {
                fprintf(stderr, "vkQueuePresentKHR failed: %d\n",
                        present_result);
                return 1;
            }
            if (!vk_ok(vkQueueWaitIdle(queue), "vkQueueWaitIdle") ||
                !vk_ok(vkResetFences(device, 1, &fence),
                       "vkResetFences")) {
                return 1;
            }
            NSDate* until =
                [NSDate dateWithTimeIntervalSinceNow:0.002];
            [[NSRunLoop currentRunLoop] runUntilDate:until];
        }

        vkDeviceWaitIdle(device);
        vkDestroyFence(device, fence, NULL);
        vkDestroySemaphore(device, rendered, NULL);
        vkDestroySemaphore(device, acquired, NULL);
        vkDestroyCommandPool(device, command_pool, NULL);
        for (uint32_t index = 0; index < swapchain_image_count; ++index) {
            vkDestroyFramebuffer(device, framebuffers[index], NULL);
            vkDestroyImageView(device, image_views[index], NULL);
        }
        vkDestroyRenderPass(device, render_pass, NULL);
        vkDestroySwapchainKHR(device, swapchain, NULL);
        vkDestroyDevice(device, NULL);
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        [window orderOut:nil];
        [window close];
        free(prior_texture);
        free(commands);
        free(framebuffers);
        free(image_views);
        free(images);

        if (replacement_events == 0) {
            fprintf(stderr,
                    "probe inconclusive: no swapchain image received a "
                    "different CAMetalDrawable texture\n");
            return 2;
        }
        if (stale_detected) {
            printf(
                "swapchain texture-cache probe: STALE view=%s "
                "replacement_events=%u\n",
#if TESO4M4_PROBE_IDENTITY_VIEW
                "identity",
#else
                "swizzled",
#endif
                replacement_events);
            return 3;
        }
        printf(
            "swapchain texture-cache probe: PASS view=%s "
            "replacement_events=%u\n",
#if TESO4M4_PROBE_IDENTITY_VIEW
            "identity",
#else
            "swizzled",
#endif
            replacement_events);
        return 0;
    }
}
