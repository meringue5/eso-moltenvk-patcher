#include "mvk_render_audit.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define HANDLE(type, value) ((type)(uintptr_t)(value))

static char g_log[65536];
static size_t g_log_length;

static void test_log(const char* message) {
    const int written = snprintf(
        g_log + g_log_length, sizeof(g_log) - g_log_length, "%s\n", message);
    if (written > 0 && (size_t)written < sizeof(g_log) - g_log_length) {
        g_log_length += (size_t)written;
    }
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Render audit probe failed: %s\n%s", message, g_log);
    }
    return condition;
}

static uint64_t monotonic_nanoseconds(void) {
    struct timespec value = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0;
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
           (uint64_t)value.tv_nsec;
}

int main(void) {
    const VkImage first_image = HANDLE(VkImage, 0x100);
    const VkImage second_image = HANDLE(VkImage, 0x101);
    const VkImageView view = HANDLE(VkImageView, 0x200);
    const VkDescriptorPool pool = HANDLE(VkDescriptorPool, 0x300);
    const VkDescriptorSetLayout set_layout =
        HANDLE(VkDescriptorSetLayout, 0x301);
    const VkDescriptorSet set = HANDLE(VkDescriptorSet, 0x302);
    const VkRenderPass render_pass = HANDLE(VkRenderPass, 0x400);
    const VkFramebuffer framebuffer = HANDLE(VkFramebuffer, 0x401);
    const VkPipeline pipeline = HANDLE(VkPipeline, 0x500);
    const VkCommandBuffer command_buffer =
        HANDLE(VkCommandBuffer, 0x600);
    const VkDeviceMemory memory = HANDLE(VkDeviceMemory, 0x700);
    const VkBuffer buffer = HANDLE(VkBuffer, 0x701);
    const VkSampler sampler = HANDLE(VkSampler, 0x702);

    teso4m4_render_audit_reset();
    teso4m4_render_audit_set_logger(&test_log);
    teso4m4_render_audit_enable_mirror();

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {64, 64, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkMemoryRequirements requirements = {
        .size = 4096,
        .alignment = 256,
        .memoryTypeBits = 1,
    };
    teso4m4_render_audit_create_image(&image_info, first_image);
    teso4m4_render_audit_create_image(&image_info, second_image);
    teso4m4_render_audit_image_requirements(first_image, &requirements);
    teso4m4_render_audit_image_requirements(second_image, &requirements);
    teso4m4_render_audit_bind_image(first_image, memory, 0);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = first_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    teso4m4_render_audit_create_image_view(&view_info, view);
    teso4m4_render_audit_create_buffer(buffer);
    teso4m4_render_audit_create_sampler(sampler);

    const VkDescriptorSetLayoutBinding layout_bindings[] = {
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = &sampler,
        },
    };
    const VkDescriptorSetLayoutCreateInfo set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount =
            sizeof(layout_bindings) / sizeof(layout_bindings[0]),
        .pBindings = layout_bindings,
    };
    teso4m4_render_audit_create_descriptor_set_layout(
        &set_layout_info, set_layout);
    VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &set_layout,
    };
    teso4m4_render_audit_allocate_descriptor_sets(&allocate_info, &set);
    VkDescriptorImageInfo descriptor_image = {
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    const VkDescriptorBufferInfo descriptor_buffer = {
        .buffer = buffer,
        .offset = 16,
        .range = 64,
    };
    VkWriteDescriptorSet writes[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptor_image,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = 3,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &descriptor_buffer,
        },
    };
    teso4m4_render_audit_update_descriptor_sets(
        sizeof(writes) / sizeof(writes[0]), writes, 0, NULL);
    enum { kUpdateBenchmarkIterations = 100000 };
    const uint64_t update_start = monotonic_nanoseconds();
    for (uint32_t index = 0; index < kUpdateBenchmarkIterations; ++index) {
        teso4m4_render_audit_update_descriptor_sets(
            1, &writes[0], 0, NULL);
    }
    const uint64_t update_end = monotonic_nanoseconds();
    const uint64_t update_nanoseconds =
        update_start != 0 && update_end > update_start
            ? (update_end - update_start) / kUpdateBenchmarkIterations
            : UINT64_MAX;
    if (!check(
            update_nanoseconds < 10000,
            "mirrored descriptor update must remain below 10 us")) {
        return 1;
    }

    VkAttachmentDescription attachment = {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
    };
    teso4m4_render_audit_create_render_pass(
        &render_pass_info, render_pass);
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &view,
        .width = 64,
        .height = 64,
        .layers = 1,
    };
    teso4m4_render_audit_create_framebuffer(
        &framebuffer_info, framebuffer);

    teso4m4_render_audit_begin();
    const VkCommandBufferAllocateInfo command_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = HANDLE(VkCommandPool, 0x601),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    teso4m4_render_audit_allocate_command_buffers(
        &command_allocate_info, &command_buffer);
    teso4m4_render_audit_begin_command_buffer(command_buffer);
    teso4m4_render_audit_bind_image(second_image, memory, 2048);
    teso4m4_render_audit_destroy_image_view(view);
    teso4m4_render_audit_destroy_buffer(buffer);
    teso4m4_render_audit_destroy_image(first_image);
    teso4m4_render_audit_bind_image(second_image, memory, 2048);
    teso4m4_render_audit_cmd_bind_descriptor_sets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1, &set);

    VkRenderPassBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {{0, 0}, {64, 64}},
    };
    teso4m4_render_audit_cmd_begin_render_pass(
        command_buffer, &begin_info);
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .renderPass = render_pass,
    };
    teso4m4_render_audit_create_graphics_pipelines(
        HANDLE(VkPipelineCache, 0x501), 1, &pipeline_info, &pipeline);
    teso4m4_render_audit_cmd_bind_pipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    teso4m4_render_audit_cmd_end_render_pass(command_buffer);

    const VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = first_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    teso4m4_render_audit_cmd_pipeline_barrier(
        command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1, &barrier);
    teso4m4_render_audit_end_command_buffer(command_buffer);
    const VkSemaphore wait_semaphore =
        HANDLE(VkSemaphore, 0x800);
    const VkSemaphore signal_semaphore =
        HANDLE(VkSemaphore, 0x801);
    const VkFence fence = HANDLE(VkFence, 0x802);
    teso4m4_render_audit_create_semaphore(wait_semaphore);
    teso4m4_render_audit_create_semaphore(signal_semaphore);
    teso4m4_render_audit_create_fence(fence, 0);
    teso4m4_render_audit_acquire_next_image(
        wait_semaphore, VK_NULL_HANDLE, VK_SUCCESS);
    const VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &wait_semaphore,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signal_semaphore,
    };
    teso4m4_render_audit_queue_submit(
        HANDLE(VkQueue, 0x803), 1, &submit, fence);
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signal_semaphore,
    };
    teso4m4_render_audit_queue_present(
        HANDLE(VkQueue, 0x803), &present_info, VK_SUCCESS);
    teso4m4_render_audit_reset_command_buffer(command_buffer);
    teso4m4_render_audit_begin_command_buffer(command_buffer);
    teso4m4_render_audit_end_command_buffer(command_buffer);
    const VkSubmitInfo generation_submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    teso4m4_render_audit_queue_submit(
        HANDLE(VkQueue, 0x803), 1, &generation_submit,
        VK_NULL_HANDLE);
    teso4m4_render_audit_reset_command_pool(
        command_allocate_info.commandPool);
    teso4m4_render_audit_begin_command_buffer(command_buffer);
    teso4m4_render_audit_end_command_buffer(command_buffer);
    teso4m4_render_audit_queue_submit(
        HANDLE(VkQueue, 0x803), 1, &generation_submit,
        VK_NULL_HANDLE);
    teso4m4_render_audit_cmd_copy_image();
    teso4m4_render_audit_cmd_blit_image();
    teso4m4_render_audit_cmd_resolve_image();
    teso4m4_render_audit_finish("probe");

    if (!check(strstr(g_log, "RENDER_AUDIT_BEGIN: mirror=enabled") != NULL,
               "mirror must be enabled") ||
        !check(strstr(g_log, "type=stale_descriptor_set_bound") != NULL,
               "stale descriptor binding must be identified") ||
        !check(strstr(g_log, "type=live_image_memory_overlap") != NULL,
               "live overlap must be identified") ||
        !check(strstr(g_log, "name=image_dead_range_reuses value=1") != NULL,
               "reuse of a destroyed image range must be counted") ||
        !check(strstr(g_log, "name=layout_mismatches value=2") != NULL,
               "render-pass and barrier layout mismatches must be counted") ||
        !check(strstr(g_log, "name=pipeline_render_pass_exact value=1") != NULL,
               "pipeline/render-pass linkage must be exact") ||
        !check(strstr(g_log, "name=descriptor_known_slots_bound value=3") !=
                   NULL,
               "bound descriptor slot content must be known") ||
        !check(
            strstr(
                g_log,
                "name=descriptor_resources_destroyed_while_referenced value=1")
                != NULL,
            "destroyed descriptor buffer must be tracked") ||
        !check(strstr(g_log, "name=command_buffer_submits value=3") != NULL &&
                   strstr(
                       g_log,
                       "name=command_buffer_submit_invalid_generation value=0")
                       != NULL &&
                   strstr(g_log, "name=command_buffer_resets value=1") !=
                       NULL &&
                   strstr(g_log, "name=command_pool_resets value=1") != NULL,
               "command generations must remain valid across both reset paths") ||
        !check(strstr(g_log, "name=submit_wait_semaphores value=1") != NULL &&
                   strstr(g_log, "name=unknown_semaphore_waits value=0") !=
                       NULL,
               "acquire-submit-present semaphore order must be known") ||
        !check(strstr(g_log, "name=subresource_barriers value=1") != NULL,
               "barrier must expand to one tracked subresource") ||
        !check(strstr(g_log, "name=copy_image_calls value=1") != NULL &&
                   strstr(g_log, "name=blit_image_calls value=1") != NULL &&
                   strstr(g_log, "name=resolve_image_calls value=1") != NULL,
               "image transfer calls must be counted") ||
        !check(strstr(g_log, "RENDER_AUDIT_SUMMARY: reason=probe complete=yes") !=
                   NULL,
               "audit summary must complete")) {
        return 1;
    }
    printf(
        "Render graph audit smoke: PASS descriptor_update_ns=%" PRIu64 "\n",
        update_nanoseconds);
    return 0;
}
