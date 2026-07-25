#pragma once

#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#define TESO4M4_RENDER_AUDIT_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_RENDER_AUDIT_HIDDEN
#endif

typedef void (*Teso4m4RenderAuditLogFunction)(const char* message);

TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_reset(void);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_set_logger(
    Teso4m4RenderAuditLogFunction logger);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_enable_mirror(void);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_begin(void);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_finish(
    const char* reason);

TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_image(
    const VkImageCreateInfo* info, VkImage image);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_image(
    VkImage image);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_image_requirements(
    VkImage image, const VkMemoryRequirements* requirements);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_bind_image(
    VkImage image, VkDeviceMemory memory, VkDeviceSize offset);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_image_view(
    const VkImageViewCreateInfo* info, VkImageView view);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_image_view(
    VkImageView view);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_buffer(
    VkBuffer buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_buffer(
    VkBuffer buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_buffer_view(
    VkBufferView buffer_view);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_buffer_view(
    VkBufferView buffer_view);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_sampler(
    VkSampler sampler);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_sampler(
    VkSampler sampler);

TESO4M4_RENDER_AUDIT_HIDDEN void
teso4m4_render_audit_create_descriptor_set_layout(
    const VkDescriptorSetLayoutCreateInfo* info,
    VkDescriptorSetLayout layout);
TESO4M4_RENDER_AUDIT_HIDDEN void
teso4m4_render_audit_destroy_descriptor_set_layout(
    VkDescriptorSetLayout layout);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_render_pass(
    const VkRenderPassCreateInfo* info, VkRenderPass render_pass);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_render_pass(
    VkRenderPass render_pass);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_framebuffer(
    const VkFramebufferCreateInfo* info, VkFramebuffer framebuffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_framebuffer(
    VkFramebuffer framebuffer);

TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_allocate_descriptor_sets(
    const VkDescriptorSetAllocateInfo* info, const VkDescriptorSet* sets);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_free_descriptor_sets(
    VkDescriptorPool pool, uint32_t count, const VkDescriptorSet* sets);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_reset_descriptor_pool(
    VkDescriptorPool pool);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_descriptor_pool(
    VkDescriptorPool pool);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_update_descriptor_sets(
    uint32_t write_count, const VkWriteDescriptorSet* writes,
    uint32_t copy_count, const VkCopyDescriptorSet* copies);

TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_graphics_pipelines(
    VkPipelineCache cache, uint32_t count,
    const VkGraphicsPipelineCreateInfo* infos, const VkPipeline* pipelines);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_pipeline(
    VkPipeline pipeline);

TESO4M4_RENDER_AUDIT_HIDDEN void
teso4m4_render_audit_allocate_command_buffers(
    const VkCommandBufferAllocateInfo* info,
    const VkCommandBuffer* command_buffers);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_free_command_buffers(
    VkCommandPool pool, uint32_t count,
    const VkCommandBuffer* command_buffers);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_reset_command_buffer(
    VkCommandBuffer command_buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_reset_command_pool(
    VkCommandPool command_pool);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_begin_command_buffer(
    VkCommandBuffer command_buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_end_command_buffer(
    VkCommandBuffer command_buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_queue_submit(
    VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
    VkFence fence);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_acquire_next_image(
    VkSemaphore semaphore, VkFence fence, VkResult result);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_queue_present(
    VkQueue queue, const VkPresentInfoKHR* present_info, VkResult result);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_semaphore(
    VkSemaphore semaphore);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_semaphore(
    VkSemaphore semaphore);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_create_fence(
    VkFence fence, VkFenceCreateFlags flags);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_destroy_fence(
    VkFence fence);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_reset_fences(
    uint32_t fence_count, const VkFence* fences);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_wait_for_fences(
    uint32_t fence_count, const VkFence* fences, VkResult result);

TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_begin_render_pass(
    VkCommandBuffer command_buffer, const VkRenderPassBeginInfo* info);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_end_render_pass(
    VkCommandBuffer command_buffer);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_bind_pipeline(
    VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
    VkPipeline pipeline);
TESO4M4_RENDER_AUDIT_HIDDEN void
teso4m4_render_audit_cmd_bind_descriptor_sets(
    VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
    uint32_t first_set, uint32_t set_count, const VkDescriptorSet* sets);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_pipeline_barrier(
    VkCommandBuffer command_buffer, VkPipelineStageFlags source_stage_mask,
    VkPipelineStageFlags destination_stage_mask,
    uint32_t image_barrier_count,
    const VkImageMemoryBarrier* image_barriers);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_copy_image(void);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_blit_image(void);
TESO4M4_RENDER_AUDIT_HIDDEN void teso4m4_render_audit_cmd_resolve_image(void);
