#define VK_USE_PLATFORM_MACOS_MVK 1

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <vulkan/vulkan.h>

extern "C" {
#include "mvk_lifecycle.h"
#include "mvk_present_pixel.h"
}

extern "C" void VKAPI_CALL vkGetMTLTextureMVK(
    VkImage image, id<MTLTexture>* mtl_texture);

typedef enum {
    kLoadOnly,
    kClearBlack,
    kClearNeonPink,
    kDrawNeonPink,
} FirstFrameMode;

typedef struct {
    VkSwapchainKHR swapchain;
    VkImage* images;
    VkImageView* views;
    VkFramebuffer* framebuffers;
    uint32_t image_count;
    VkRenderPass render_pass;
} Generation;

static char g_audit_log[65536];
static size_t g_audit_log_length;
static PFN_vkCreateSwapchainKHR g_create_swapchain = vkCreateSwapchainKHR;
static PFN_vkGetSwapchainImagesKHR g_get_swapchain_images =
    vkGetSwapchainImagesKHR;
static PFN_vkCreateImageView g_create_image_view = vkCreateImageView;
static PFN_vkCreateRenderPass g_create_render_pass = vkCreateRenderPass;
static PFN_vkCreateFramebuffer g_create_framebuffer = vkCreateFramebuffer;
static PFN_vkCmdBeginRenderPass g_cmd_begin_render_pass = vkCmdBeginRenderPass;
static PFN_vkCmdClearAttachments g_cmd_clear_attachments =
    vkCmdClearAttachments;
static PFN_vkCmdEndRenderPass g_cmd_end_render_pass = vkCmdEndRenderPass;
static PFN_vkAcquireNextImageKHR g_acquire_next_image =
    vkAcquireNextImageKHR;
static PFN_vkQueuePresentKHR g_queue_present = vkQueuePresentKHR;
static PFN_vkQueueSubmit g_queue_submit = vkQueueSubmit;
static PFN_vkBeginCommandBuffer g_begin_command_buffer = vkBeginCommandBuffer;
static PFN_vkCreateShaderModule g_create_shader_module = vkCreateShaderModule;
static PFN_vkCreateGraphicsPipelines g_create_graphics_pipelines =
    vkCreateGraphicsPipelines;
static PFN_vkCmdBindPipeline g_cmd_bind_pipeline = vkCmdBindPipeline;
static PFN_vkCmdDraw g_cmd_draw = vkCmdDraw;

static void audit_log(const char* message) {
    const int written = snprintf(
        g_audit_log + g_audit_log_length,
        sizeof(g_audit_log) - g_audit_log_length, "%s\n", message);
    if (written > 0 &&
        (size_t)written < sizeof(g_audit_log) - g_audit_log_length) {
        g_audit_log_length += (size_t)written;
    }
}

static void enable_startup_color_audit(void) {
    teso4m4_lifecycle_reset();
    teso4m4_lifecycle_set_logger(&audit_log);
    teso4m4_present_pixel_reset();
    if (!teso4m4_present_pixel_configure(
            (PFN_vkVoidFunction)vkGetMTLTextureMVK,
            (PFN_vkQueueWaitIdle)vkQueueWaitIdle,
            &audit_log)) {
        fprintf(stderr, "present pixel sampler configuration failed\n");
        exit(3);
    }
    teso4m4_lifecycle_set_present_pixel_sampler(
        &teso4m4_present_pixel_sample);
    teso4m4_lifecycle_set_startup_color_audit(true);
    teso4m4_lifecycle_set_startup_present_pixel_audit(true);
    teso4m4_lifecycle_set_startup_draw_audit(true);
    g_create_swapchain = (PFN_vkCreateSwapchainKHR)
        teso4m4_lifecycle_intercept(
            "vkCreateSwapchainKHR",
            (PFN_vkVoidFunction)vkCreateSwapchainKHR);
    g_get_swapchain_images = (PFN_vkGetSwapchainImagesKHR)
        teso4m4_lifecycle_intercept(
            "vkGetSwapchainImagesKHR",
            (PFN_vkVoidFunction)vkGetSwapchainImagesKHR);
    g_create_image_view = (PFN_vkCreateImageView)
        teso4m4_lifecycle_intercept(
            "vkCreateImageView", (PFN_vkVoidFunction)vkCreateImageView);
    g_create_render_pass = (PFN_vkCreateRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCreateRenderPass", (PFN_vkVoidFunction)vkCreateRenderPass);
    g_create_framebuffer = (PFN_vkCreateFramebuffer)
        teso4m4_lifecycle_intercept(
            "vkCreateFramebuffer", (PFN_vkVoidFunction)vkCreateFramebuffer);
    g_cmd_begin_render_pass = (PFN_vkCmdBeginRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCmdBeginRenderPass", (PFN_vkVoidFunction)vkCmdBeginRenderPass);
    g_cmd_clear_attachments = (PFN_vkCmdClearAttachments)
        teso4m4_lifecycle_intercept(
            "vkCmdClearAttachments",
            (PFN_vkVoidFunction)vkCmdClearAttachments);
    g_cmd_end_render_pass = (PFN_vkCmdEndRenderPass)
        teso4m4_lifecycle_intercept(
            "vkCmdEndRenderPass", (PFN_vkVoidFunction)vkCmdEndRenderPass);
    g_acquire_next_image = (PFN_vkAcquireNextImageKHR)
        teso4m4_lifecycle_intercept(
            "vkAcquireNextImageKHR",
            (PFN_vkVoidFunction)vkAcquireNextImageKHR);
    g_queue_present = (PFN_vkQueuePresentKHR)
        teso4m4_lifecycle_intercept(
            "vkQueuePresentKHR", (PFN_vkVoidFunction)vkQueuePresentKHR);
    g_queue_submit = (PFN_vkQueueSubmit)
        teso4m4_lifecycle_intercept(
            "vkQueueSubmit", (PFN_vkVoidFunction)vkQueueSubmit);
    g_begin_command_buffer = (PFN_vkBeginCommandBuffer)
        teso4m4_lifecycle_intercept(
            "vkBeginCommandBuffer",
            (PFN_vkVoidFunction)vkBeginCommandBuffer);
    g_create_shader_module = (PFN_vkCreateShaderModule)
        teso4m4_lifecycle_intercept(
            "vkCreateShaderModule",
            (PFN_vkVoidFunction)vkCreateShaderModule);
    g_create_graphics_pipelines = (PFN_vkCreateGraphicsPipelines)
        teso4m4_lifecycle_intercept(
            "vkCreateGraphicsPipelines",
            (PFN_vkVoidFunction)vkCreateGraphicsPipelines);
    g_cmd_bind_pipeline = (PFN_vkCmdBindPipeline)
        teso4m4_lifecycle_intercept(
            "vkCmdBindPipeline",
            (PFN_vkVoidFunction)vkCmdBindPipeline);
    g_cmd_draw = (PFN_vkCmdDraw)
        teso4m4_lifecycle_intercept(
            "vkCmdDraw", (PFN_vkVoidFunction)vkCmdDraw);
}

static bool vk_ok(VkResult result, const char* operation) {
    if (result == VK_SUCCESS) {
        return true;
    }
    fprintf(stderr, "%s failed: %d\n", operation, result);
    return false;
}

static VkCompositeAlphaFlagBitsKHR choose_composite_alpha(
    VkCompositeAlphaFlagsKHR supported) {
    const VkCompositeAlphaFlagBitsKHR choices[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (size_t index = 0; index < sizeof(choices) / sizeof(choices[0]); ++index) {
        if (supported & choices[index]) {
            return choices[index];
        }
    }
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

static bool read_pixel(id<MTLTexture> texture, uint8_t pixel[4]) {
    if (!texture || texture.width == 0 || texture.height == 0) {
        fprintf(stderr, "drawable texture unavailable\n");
        return false;
    }
    id<MTLCommandQueue> queue = [texture.device newCommandQueue];
    id<MTLBuffer> buffer =
        [texture.device newBufferWithLength:256
                                    options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> command = [queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    if (!queue || !buffer || !command || !blit) {
        fprintf(stderr, "Metal readback allocation failed\n");
        return false;
    }
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
    memcpy(pixel, buffer.contents, 4);
    return true;
}

static bool create_generation(
    VkPhysicalDevice physical_device,
    VkDevice device,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surface_format,
    VkExtent2D extent,
    VkSwapchainKHR old_swapchain,
    Generation* generation) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    if (!vk_ok(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                   physical_device, surface, &capabilities),
               "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")) {
        return false;
    }
    const VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha =
            choose_composite_alpha(capabilities.supportedCompositeAlpha),
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };
    if (!vk_ok(g_create_swapchain(
                   device, &swapchain_info, NULL, &generation->swapchain),
               "vkCreateSwapchainKHR")) {
        return false;
    }
    if (!vk_ok(g_get_swapchain_images(
                   device, generation->swapchain,
                   &generation->image_count, NULL),
               "vkGetSwapchainImagesKHR(count)")) {
        return false;
    }
    generation->images = (VkImage*)calloc(
        generation->image_count, sizeof(*generation->images));
    generation->views = (VkImageView*)calloc(
        generation->image_count, sizeof(*generation->views));
    generation->framebuffers = (VkFramebuffer*)calloc(
        generation->image_count, sizeof(*generation->framebuffers));
    if (!generation->images || !generation->views ||
        !generation->framebuffers ||
        !vk_ok(g_get_swapchain_images(
                   device, generation->swapchain,
                   &generation->image_count, generation->images),
               "vkGetSwapchainImagesKHR(data)")) {
        return false;
    }

    const VkAttachmentDescription attachment = {
        .format = surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
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
    if (!vk_ok(g_create_render_pass(
                   device, &render_pass_info, NULL,
                   &generation->render_pass),
               "vkCreateRenderPass")) {
        return false;
    }
    for (uint32_t index = 0; index < generation->image_count; ++index) {
        const VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = generation->images[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
            },
        };
        if (!vk_ok(g_create_image_view(
                       device, &view_info, NULL, &generation->views[index]),
                   "vkCreateImageView")) {
            return false;
        }
        const VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = generation->render_pass,
            .attachmentCount = 1,
            .pAttachments = &generation->views[index],
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };
        if (!vk_ok(g_create_framebuffer(
                       device, &framebuffer_info, NULL,
                       &generation->framebuffers[index]),
                   "vkCreateFramebuffer")) {
            return false;
        }
    }
    return true;
}

static void destroy_generation(VkDevice device, Generation* generation) {
    for (uint32_t index = 0; index < generation->image_count; ++index) {
        if (generation->framebuffers[index]) {
            vkDestroyFramebuffer(device, generation->framebuffers[index], NULL);
        }
        if (generation->views[index]) {
            vkDestroyImageView(device, generation->views[index], NULL);
        }
    }
    if (generation->render_pass) {
        vkDestroyRenderPass(device, generation->render_pass, NULL);
    }
    if (generation->swapchain) {
        vkDestroySwapchainKHR(device, generation->swapchain, NULL);
    }
    free(generation->framebuffers);
    free(generation->views);
    free(generation->images);
    *generation = {};
}

static VkShaderModule load_shader_module(
    VkDevice device, const char* environment_name) {
    const char* path = getenv(environment_name);
    if (!path) {
        fprintf(stderr, "%s is unset\n", environment_name);
        return VK_NULL_HANDLE;
    }
    FILE* file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END) != 0) {
        if (file) {
            fclose(file);
        }
        fprintf(stderr, "could not open shader: %s\n", path);
        return VK_NULL_HANDLE;
    }
    const long length = ftell(file);
    if (length <= 0 || (length % 4) != 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "invalid shader: %s\n", path);
        return VK_NULL_HANDLE;
    }
    uint32_t* code = (uint32_t*)malloc((size_t)length);
    if (!code || fread(code, 1, (size_t)length, file) != (size_t)length) {
        free(code);
        fclose(file);
        fprintf(stderr, "could not read shader: %s\n", path);
        return VK_NULL_HANDLE;
    }
    fclose(file);
    const VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)length,
        .pCode = code,
    };
    VkShaderModule module = VK_NULL_HANDLE;
    const VkResult result =
        g_create_shader_module(device, &info, NULL, &module);
    free(code);
    return vk_ok(result, "vkCreateShaderModule")
        ? module
        : VK_NULL_HANDLE;
}

static VkPipeline create_magenta_pipeline(
    VkDevice device,
    VkRenderPass render_pass,
    VkExtent2D extent,
    VkPipelineLayout* layout,
    VkShaderModule* vertex,
    VkShaderModule* fragment) {
    *vertex = load_shader_module(device, "TESO4M4_STARTUP_VERTEX_SPV");
    *fragment = load_shader_module(device, "TESO4M4_STARTUP_FRAGMENT_SPV");
    if (!*vertex || !*fragment) {
        return VK_NULL_HANDLE;
    }
    const VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    if (!vk_ok(
            vkCreatePipelineLayout(device, &layout_info, NULL, layout),
            "vkCreatePipelineLayout")) {
        return VK_NULL_HANDLE;
    }
    const VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *vertex,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *fragment,
            .pName = "main",
        },
    };
    const VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    const VkPipelineInputAssemblyStateCreateInfo assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    const VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0,
        .maxDepth = 1,
    };
    const VkRect2D scissor = {{0, 0}, extent};
    const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    const VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1,
    };
    const VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    const VkPipelineColorBlendAttachmentState blend_attachment = {
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
    };
    const VkGraphicsPipelineCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pColorBlendState = &blend,
        .layout = *layout,
        .renderPass = render_pass,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    return vk_ok(
        g_create_graphics_pipelines(
            device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline),
        "vkCreateGraphicsPipelines") ? pipeline : VK_NULL_HANDLE;
}

static bool submit_frame(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    Generation* generation,
    VkExtent2D extent,
    FirstFrameMode mode,
    uint8_t pixel[4]) {
    VkSemaphore acquired = VK_NULL_HANDLE;
    VkSemaphore rendered = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    const VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    const VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    if (!vk_ok(vkCreateSemaphore(device, &semaphore_info, NULL, &acquired),
               "vkCreateSemaphore(acquired)") ||
        !vk_ok(vkCreateSemaphore(device, &semaphore_info, NULL, &rendered),
               "vkCreateSemaphore(rendered)") ||
        !vk_ok(vkCreateFence(device, &fence_info, NULL, &fence),
               "vkCreateFence")) {
        return false;
    }
    uint32_t image_index = 0;
    VkResult acquire_result = g_acquire_next_image(
        device, generation->swapchain, UINT64_MAX, acquired,
        VK_NULL_HANDLE, &image_index);
    if (acquire_result != VK_SUCCESS &&
        acquire_result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "vkAcquireNextImageKHR failed: %d\n", acquire_result);
        return false;
    }

    VkCommandBuffer command = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkCommandBufferAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (!vk_ok(vkAllocateCommandBuffers(device, &allocation_info, &command),
               "vkAllocateCommandBuffers")) {
        return false;
    }
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    if (mode == kDrawNeonPink) {
        pipeline = create_magenta_pipeline(
            device, generation->render_pass, extent, &pipeline_layout,
            &vertex, &fragment);
        if (!pipeline) {
            return false;
        }
    }
    if (!vk_ok(g_begin_command_buffer(command, &begin_info),
               "vkBeginCommandBuffer")) {
        return false;
    }
    const VkRenderPassBeginInfo pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = generation->render_pass,
        .framebuffer = generation->framebuffers[image_index],
        .renderArea = {{0, 0}, extent},
    };
    g_cmd_begin_render_pass(command, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
    if (mode == kDrawNeonPink) {
        g_cmd_bind_pipeline(
            command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        g_cmd_draw(command, 3, 1, 0, 0);
    } else if (mode != kLoadOnly) {
        const VkClearAttachment attachment = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .colorAttachment = 0,
            .clearValue = {
                .color = {{
                    mode == kClearNeonPink ? 1.0f : 0.0f,
                    0.0f,
                    mode == kClearNeonPink ? 1.0f : 0.0f,
                    1.0f,
                }},
            },
        };
        const VkClearRect rect = {
            .rect = {{0, 0}, extent},
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        g_cmd_clear_attachments(command, 1, &attachment, 1, &rect);
    }
    g_cmd_end_render_pass(command);
    if (!vk_ok(vkEndCommandBuffer(command), "vkEndCommandBuffer")) {
        return false;
    }
    const VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquired,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &rendered,
    };
    if (!vk_ok(g_queue_submit(queue, 1, &submit_info, fence),
               "vkQueueSubmit") ||
        !vk_ok(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX),
               "vkWaitForFences")) {
        return false;
    }
    id<MTLTexture> texture = nil;
    vkGetMTLTextureMVK(generation->images[image_index], &texture);
    if (!read_pixel(texture, pixel)) {
        return false;
    }
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &rendered,
        .swapchainCount = 1,
        .pSwapchains = &generation->swapchain,
        .pImageIndices = &image_index,
    };
    VkResult present_result = g_queue_present(queue, &present_info);
    if (present_result != VK_SUCCESS &&
        present_result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "vkQueuePresentKHR failed: %d\n", present_result);
        return false;
    }
    vkQueueWaitIdle(queue);
    if (pipeline) {
        vkDestroyPipeline(device, pipeline, NULL);
    }
    if (pipeline_layout) {
        vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    }
    if (fragment) {
        vkDestroyShaderModule(device, fragment, NULL);
    }
    if (vertex) {
        vkDestroyShaderModule(device, vertex, NULL);
    }
    vkFreeCommandBuffers(device, command_pool, 1, &command);
    vkDestroyFence(device, fence, NULL);
    vkDestroySemaphore(device, rendered, NULL);
    vkDestroySemaphore(device, acquired, NULL);
    return true;
}

static bool present_without_rendering(
    VkDevice device,
    VkQueue queue,
    Generation* generation) {
    VkSemaphore acquired = VK_NULL_HANDLE;
    const VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    if (!vk_ok(vkCreateSemaphore(device, &semaphore_info, NULL, &acquired),
               "vkCreateSemaphore(audit-bound)")) {
        return false;
    }
    uint32_t image_index = 0;
    const VkResult acquire_result = g_acquire_next_image(
        device, generation->swapchain, UINT64_MAX, acquired,
        VK_NULL_HANDLE, &image_index);
    if (acquire_result != VK_SUCCESS &&
        acquire_result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "audit-bound acquire failed: %d\n", acquire_result);
        return false;
    }
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquired,
        .swapchainCount = 1,
        .pSwapchains = &generation->swapchain,
        .pImageIndices = &image_index,
    };
    const VkResult present_result = g_queue_present(queue, &present_info);
    if (present_result != VK_SUCCESS &&
        present_result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "audit-bound present failed: %d\n", present_result);
        return false;
    }
    vkQueueWaitIdle(queue);
    vkDestroySemaphore(device, acquired, NULL);
    return true;
}

static FirstFrameMode parse_mode(const char* value) {
    if (strcmp(value, "load") == 0) {
        return kLoadOnly;
    }
    if (strcmp(value, "black") == 0) {
        return kClearBlack;
    }
    if (strcmp(value, "neon-pink") == 0) {
        return kClearNeonPink;
    }
    if (strcmp(value, "draw-neon-pink") == 0) {
        return kDrawNeonPink;
    }
    fprintf(
        stderr,
        "mode must be load, black, neon-pink, or draw-neon-pink\n");
    exit(2);
}

int main(int argc, char** argv) {
    @autoreleasepool {
        if (argc != 2 && argc != 4) {
            fprintf(
                stderr,
                "usage: %s load|black|neon-pink|draw-neon-pink "
                "[width corrected-height]\n",
                argv[0]);
            return 2;
        }
        const FirstFrameMode first_mode = parse_mode(argv[1]);
        const uint32_t width =
            argc == 4 ? (uint32_t)strtoul(argv[2], NULL, 10) : 64;
        const uint32_t corrected_height =
            argc == 4 ? (uint32_t)strtoul(argv[3], NULL, 10) : 64;
        if (width == 0 || corrected_height == 0 ||
            corrected_height == UINT32_MAX) {
            fprintf(stderr, "extent must contain positive 32-bit values\n");
            return 2;
        }
        const uint32_t first_height = corrected_height + 2;
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
        NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 64, 66)];
        CAMetalLayer* layer = [CAMetalLayer layer];
        layer.drawableSize = CGSizeMake(width, first_height);
        layer.framebufferOnly = NO;
        [view setWantsLayer:YES];
        [view setLayer:layer];
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(20, 20, 64, 66)
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
            .pApplicationName = "teso4m4 startup surface probe",
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
        if (!vk_ok(vkEnumeratePhysicalDevices(instance, &physical_count, NULL),
                   "vkEnumeratePhysicalDevices(count)") ||
            physical_count == 0) {
            return 1;
        }
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        physical_count = 1;
        if (!vk_ok(vkEnumeratePhysicalDevices(
                       instance, &physical_count, &physical_device),
                   "vkEnumeratePhysicalDevices(data)")) {
            return 1;
        }
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &family_count, NULL);
        VkQueueFamilyProperties* families = (VkQueueFamilyProperties*)calloc(
            family_count, sizeof(*families));
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
        const float priority = 1.0f;
        const VkDeviceQueueCreateInfo queue_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };
        const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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
        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &format_count, NULL);
        VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)calloc(
            format_count, sizeof(*formats));
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &format_count, formats);
        VkSurfaceFormatKHR surface_format = {};
        for (uint32_t index = 0; index < format_count; ++index) {
            if (formats[index].format == VK_FORMAT_B8G8R8A8_UNORM &&
                formats[index].colorSpace ==
                    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surface_format = formats[index];
                break;
            }
        }
        free(formats);
        if (surface_format.format == VK_FORMAT_UNDEFINED) {
            fprintf(stderr, "B8G8R8A8_UNORM surface format unavailable\n");
            return 1;
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

        enable_startup_color_audit();
        const VkExtent2D first_extent = {width, first_height};
        Generation first = {};
        if (!create_generation(
                physical_device, device, surface, surface_format,
                first_extent, VK_NULL_HANDLE, &first)) {
            return 1;
        }
        uint8_t first_pixel[4] = {};
        if (!submit_frame(
                device, queue, command_pool, &first, first_extent,
                first_mode, first_pixel)) {
            return 1;
        }

        layer.drawableSize = CGSizeMake(width, corrected_height);
        const VkExtent2D corrected_extent = {width, corrected_height};
        Generation corrected = {};
        if (!create_generation(
                physical_device, device, surface, surface_format,
                corrected_extent, first.swapchain, &corrected)) {
            return 1;
        }
        uint8_t corrected_pixel[4] = {};
        if (!submit_frame(
                device, queue, command_pool, &corrected, corrected_extent,
                kClearBlack, corrected_pixel)) {
            return 1;
        }
        for (uint32_t ordinal = 1; ordinal < 180; ++ordinal) {
            if (!present_without_rendering(device, queue, &corrected)) {
                return 1;
            }
        }
        printf(
            "mode=%s first=%ux%u pixel-bgra=%u,%u,%u,%u "
            "corrected=%ux%u pixel-bgra=%u,%u,%u,%u\n",
            argv[1], width, first_height, first_pixel[0], first_pixel[1],
            first_pixel[2], first_pixel[3], width, corrected_height,
            corrected_pixel[0], corrected_pixel[1], corrected_pixel[2],
            corrected_pixel[3]);
        fputs(g_audit_log, stdout);

        vkDeviceWaitIdle(device);
        destroy_generation(device, &corrected);
        destroy_generation(device, &first);
        vkDestroyCommandPool(device, command_pool, NULL);
        vkDestroyDevice(device, NULL);
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        [window orderOut:nil];

        if ((first_mode == kClearNeonPink ||
             first_mode == kDrawNeonPink) &&
            !(first_pixel[0] >= 252 && first_pixel[1] <= 3 &&
              first_pixel[2] >= 252 && first_pixel[3] >= 252)) {
            fprintf(stderr, "neon-pink control did not produce BGRA 255,0,255,255\n");
            return 3;
        }
        if (first_mode == kClearBlack &&
            !(first_pixel[0] <= 3 && first_pixel[1] <= 3 &&
              first_pixel[2] <= 3 && first_pixel[3] >= 252)) {
            fprintf(stderr, "black first-frame control failed\n");
            return 3;
        }
        if (first_mode == kLoadOnly &&
            first_pixel[0] >= 252 && first_pixel[1] <= 3 &&
            first_pixel[2] >= 252) {
            fprintf(stderr, "load-only first frame unexpectedly matched neon pink\n");
            return 3;
        }
        const bool has_audit_clear =
            strstr(
                g_audit_log,
                "STARTUP_COLOR_CLEAR: generation=1") != NULL;
        const char* expected_audit_rgba =
            first_mode == kClearNeonPink
                ? "rgba=1,0,1,1"
                : "rgba=0,0,0,1";
        if (strstr(g_audit_log, "STARTUP_COLOR_BEGIN: generation=1") == NULL ||
            strstr(g_audit_log, "framebuffer_extent=") == NULL ||
            strstr(g_audit_log, "STARTUP_COLOR_SUBMIT: generation=1") == NULL ||
            strstr(g_audit_log, "STARTUP_COLOR_BEGIN: generation=2") == NULL ||
            strstr(g_audit_log, "STARTUP_COLOR_SUBMIT: generation=2") == NULL ||
            strstr(g_audit_log,
                   "STARTUP_COLOR_AUDIT_FINISH: "
                   "reason=generation-2-present-limit generation=2 "
                   "ordinal=180") == NULL ||
            strstr(g_audit_log, " ordinal=9 ") != NULL ||
            ((first_mode == kLoadOnly || first_mode == kDrawNeonPink) &&
             has_audit_clear) ||
            ((first_mode == kClearBlack ||
              first_mode == kClearNeonPink) &&
             (!has_audit_clear ||
              strstr(g_audit_log, expected_audit_rgba) == NULL))) {
            fprintf(stderr, "startup color audit did not classify %s\n", argv[1]);
            return 3;
        }
        const char* expected_pixel_summary =
            (first_mode == kClearNeonPink ||
             first_mode == kDrawNeonPink)
                ? "STARTUP_PRESENT_PIXEL_SUMMARY: generation=1 ordinal=1"
                  " image_index="
                : "STARTUP_PRESENT_PIXEL_SUMMARY: generation=2 ordinal=1"
                  " image_index=";
        const char* expected_pixel_class =
            (first_mode == kClearNeonPink ||
             first_mode == kDrawNeonPink)
                ? "exact_magenta=5 near_magenta=5 black=0"
                : "exact_magenta=0 near_magenta=0 black=5";
        const char* summary = strstr(g_audit_log, expected_pixel_summary);
        if (!summary || !strstr(summary, expected_pixel_class)) {
            fprintf(stderr, "present pixel audit did not classify %s\n", argv[1]);
            return 3;
        }
        if (first_mode == kDrawNeonPink &&
            (!strstr(
                 g_audit_log,
                 "STARTUP_PRESENT_DRAW_SUMMARY: generation=1 ordinal=1") ||
             !strstr(
                 g_audit_log,
                 "draw_count=1 indexed_draw_count=0 distinct_pipelines=1 "
                 "pipeline_overflow=no") ||
             !strstr(
                 g_audit_log,
                 "STARTUP_PRESENT_DRAW_PIPELINE: generation=1 ordinal=1") ||
             !strstr(g_audit_log, "shader_hash_complete=yes") ||
             !strstr(g_audit_log, "pipeline_state=tracked"))) {
            fprintf(stderr, "draw provenance did not match magenta frame\n");
            return 3;
        }
        if (!(corrected_pixel[0] <= 3 && corrected_pixel[1] <= 3 &&
              corrected_pixel[2] <= 3 && corrected_pixel[3] >= 252)) {
            fprintf(stderr, "black correction control failed\n");
            return 3;
        }
        return 0;
    }
}
