#define VK_USE_PLATFORM_MACOS_MVK 1

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <vulkan/vulkan.h>

extern "C" void VKAPI_CALL vkGetMTLTextureMVK(
    VkImage image, id<MTLTexture>* mtl_texture);

static const VkFormat kFormat = VK_FORMAT_B8G8R8A8_UNORM;
static const VkExtent2D kOutputExtent = {342, 214};
static const uint32_t kCycleCount = 24;
static const uint32_t kBenchmarkSampleCount = 7;

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
} RenderTarget;

static bool vk_ok(VkResult result, const char* operation) {
    if (result == VK_SUCCESS) {
        return true;
    }
    fprintf(stderr, "%s failed: %d\n", operation, result);
    return false;
}

static uint64_t monotonic_nanoseconds(void) {
    struct timespec now = {};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * 1000000000ull +
           (uint64_t)now.tv_nsec;
}

static uint8_t to_unorm8(float value) {
    return (uint8_t)lroundf(value * 255.0f);
}

static bool close_byte(uint8_t actual, uint8_t expected) {
    const int difference = (int)actual - (int)expected;
    return difference >= -3 && difference <= 3;
}

static bool read_first_pixel(id<MTLTexture> texture, uint8_t pixel[4]) {
    if (!texture || texture.width == 0 || texture.height == 0) {
        fprintf(stderr, "output Metal texture is unavailable\n");
        return false;
    }
    id<MTLCommandQueue> queue = [texture.device newCommandQueue];
    id<MTLBuffer> buffer =
        [texture.device newBufferWithLength:256
                                    options:MTLResourceStorageModeShared];
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
    memcpy(pixel, bytes, 4);
    return true;
}

static bool pixel_matches(uint8_t pixel[4], float red) {
    return close_byte(pixel[0], to_unorm8(0.75f)) &&
           close_byte(pixel[1], to_unorm8(0.25f)) &&
           close_byte(pixel[2], to_unorm8(red)) &&
           close_byte(pixel[3], 255);
}

static bool choose_memory_type(
    VkPhysicalDevice physical_device, uint32_t type_bits,
    VkMemoryPropertyFlags preferred, uint32_t* type_index) {
    VkPhysicalDeviceMemoryProperties properties = {};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if ((type_bits & (1u << index)) &&
            (properties.memoryTypes[index].propertyFlags & preferred) ==
                preferred) {
            *type_index = index;
            return true;
        }
    }
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if (type_bits & (1u << index)) {
            *type_index = index;
            return true;
        }
    }
    return false;
}

static bool create_render_pass(
    VkDevice device, VkImageLayout final_layout, bool sampled_output,
    VkRenderPass* render_pass) {
    const VkAttachmentDescription attachment = {
        .format = kFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = final_layout,
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
    const VkSubpassDependency dependency = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = sampled_output
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            : VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = sampled_output
            ? VK_ACCESS_SHADER_READ_BIT
            : VK_ACCESS_TRANSFER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    };
    const VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    return vk_ok(
        vkCreateRenderPass(device, &info, NULL, render_pass),
        "vkCreateRenderPass");
}

static bool create_target(
    VkPhysicalDevice physical_device, VkDevice device, VkExtent2D extent,
    VkImageUsageFlags usage, VkImageLayout final_layout,
    bool sampled_output, RenderTarget* target) {
    memset(target, 0, sizeof(*target));
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = kFormat,
        .extent = {extent.width, extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!vk_ok(
            vkCreateImage(device, &image_info, NULL, &target->image),
            "vkCreateImage")) {
        return false;
    }
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(device, target->image, &requirements);
    uint32_t type_index = 0;
    if (!choose_memory_type(
            physical_device, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &type_index)) {
        fprintf(stderr, "no compatible image memory type\n");
        return false;
    }
    const VkMemoryAllocateInfo allocation = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = type_index,
    };
    if (!vk_ok(
            vkAllocateMemory(device, &allocation, NULL, &target->memory),
            "vkAllocateMemory") ||
        !vk_ok(
            vkBindImageMemory(device, target->image, target->memory, 0),
            "vkBindImageMemory")) {
        return false;
    }
    const VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = target->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = kFormat,
        .components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            1,
            0,
            1,
        },
    };
    if (!vk_ok(
            vkCreateImageView(device, &view_info, NULL, &target->view),
            "vkCreateImageView") ||
        !create_render_pass(
            device, final_layout, sampled_output, &target->render_pass)) {
        return false;
    }
    const VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = target->render_pass,
        .attachmentCount = 1,
        .pAttachments = &target->view,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };
    return vk_ok(
        vkCreateFramebuffer(
            device, &framebuffer_info, NULL, &target->framebuffer),
        "vkCreateFramebuffer");
}

static void destroy_target(VkDevice device, RenderTarget* target) {
    if (target->framebuffer) {
        vkDestroyFramebuffer(device, target->framebuffer, NULL);
    }
    if (target->render_pass) {
        vkDestroyRenderPass(device, target->render_pass, NULL);
    }
    if (target->view) {
        vkDestroyImageView(device, target->view, NULL);
    }
    if (target->image) {
        vkDestroyImage(device, target->image, NULL);
    }
    if (target->memory) {
        vkFreeMemory(device, target->memory, NULL);
    }
    memset(target, 0, sizeof(*target));
}

static VkShaderModule create_spirv_module(
    VkDevice device, const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        perror(path);
        return VK_NULL_HANDLE;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return VK_NULL_HANDLE;
    }
    const long length = ftell(file);
    if (length <= 0 || (length % 4) != 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "invalid SPIR-V file: %s\n", path);
        fclose(file);
        return VK_NULL_HANDLE;
    }
    uint32_t* code = (uint32_t*)malloc((size_t)length);
    if (!code ||
        fread(code, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "failed to read SPIR-V file: %s\n", path);
        free(code);
        fclose(file);
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
        vkCreateShaderModule(device, &info, NULL, &module);
    free(code);
    if (!vk_ok(result, "vkCreateShaderModule(SPIR-V)")) {
        return VK_NULL_HANDLE;
    }
    return module;
}

static VkPipeline create_pipeline(
    VkDevice device, VkPipelineLayout layout, VkRenderPass render_pass,
    VkShaderModule vertex, VkShaderModule fragment, VkExtent2D extent) {
    const VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment,
            .pName = "main",
        },
    };
    const VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    const VkPipelineInputAssemblyStateCreateInfo assembly = {
        .sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
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
        .sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
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
        .layout = layout,
        .renderPass = render_pass,
        .subpass = 0,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!vk_ok(
            vkCreateGraphicsPipelines(
                device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline),
            "vkCreateGraphicsPipelines")) {
        return VK_NULL_HANDLE;
    }
    return pipeline;
}

static bool run_descriptor_benchmark(
    VkPhysicalDevice physical_device,
    VkDevice device,
    VkQueue queue,
    VkCommandBuffer command,
    VkFence fence,
    RenderTarget* output,
    VkPipelineLayout pipeline_layout,
    VkDescriptorSet descriptor_sets[2],
    VkSampler sampler,
    VkShaderModule vertex,
    VkShaderModule fragment,
    uint32_t draw_count) {
    const VkExtent2D extent = {1, 1};
    const float red_values[2] = {0.2f, 0.8f};
    RenderTarget sources[2] = {};
    VkDescriptorImageInfo image_infos[2] = {};
    VkWriteDescriptorSet writes[2] = {};

    for (uint32_t index = 0; index < 2; ++index) {
        if (!create_target(
                physical_device, device, extent,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true,
                &sources[index])) {
            return false;
        }
        image_infos[index] = {
            .sampler = sampler,
            .imageView = sources[index].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writes[index] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets[index],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_infos[index],
        };
    }
    vkUpdateDescriptorSets(device, 2, writes, 0, NULL);

    VkPipeline pipeline = create_pipeline(
        device, pipeline_layout, output->render_pass,
        vertex, fragment, extent);
    if (!pipeline) {
        return false;
    }

    const VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (!vk_ok(
            vkBeginCommandBuffer(command, &begin),
            "vkBeginCommandBuffer(benchmark)")) {
        return false;
    }
    for (uint32_t index = 0; index < 2; ++index) {
        const VkClearValue source_clear = {
            .color = {{red_values[index], 0.25f, 0.75f, 1.0f}},
        };
        const VkRenderPassBeginInfo source_pass = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = sources[index].render_pass,
            .framebuffer = sources[index].framebuffer,
            .renderArea = {{0, 0}, extent},
            .clearValueCount = 1,
            .pClearValues = &source_clear,
        };
        vkCmdBeginRenderPass(
            command, &source_pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(command);
    }
    const VkClearValue output_clear = {
        .color = {{0, 0, 0, 1}},
    };
    const VkRenderPassBeginInfo output_pass = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = output->render_pass,
        .framebuffer = output->framebuffer,
        .renderArea = {{0, 0}, extent},
        .clearValueCount = 1,
        .pClearValues = &output_clear,
    };
    vkCmdBeginRenderPass(
        command, &output_pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    for (uint32_t draw = 0; draw < draw_count; ++draw) {
        const VkDescriptorSet descriptor_set =
            descriptor_sets[draw & 1u];
        vkCmdBindDescriptorSets(
            command, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
        vkCmdDraw(command, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(command);
    if (!vk_ok(
            vkEndCommandBuffer(command),
            "vkEndCommandBuffer(benchmark)")) {
        return false;
    }

    const VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
    };
    if (!vk_ok(
            vkQueueSubmit(queue, 1, &submit, fence),
            "vkQueueSubmit(benchmark warmup)") ||
        !vk_ok(
            vkWaitForFences(
                device, 1, &fence, VK_TRUE, UINT64_MAX),
            "vkWaitForFences(benchmark warmup)") ||
        !vk_ok(
            vkResetFences(device, 1, &fence),
            "vkResetFences(benchmark warmup)")) {
        return false;
    }

    for (uint32_t sample = 0;
         sample < kBenchmarkSampleCount;
         ++sample) {
        const uint64_t submit_start = monotonic_nanoseconds();
        if (!submit_start ||
            !vk_ok(
                vkQueueSubmit(queue, 1, &submit, fence),
                "vkQueueSubmit(benchmark)")) {
            return false;
        }
        const uint64_t submit_end = monotonic_nanoseconds();
        if (!vk_ok(
                vkWaitForFences(
                    device, 1, &fence, VK_TRUE, UINT64_MAX),
                "vkWaitForFences(benchmark)")) {
            return false;
        }
        const uint64_t wait_end = monotonic_nanoseconds();
        if (!submit_end || !wait_end ||
            !vk_ok(
                vkResetFences(device, 1, &fence),
                "vkResetFences(benchmark)")) {
            return false;
        }
        printf(
            "descriptor benchmark sample=%u draws=%u "
            "submit_ns=%llu wait_ns=%llu\n",
            sample, draw_count,
            (unsigned long long)(submit_end - submit_start),
            (unsigned long long)(wait_end - submit_end));
    }

    id<MTLTexture> output_texture = nil;
    vkGetMTLTextureMVK(output->image, &output_texture);
    uint8_t pixel[4] = {};
    const float expected_red =
        red_values[(draw_count - 1u) & 1u];
    const bool matches =
        read_first_pixel(output_texture, pixel) &&
        pixel_matches(pixel, expected_red);
    printf(
        "descriptor benchmark pixel=%u,%u,%u,%u "
        "expected-red=%u match=%s\n",
        pixel[0], pixel[1], pixel[2], pixel[3],
        to_unorm8(expected_red), matches ? "yes" : "NO");
    if (!matches ||
        !vk_ok(vkDeviceWaitIdle(device), "vkDeviceWaitIdle(benchmark)")) {
        return false;
    }

    vkDestroyPipeline(device, pipeline, NULL);
    for (uint32_t index = 0; index < 2; ++index) {
        destroy_target(device, &sources[index]);
    }
    printf(
        "descriptor benchmark: PASS draws=%u samples=%u "
        "alternating_resources=yes\n",
        draw_count, kBenchmarkSampleCount);
    return true;
}

int main(int argc, char** argv) {
    @autoreleasepool {
        if (argc != 3) {
            fprintf(
                stderr, "usage: %s vertex.spv fragment.spv\n", argv[0]);
            return 2;
        }
        uint32_t benchmark_draws = 0;
        const char* benchmark_value =
            getenv("TESO4M4_DESCRIPTOR_BENCHMARK_DRAWS");
        if (benchmark_value) {
            char* end = NULL;
            const unsigned long parsed =
                strtoul(benchmark_value, &end, 10);
            if (!benchmark_value[0] || !end || *end ||
                parsed < 2 || parsed > 100000) {
                fprintf(
                    stderr,
                    "TESO4M4_DESCRIPTOR_BENCHMARK_DRAWS must be 2..100000\n");
                return 2;
            }
            benchmark_draws = (uint32_t)parsed;
        }
        const bool benchmark_mode = benchmark_draws != 0;
        const VkExtent2D output_extent =
            benchmark_mode ? VkExtent2D{1, 1} : kOutputExtent;
        const VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "teso4m4 reset composite probe",
            .apiVersion = VK_API_VERSION_1_0,
        };
        const VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &application_info,
        };
        VkInstance instance = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateInstance(&instance_info, NULL, &instance),
                "vkCreateInstance")) {
            return 1;
        }
        uint32_t physical_count = 0;
        if (!vk_ok(
                vkEnumeratePhysicalDevices(
                    instance, &physical_count, NULL),
                "vkEnumeratePhysicalDevices(count)") ||
            physical_count == 0) {
            return 1;
        }
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        physical_count = 1;
        if (!vk_ok(
                vkEnumeratePhysicalDevices(
                    instance, &physical_count, &physical_device),
                "vkEnumeratePhysicalDevices(data)")) {
            return 1;
        }
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &family_count, NULL);
        VkQueueFamilyProperties* families =
            (VkQueueFamilyProperties*)calloc(
                family_count, sizeof(*families));
        if (!families) {
            return 1;
        }
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &family_count, families);
        uint32_t family_index = UINT32_MAX;
        for (uint32_t index = 0; index < family_count; ++index) {
            if (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                family_index = index;
                break;
            }
        }
        free(families);
        if (family_index == UINT32_MAX) {
            fprintf(stderr, "no graphics queue family\n");
            return 1;
        }

        const float priority = 1;
        const VkDeviceQueueCreateInfo queue_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };
        const VkDeviceCreateInfo device_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
        };
        VkDevice device = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateDevice(
                    physical_device, &device_info, NULL, &device),
                "vkCreateDevice")) {
            return 1;
        }
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, family_index, 0, &queue);

        RenderTarget output = {};
        if (!create_target(
                physical_device, device, output_extent,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, false, &output)) {
            return 1;
        }

        const VkDescriptorSetLayoutBinding binding = {
            .binding = 0,
            .descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        const VkDescriptorSetLayoutCreateInfo set_layout_info = {
            .sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &binding,
        };
        VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateDescriptorSetLayout(
                    device, &set_layout_info, NULL, &set_layout),
                "vkCreateDescriptorSetLayout")) {
            return 1;
        }
        const VkPipelineLayoutCreateInfo pipeline_layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &set_layout,
        };
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreatePipelineLayout(
                    device, &pipeline_layout_info, NULL, &pipeline_layout),
                "vkCreatePipelineLayout")) {
            return 1;
        }
        const VkDescriptorPoolSize pool_size = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = benchmark_mode ? 2u : 1u,
        };
        const VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = benchmark_mode ? 2u : 1u,
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
        };
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateDescriptorPool(
                    device, &pool_info, NULL, &descriptor_pool),
                "vkCreateDescriptorPool")) {
            return 1;
        }
        const VkDescriptorSetLayout set_layouts[2] = {
            set_layout,
            set_layout,
        };
        const uint32_t descriptor_set_count =
            benchmark_mode ? 2u : 1u;
        const VkDescriptorSetAllocateInfo set_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = descriptor_set_count,
            .pSetLayouts = set_layouts,
        };
        VkDescriptorSet descriptor_sets[2] = {
            VK_NULL_HANDLE,
            VK_NULL_HANDLE,
        };
        if (!vk_ok(
                vkAllocateDescriptorSets(
                    device, &set_info, descriptor_sets),
                "vkAllocateDescriptorSets")) {
            return 1;
        }
        const VkSamplerCreateInfo sampler_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .maxLod = 1,
        };
        VkSampler sampler = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateSampler(
                    device, &sampler_info, NULL, &sampler),
                "vkCreateSampler")) {
            return 1;
        }

        VkShaderModule vertex =
            create_spirv_module(device, argv[1]);
        VkShaderModule fragment =
            create_spirv_module(device, argv[2]);
        if (!vertex || !fragment) {
            return 1;
        }

        const VkCommandPoolCreateInfo command_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = family_index,
        };
        VkCommandPool command_pool = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateCommandPool(
                    device, &command_pool_info, NULL, &command_pool),
                "vkCreateCommandPool")) {
            return 1;
        }
        const VkCommandBufferAllocateInfo command_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!vk_ok(
                vkAllocateCommandBuffers(
                    device, &command_info, &command),
                "vkAllocateCommandBuffers")) {
            return 1;
        }
        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };
        VkFence fence = VK_NULL_HANDLE;
        if (!vk_ok(
                vkCreateFence(device, &fence_info, NULL, &fence),
                "vkCreateFence")) {
            return 1;
        }

        uint32_t passed_cycles = 0;
        if (benchmark_mode) {
            if (!run_descriptor_benchmark(
                    physical_device, device, queue, command, fence,
                    &output, pipeline_layout, descriptor_sets, sampler,
                    vertex, fragment, benchmark_draws)) {
                return 1;
            }
        } else {
            for (uint32_t cycle = 0; cycle < kCycleCount; ++cycle) {
            const VkExtent2D source_extent =
                (cycle & 1u) ? VkExtent2D{1920, 1200}
                             : VkExtent2D{2048, 1280};
            const float red =
                0.08f + 0.84f * (float)cycle /
                    (float)(kCycleCount - 1);
            RenderTarget source = {};
            if (!create_target(
                    physical_device, device, source_extent,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true,
                    &source)) {
                return 1;
            }

            const VkDescriptorImageInfo image_info = {
                .sampler = sampler,
                .imageView = source.view,
                .imageLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            const VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[0],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info,
            };
            vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

            VkPipeline pipeline = create_pipeline(
                device, pipeline_layout, output.render_pass,
                vertex, fragment, output_extent);
            if (!pipeline) {
                return 1;
            }
            if (cycle & 1u) {
                if (!vk_ok(
                        vkResetCommandPool(device, command_pool, 0),
                        "vkResetCommandPool")) {
                    return 1;
                }
            } else if (!vk_ok(
                           vkResetCommandBuffer(command, 0),
                           "vkResetCommandBuffer")) {
                return 1;
            }
            const VkCommandBufferBeginInfo begin = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            if (!vk_ok(
                    vkBeginCommandBuffer(command, &begin),
                    "vkBeginCommandBuffer")) {
                return 1;
            }
            const VkClearValue source_clear = {
                .color = {{red, 0.25f, 0.75f, 1.0f}},
            };
            const VkRenderPassBeginInfo source_pass = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = source.render_pass,
                .framebuffer = source.framebuffer,
                .renderArea = {{0, 0}, source_extent},
                .clearValueCount = 1,
                .pClearValues = &source_clear,
            };
            vkCmdBeginRenderPass(
                command, &source_pass, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(command);

            const VkClearValue output_clear = {
                .color = {{0, 0, 0, 1}},
            };
            const VkRenderPassBeginInfo output_pass = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = output.render_pass,
                .framebuffer = output.framebuffer,
                .renderArea = {{0, 0}, output_extent},
                .clearValueCount = 1,
                .pClearValues = &output_clear,
            };
            vkCmdBeginRenderPass(
                command, &output_pass, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(
                command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(
                command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout, 0, 1, &descriptor_sets[0], 0, NULL);
            vkCmdDraw(command, 3, 1, 0, 0);
            vkCmdEndRenderPass(command);
            if (!vk_ok(
                    vkEndCommandBuffer(command),
                    "vkEndCommandBuffer")) {
                return 1;
            }
            const VkSubmitInfo submit = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &command,
            };
            if (!vk_ok(
                    vkQueueSubmit(queue, 1, &submit, fence),
                    "vkQueueSubmit") ||
                !vk_ok(
                    vkWaitForFences(
                        device, 1, &fence, VK_TRUE, UINT64_MAX),
                    "vkWaitForFences")) {
                return 1;
            }

            id<MTLTexture> output_texture = nil;
            vkGetMTLTextureMVK(output.image, &output_texture);
            uint8_t pixel[4] = {};
            if (!read_first_pixel(output_texture, pixel)) {
                return 1;
            }
            const bool matches = pixel_matches(pixel, red);
            printf(
                "cycle=%u source=%ux%u reset=%s "
                "pixel=%u,%u,%u,%u expected-red=%u match=%s\n",
                cycle, source_extent.width, source_extent.height,
                (cycle & 1u) ? "pool" : "command",
                pixel[0], pixel[1], pixel[2], pixel[3],
                to_unorm8(red), matches ? "yes" : "NO");
            if (!matches) {
                return 3;
            }
            ++passed_cycles;

            if (!vk_ok(
                    vkResetFences(device, 1, &fence),
                    "vkResetFences") ||
                !vk_ok(vkDeviceWaitIdle(device), "vkDeviceWaitIdle")) {
                return 1;
            }
            vkDestroyPipeline(device, pipeline, NULL);
            destroy_target(device, &source);
            }
        }

        vkDeviceWaitIdle(device);
        vkDestroyFence(device, fence, NULL);
        vkDestroyCommandPool(device, command_pool, NULL);
        vkDestroyShaderModule(device, fragment, NULL);
        vkDestroyShaderModule(device, vertex, NULL);
        vkDestroySampler(device, sampler, NULL);
        vkDestroyDescriptorPool(device, descriptor_pool, NULL);
        vkDestroyPipelineLayout(device, pipeline_layout, NULL);
        vkDestroyDescriptorSetLayout(device, set_layout, NULL);
        destroy_target(device, &output);
        vkDestroyDevice(device, NULL);
        vkDestroyInstance(instance, NULL);

        if (benchmark_mode) {
            printf(
                "descriptor encode probe: PASS draws=%u samples=%u\n",
                benchmark_draws, kBenchmarkSampleCount);
        } else {
            printf(
                "reset composite probe: PASS cycles=%u "
                "descriptor_set=full-lifetime command_reuse=yes\n",
                passed_cycles);
        }
        return 0;
    }
}
