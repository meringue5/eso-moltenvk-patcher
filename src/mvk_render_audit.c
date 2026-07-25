#include "mvk_render_audit.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    kMaxImages = 8192,
    kMaxViews = 16384,
    kMaxBuffers = 16384,
    kMaxBufferViews = 8192,
    kMaxSamplers = 4096,
    kMaxDescriptorSets = 16384,
    kMaxDescriptorSlots = 131072,
    kMaxDescriptorSetLayouts = 2048,
    kMaxLayoutBindings = 32,
    kMaxImmutableSamplersPerLayout = 256,
    kMaxRenderPasses = 2048,
    kMaxFramebuffers = 4096,
    kMaxPipelines = 16384,
    kMaxCommandBuffers = 512,
    kMaxSubresources = 131072,
    kMaxSyncObjects = 4096,
    kMaxQueues = 32,
    kMaxBoundSetsPerCommand = 32,
    kMaxTransitionsPerCommand = 512,
    kMaxAttachments = 12,
    kSampleLimit = 64,
    kPassTailLimit = 128,
    kDescriptorSlotSampleLimit = 256,
    kCommandSampleLimit = 64,
    kBarrierSampleLimit = 64,
    kAnomalyLimit = 64,
};

typedef enum {
    kDescriptorUpdateCalls,
    kDescriptorImageWrites,
    kDescriptorCopies,
    kDescriptorMultiWrites,
    kDescriptorUnknownViewsWritten,
    kDescriptorDeadViewsWritten,
    kDescriptorViewsDestroyedWhileReferenced,
    kDescriptorResourcesDestroyedWhileReferenced,
    kDescriptorDeadResourcesWritten,
    kDescriptorUnknownResourcesWritten,
    kDescriptorSetBindCalls,
    kDescriptorSetsBound,
    kDescriptorUnknownSetsBound,
    kDescriptorKnownSlotsBound,
    kDescriptorUnknownSlotsBound,
    kDescriptorUnknownLayoutSetsBound,
    kDescriptorZeroUpdateSetsBound,
    kDescriptorStaleSetsBound,
    kDescriptorStaleSlotsBound,
    kImageBinds,
    kImageDeadRangeReuses,
    kImageLiveOverlaps,
    kImageUndeclaredOverlaps,
    kPipelineBarrierCalls,
    kImageBarriers,
    kSubresourceBarriers,
    kBarrierStageAccessSamples,
    kLayoutMismatches,
    kRenderPassBegins,
    kAttachmentSamples,
    kAttachmentLoadClear,
    kAttachmentLoadLoad,
    kAttachmentLoadDontCare,
    kAttachmentStoreStore,
    kAttachmentStoreDontCare,
    kAttachmentMissingClearValues,
    kUnknownAttachmentViews,
    kDeadAttachmentViews,
    kPipelineBinds,
    kPipelineBindsKnown,
    kPipelineBindsCreatedDuringAudit,
    kPipelineRenderPassExact,
    kPipelineRenderPassDifferent,
    kGraphicsPipelinesCreated,
    kGraphicsPipelinesCreatedDuringAudit,
    kGraphicsPipelinesWithCache,
    kCommandBufferResets,
    kCommandPoolResets,
    kCommandBufferBegins,
    kCommandBufferEnds,
    kCommandBufferSubmits,
    kCommandBufferSubmitInvalidGeneration,
    kDescriptorUpdatesAfterRecord,
    kQueueSubmits,
    kSubmitWaitSemaphores,
    kSubmitSignalSemaphores,
    kUnknownSemaphoreWaits,
    kFenceSubmits,
    kFenceResets,
    kFenceWaits,
    kFenceWaitsWithoutSubmit,
    kCopyImageCalls,
    kBlitImageCalls,
    kResolveImageCalls,
    kStateOverflows,
    kAuditCounterCount,
} AuditCounter;

static const char* const kCounterNames[kAuditCounterCount] = {
    [kDescriptorUpdateCalls] = "descriptor_update_calls",
    [kDescriptorImageWrites] = "descriptor_image_writes",
    [kDescriptorCopies] = "descriptor_copies",
    [kDescriptorMultiWrites] = "descriptor_multi_writes",
    [kDescriptorUnknownViewsWritten] = "descriptor_unknown_views_written",
    [kDescriptorDeadViewsWritten] = "descriptor_dead_views_written",
    [kDescriptorViewsDestroyedWhileReferenced] =
        "descriptor_views_destroyed_while_referenced",
    [kDescriptorResourcesDestroyedWhileReferenced] =
        "descriptor_resources_destroyed_while_referenced",
    [kDescriptorDeadResourcesWritten] =
        "descriptor_dead_resources_written",
    [kDescriptorUnknownResourcesWritten] =
        "descriptor_unknown_resources_written",
    [kDescriptorSetBindCalls] = "descriptor_set_bind_calls",
    [kDescriptorSetsBound] = "descriptor_sets_bound",
    [kDescriptorUnknownSetsBound] = "descriptor_unknown_sets_bound",
    [kDescriptorKnownSlotsBound] = "descriptor_known_slots_bound",
    [kDescriptorUnknownSlotsBound] = "descriptor_unknown_slots_bound",
    [kDescriptorUnknownLayoutSetsBound] =
        "descriptor_unknown_layout_sets_bound",
    [kDescriptorZeroUpdateSetsBound] =
        "descriptor_zero_update_sets_bound",
    [kDescriptorStaleSetsBound] = "descriptor_stale_sets_bound",
    [kDescriptorStaleSlotsBound] = "descriptor_stale_slots_bound",
    [kImageBinds] = "image_binds",
    [kImageDeadRangeReuses] = "image_dead_range_reuses",
    [kImageLiveOverlaps] = "image_live_overlaps",
    [kImageUndeclaredOverlaps] = "image_undeclared_overlaps",
    [kPipelineBarrierCalls] = "pipeline_barrier_calls",
    [kImageBarriers] = "image_barriers",
    [kSubresourceBarriers] = "subresource_barriers",
    [kBarrierStageAccessSamples] = "barrier_stage_access_samples",
    [kLayoutMismatches] = "layout_mismatches",
    [kRenderPassBegins] = "render_pass_begins",
    [kAttachmentSamples] = "attachment_samples",
    [kAttachmentLoadClear] = "attachment_load_clear",
    [kAttachmentLoadLoad] = "attachment_load_load",
    [kAttachmentLoadDontCare] = "attachment_load_dont_care",
    [kAttachmentStoreStore] = "attachment_store_store",
    [kAttachmentStoreDontCare] = "attachment_store_dont_care",
    [kAttachmentMissingClearValues] = "attachment_missing_clear_values",
    [kUnknownAttachmentViews] = "unknown_attachment_views",
    [kDeadAttachmentViews] = "dead_attachment_views",
    [kPipelineBinds] = "pipeline_binds",
    [kPipelineBindsKnown] = "pipeline_binds_known",
    [kPipelineBindsCreatedDuringAudit] =
        "pipeline_binds_created_during_audit",
    [kPipelineRenderPassExact] = "pipeline_render_pass_exact",
    [kPipelineRenderPassDifferent] = "pipeline_render_pass_different",
    [kGraphicsPipelinesCreated] = "graphics_pipelines_created",
    [kGraphicsPipelinesCreatedDuringAudit] =
        "graphics_pipelines_created_during_audit",
    [kGraphicsPipelinesWithCache] = "graphics_pipelines_with_cache",
    [kCommandBufferResets] = "command_buffer_resets",
    [kCommandPoolResets] = "command_pool_resets",
    [kCommandBufferBegins] = "command_buffer_begins",
    [kCommandBufferEnds] = "command_buffer_ends",
    [kCommandBufferSubmits] = "command_buffer_submits",
    [kCommandBufferSubmitInvalidGeneration] =
        "command_buffer_submit_invalid_generation",
    [kDescriptorUpdatesAfterRecord] = "descriptor_updates_after_record",
    [kQueueSubmits] = "queue_submits",
    [kSubmitWaitSemaphores] = "submit_wait_semaphores",
    [kSubmitSignalSemaphores] = "submit_signal_semaphores",
    [kUnknownSemaphoreWaits] = "unknown_semaphore_waits",
    [kFenceSubmits] = "fence_submits",
    [kFenceResets] = "fence_resets",
    [kFenceWaits] = "fence_waits",
    [kFenceWaitsWithoutSubmit] = "fence_waits_without_submit",
    [kCopyImageCalls] = "copy_image_calls",
    [kBlitImageCalls] = "blit_image_calls",
    [kResolveImageCalls] = "resolve_image_calls",
    [kStateOverflows] = "state_overflows",
};

typedef struct {
    bool occupied;
    bool live;
    VkImage handle;
    VkImageCreateFlags flags;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mip_levels;
    uint32_t array_layers;
    VkImageLayout layout;
    VkImageLayout initial_layout;
    bool layout_known;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    bool size_known;
    bool bound;
    uint64_t destroy_sequence;
} ImageState;

typedef struct {
    bool occupied;
    bool live;
    VkImageView handle;
    VkImage image;
    VkImageSubresourceRange range;
} ViewState;

typedef struct {
    bool occupied;
    bool live;
    uintptr_t handle;
} ResourceState;

typedef struct {
    uint32_t binding;
    VkDescriptorType type;
    uint32_t count;
    uint32_t immutable_sampler_offset;
    bool has_immutable_samplers;
} LayoutBindingState;

typedef struct {
    bool occupied;
    bool live;
    VkDescriptorSetLayout handle;
    uint32_t binding_count;
    uint32_t total_slots;
    uint32_t immutable_sampler_count;
    LayoutBindingState bindings[kMaxLayoutBindings];
    VkSampler immutable_samplers[kMaxImmutableSamplersPerLayout];
} DescriptorSetLayoutState;

typedef struct {
    bool occupied;
    bool live;
    VkDescriptorSet handle;
    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    uint32_t stale_slots;
    uint64_t last_update_sequence;
} DescriptorSetState;

typedef struct {
    bool occupied;
    bool valid;
    bool stale;
    bool known;
    VkDescriptorSet set;
    uint32_t binding;
    uint32_t element;
    VkDescriptorType type;
    VkImageView view;
    VkSampler sampler;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
    VkBufferView buffer_view;
    VkImageLayout image_layout;
} DescriptorSlotState;

typedef struct {
    bool occupied;
    bool live;
    VkRenderPass handle;
    uint32_t attachment_count;
    VkAttachmentDescription attachments[kMaxAttachments];
    uint32_t attachment_roles[kMaxAttachments];
    uint32_t attachment_first_subpass[kMaxAttachments];
    uint32_t attachment_last_subpass[kMaxAttachments];
    VkImageLayout attachment_use_layouts[kMaxAttachments];
    bool attachment_use_layout_known[kMaxAttachments];
} RenderPassState;

typedef struct {
    bool occupied;
    bool live;
    VkFramebuffer handle;
    VkRenderPass render_pass;
    uint32_t attachment_count;
    VkImageView attachments[kMaxAttachments];
} FramebufferState;

typedef struct {
    bool occupied;
    bool live;
    bool created_during_audit;
    VkPipeline handle;
    VkPipelineLayout layout;
    VkRenderPass render_pass;
    uint32_t subpass;
    VkPipelineCache cache;
} PipelineState;

typedef struct {
    VkImage image;
    VkImageSubresourceRange range;
    VkImageLayout old_layout;
    VkImageLayout new_layout;
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    VkAccessFlags source_access;
    VkAccessFlags destination_access;
    uint64_t record_sequence;
    bool explicit_barrier;
} TransitionState;

typedef struct {
    bool occupied;
    bool live;
    bool recording;
    bool executable;
    bool inside_render_pass;
    VkCommandBuffer handle;
    VkCommandPool pool;
    uint64_t generation;
    uint64_t begin_sequence;
    uint64_t end_sequence;
    uint64_t reset_sequence;
    uint64_t submit_sequence;
    uint64_t last_record_sequence;
    uint64_t max_bound_set_update_sequence;
    uint64_t max_submitted_set_update_sequence;
    uint32_t bound_set_count;
    VkDescriptorSet bound_sets[kMaxBoundSetsPerCommand];
    uint64_t bound_set_update_sequences[kMaxBoundSetsPerCommand];
    uint32_t transition_count;
    TransitionState transitions[kMaxTransitionsPerCommand];
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    VkPipeline last_pipeline;
    uint32_t last_first_set;
    uint32_t last_set_count;
    VkDescriptorSet last_sets[4];
    uint32_t descriptor_bind_calls;
    uint64_t last_descriptor_sequence;
} CommandBufferState;

typedef struct {
    bool occupied;
    VkImage image;
    VkImageAspectFlagBits aspect;
    uint32_t mip;
    uint32_t layer;
    VkImageLayout layout;
    bool layout_known;
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    VkAccessFlags source_access;
    VkAccessFlags destination_access;
    uint64_t sequence;
} SubresourceState;

typedef struct {
    bool occupied;
    bool live;
    bool signaled;
    uintptr_t handle;
    uint64_t signal_sequence;
    uint64_t wait_sequence;
    uint64_t submit_sequence;
} SyncState;

typedef struct {
    bool occupied;
    VkQueue handle;
    uint64_t submit_ordinal;
    uint64_t last_submit_sequence;
} QueueState;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static Teso4m4RenderAuditLogFunction g_logger;
static bool g_mirror_enabled;
static uint64_t g_mirror_start_sequence;
static bool g_active;
static bool g_complete;
static uint64_t g_sequence;
static uint64_t g_audit_start_sequence;
static uint64_t g_mirror_overflows;
static uint64_t g_counters[kAuditCounterCount];
static uint32_t g_samples;
static uint32_t g_pass_tail_samples;
static uint32_t g_descriptor_slot_samples;
static uint32_t g_command_samples;
static uint32_t g_barrier_samples;
static uint32_t g_anomalies;
static ImageState g_images[kMaxImages];
static ViewState g_views[kMaxViews];
static ResourceState g_buffers[kMaxBuffers];
static ResourceState g_buffer_views[kMaxBufferViews];
static ResourceState g_samplers[kMaxSamplers];
static DescriptorSetLayoutState
    g_set_layouts[kMaxDescriptorSetLayouts];
static DescriptorSetState g_sets[kMaxDescriptorSets];
static DescriptorSlotState g_slots[kMaxDescriptorSlots];
static RenderPassState g_render_passes[kMaxRenderPasses];
static FramebufferState g_framebuffers[kMaxFramebuffers];
static PipelineState g_pipelines[kMaxPipelines];
static CommandBufferState g_command_buffers[kMaxCommandBuffers];
static SubresourceState g_subresources[kMaxSubresources];
static SyncState g_semaphores[kMaxSyncObjects];
static SyncState g_fences[kMaxSyncObjects];
static QueueState g_queues[kMaxQueues];

static void audit_log(const char* format, ...) {
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

static uint64_t next_sequence(void) {
    return ++g_sequence;
}

static void count(AuditCounter counter, uint64_t value) {
    if (g_active) {
        g_counters[counter] += value;
    }
}

static void overflow(const char* table) {
    ++g_mirror_overflows;
    if (g_active) {
        count(kStateOverflows, 1);
    }
    if (g_active && g_anomalies++ < kAnomalyLimit) {
        audit_log(
            "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
            " type=state_overflow table=%s",
            next_sequence(), table);
    }
}

static ImageState* find_image(VkImage handle, bool create) {
    ImageState* empty = NULL;
    for (size_t index = 0; index < kMaxImages; ++index) {
        ImageState* state = &g_images[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && !empty) {
            empty = state;
        }
    }
    if (create && empty) {
        memset(empty, 0, sizeof(*empty));
        empty->occupied = true;
        empty->handle = handle;
        return empty;
    }
    if (create) {
        overflow("images");
    }
    return NULL;
}

static size_t handle_hash(uintptr_t handle, size_t capacity);

static ViewState* find_view(VkImageView handle, bool create) {
    const size_t start = handle_hash((uintptr_t)handle, kMaxViews);
    for (size_t probe = 0; probe < kMaxViews; ++probe) {
        ViewState* state =
            &g_views[(start + probe) & (kMaxViews - 1)];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow("views");
    }
    return NULL;
}

static size_t handle_hash(uintptr_t handle, size_t capacity) {
    uint64_t value = (uint64_t)(handle >> 4);
    value ^= value >> 33;
    value *= UINT64_C(0xff51afd7ed558ccd);
    value ^= value >> 33;
    return (size_t)value & (capacity - 1);
}

static ResourceState* find_resource(
    ResourceState* states, size_t capacity, uintptr_t handle,
    bool create, const char* table) {
    const size_t start = handle_hash(handle, capacity);
    for (size_t probe = 0; probe < capacity; ++probe) {
        ResourceState* state =
            &states[(start + probe) & (capacity - 1)];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow(table);
    }
    return NULL;
}

static DescriptorSetLayoutState* find_set_layout(
    VkDescriptorSetLayout handle, bool create) {
    const size_t start =
        handle_hash((uintptr_t)handle, kMaxDescriptorSetLayouts);
    for (size_t probe = 0; probe < kMaxDescriptorSetLayouts; ++probe) {
        DescriptorSetLayoutState* state =
            &g_set_layouts[
                (start + probe) & (kMaxDescriptorSetLayouts - 1)];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow("descriptor_set_layouts");
    }
    return NULL;
}

static DescriptorSetState* find_set(VkDescriptorSet handle, bool create) {
    size_t start = handle_hash((uintptr_t)handle, kMaxDescriptorSets);
    for (size_t probe = 0; probe < kMaxDescriptorSets; ++probe) {
        DescriptorSetState* state =
            &g_sets[(start + probe) & (kMaxDescriptorSets - 1)];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow("descriptor_sets");
    }
    return NULL;
}

static uint64_t slot_hash_value(
    VkDescriptorSet set, uint32_t binding, uint32_t element) {
    uint64_t value = (uint64_t)((uintptr_t)set >> 4);
    value ^= (uint64_t)binding * UINT64_C(0x9e3779b185ebca87);
    value ^= (uint64_t)element * UINT64_C(0xc2b2ae3d27d4eb4f);
    value ^= value >> 29;
    return value;
}

static DescriptorSlotState* find_slot(
    VkDescriptorSet set, uint32_t binding, uint32_t element, bool create) {
    size_t start =
        (size_t)slot_hash_value(set, binding, element) &
        (kMaxDescriptorSlots - 1);
    for (size_t probe = 0; probe < kMaxDescriptorSlots; ++probe) {
        DescriptorSlotState* state =
            &g_slots[(start + probe) & (kMaxDescriptorSlots - 1)];
        if (state->occupied && state->set == set &&
            state->binding == binding && state->element == element) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->set = set;
            state->binding = binding;
            state->element = element;
            return state;
        }
    }
    if (create) {
        overflow("descriptor_slots");
    }
    return NULL;
}

static RenderPassState* find_render_pass(
    VkRenderPass handle, bool create) {
    RenderPassState* empty = NULL;
    for (size_t index = 0; index < kMaxRenderPasses; ++index) {
        RenderPassState* state = &g_render_passes[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && !empty) {
            empty = state;
        }
    }
    if (create && empty) {
        memset(empty, 0, sizeof(*empty));
        empty->occupied = true;
        empty->handle = handle;
        return empty;
    }
    if (create) {
        overflow("render_passes");
    }
    return NULL;
}

static FramebufferState* find_framebuffer(
    VkFramebuffer handle, bool create) {
    FramebufferState* empty = NULL;
    for (size_t index = 0; index < kMaxFramebuffers; ++index) {
        FramebufferState* state = &g_framebuffers[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && !empty) {
            empty = state;
        }
    }
    if (create && empty) {
        memset(empty, 0, sizeof(*empty));
        empty->occupied = true;
        empty->handle = handle;
        return empty;
    }
    if (create) {
        overflow("framebuffers");
    }
    return NULL;
}

static PipelineState* find_pipeline(VkPipeline handle, bool create) {
    PipelineState* empty = NULL;
    for (size_t index = 0; index < kMaxPipelines; ++index) {
        PipelineState* state = &g_pipelines[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && !empty) {
            empty = state;
        }
    }
    if (create && empty) {
        memset(empty, 0, sizeof(*empty));
        empty->occupied = true;
        empty->handle = handle;
        return empty;
    }
    if (create) {
        overflow("pipelines");
    }
    return NULL;
}

static CommandBufferState* find_command_buffer(
    VkCommandBuffer handle, bool create) {
    CommandBufferState* empty = NULL;
    for (size_t index = 0; index < kMaxCommandBuffers; ++index) {
        CommandBufferState* state = &g_command_buffers[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && !empty) {
            empty = state;
        }
    }
    if (create && empty) {
        memset(empty, 0, sizeof(*empty));
        empty->occupied = true;
        empty->handle = handle;
        return empty;
    }
    if (create) {
        overflow("command_buffers");
    }
    return NULL;
}

static uint64_t subresource_hash_value(
    VkImage image, VkImageAspectFlagBits aspect, uint32_t mip,
    uint32_t layer) {
    uint64_t value = (uint64_t)((uintptr_t)image >> 4);
    value ^= (uint64_t)aspect * UINT64_C(0x9e3779b185ebca87);
    value ^= (uint64_t)mip * UINT64_C(0xc2b2ae3d27d4eb4f);
    value ^= (uint64_t)layer * UINT64_C(0x165667b19e3779f9);
    value ^= value >> 31;
    return value;
}

static SubresourceState* find_subresource(
    VkImage image, VkImageAspectFlagBits aspect, uint32_t mip,
    uint32_t layer, bool create) {
    const size_t start =
        (size_t)subresource_hash_value(image, aspect, mip, layer) &
        (kMaxSubresources - 1);
    for (size_t probe = 0; probe < kMaxSubresources; ++probe) {
        SubresourceState* state =
            &g_subresources[(start + probe) & (kMaxSubresources - 1)];
        if (state->occupied && state->image == image &&
            state->aspect == aspect && state->mip == mip &&
            state->layer == layer) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->image = image;
            state->aspect = aspect;
            state->mip = mip;
            state->layer = layer;
            return state;
        }
    }
    if (create) {
        overflow("image_subresources");
    }
    return NULL;
}

static SyncState* find_sync(
    SyncState* states, uintptr_t handle, bool create,
    const char* table) {
    const size_t start = handle_hash(handle, kMaxSyncObjects);
    for (size_t probe = 0; probe < kMaxSyncObjects; ++probe) {
        SyncState* state =
            &states[(start + probe) & (kMaxSyncObjects - 1)];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied) {
            if (!create) {
                return NULL;
            }
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow(table);
    }
    return NULL;
}

static QueueState* find_queue(VkQueue handle, bool create) {
    for (size_t index = 0; index < kMaxQueues; ++index) {
        QueueState* state = &g_queues[index];
        if (state->occupied && state->handle == handle) {
            return state;
        }
        if (!state->occupied && create) {
            memset(state, 0, sizeof(*state));
            state->occupied = true;
            state->handle = handle;
            return state;
        }
    }
    if (create) {
        overflow("queues");
    }
    return NULL;
}

static bool image_descriptor_type(VkDescriptorType type) {
    return type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
}

static const LayoutBindingState* find_layout_binding(
    const DescriptorSetLayoutState* layout, uint32_t binding) {
    if (!layout || !layout->live) {
        return NULL;
    }
    for (uint32_t index = 0; index < layout->binding_count; ++index) {
        if (layout->bindings[index].binding == binding) {
            return &layout->bindings[index];
        }
    }
    return NULL;
}

static bool resolve_descriptor_location(
    const DescriptorSetState* set, uint32_t first_binding,
    uint32_t first_element, uint32_t linear_index,
    uint32_t* binding, uint32_t* element) {
    DescriptorSetLayoutState* layout =
        set ? find_set_layout(set->layout, false) : NULL;
    uint32_t current_binding = first_binding;
    uint32_t current_element = first_element;
    uint32_t remaining = linear_index;
    while (layout) {
        const LayoutBindingState* entry =
            find_layout_binding(layout, current_binding);
        if (!entry || current_element > entry->count) {
            return false;
        }
        const uint32_t available = entry->count - current_element;
        if (remaining < available) {
            *binding = current_binding;
            *element = current_element + remaining;
            return true;
        }
        remaining -= available;
        ++current_binding;
        current_element = 0;
    }
    return false;
}

static VkSampler immutable_sampler_for_slot(
    const DescriptorSetState* set, uint32_t binding,
    uint32_t element) {
    DescriptorSetLayoutState* layout =
        set ? find_set_layout(set->layout, false) : NULL;
    if (!layout || !layout->live) {
        return VK_NULL_HANDLE;
    }
    const LayoutBindingState* entry =
        find_layout_binding(layout, binding);
    if (!entry || !entry->has_immutable_samplers ||
        element >= entry->count) {
        return VK_NULL_HANDLE;
    }
    const uint32_t sampler_index =
        entry->immutable_sampler_offset + element;
    if (sampler_index >= layout->immutable_sampler_count) {
        return VK_NULL_HANDLE;
    }
    return layout->immutable_samplers[sampler_index];
}

static void invalidate_set_record(DescriptorSetState* set) {
    if (!set) {
        return;
    }
    set->live = false;
    set->stale_slots = 0;
}

static void invalidate_dead_slots(void) {
    for (size_t index = 0; index < kMaxDescriptorSlots; ++index) {
        DescriptorSlotState* slot = &g_slots[index];
        if (!slot->occupied || !slot->valid) {
            continue;
        }
        DescriptorSetState* set = find_set(slot->set, false);
        if (!set || !set->live) {
            slot->valid = false;
            slot->stale = false;
        }
    }
}

static void write_slot(
    VkDescriptorSet set_handle, uint32_t binding, uint32_t element,
    VkDescriptorType type, VkImageView view, VkSampler sampler,
    VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range,
    VkBufferView buffer_view, VkImageLayout image_layout,
    bool known, uint64_t sequence) {
    DescriptorSetState* set = find_set(set_handle, true);
    DescriptorSlotState* slot =
        find_slot(set_handle, binding, element, true);
    if (!set || !slot) {
        return;
    }
    if (slot->valid && slot->stale && set->stale_slots > 0) {
        --set->stale_slots;
    }
    slot->valid = true;
    slot->stale = false;
    slot->known = known;
    slot->type = type;
    slot->view = view;
    slot->sampler = sampler;
    slot->buffer = buffer;
    slot->offset = offset;
    slot->range = range;
    slot->buffer_view = buffer_view;
    slot->image_layout = image_layout;
    ViewState* view_state =
        view != VK_NULL_HANDLE ? find_view(view, false) : NULL;
    if (view != VK_NULL_HANDLE && !view_state) {
        count(kDescriptorUnknownViewsWritten, 1);
    } else if (view_state && !view_state->live) {
        slot->stale = true;
        ++set->stale_slots;
        count(kDescriptorDeadViewsWritten, 1);
        if (g_active && g_anomalies++ < kAnomalyLimit) {
            audit_log(
                "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                " type=descriptor_write_dead_view set=%p binding=%u"
                " element=%u view=%p",
                sequence, (void*)set_handle, binding, element, (void*)view);
        }
    }
    ResourceState* buffer_state = buffer != VK_NULL_HANDLE
        ? find_resource(
              g_buffers, kMaxBuffers, (uintptr_t)buffer, false, "buffers")
        : NULL;
    ResourceState* sampler_state = sampler != VK_NULL_HANDLE
        ? find_resource(
              g_samplers, kMaxSamplers, (uintptr_t)sampler, false, "samplers")
        : NULL;
    ResourceState* buffer_view_state = buffer_view != VK_NULL_HANDLE
        ? find_resource(
              g_buffer_views, kMaxBufferViews,
              (uintptr_t)buffer_view, false, "buffer_views")
        : NULL;
    if ((buffer != VK_NULL_HANDLE && !buffer_state) ||
        (sampler != VK_NULL_HANDLE && !sampler_state) ||
        (buffer_view != VK_NULL_HANDLE && !buffer_view_state)) {
        count(kDescriptorUnknownResourcesWritten, 1);
    }
    if ((buffer_state && !buffer_state->live) ||
        (sampler_state && !sampler_state->live) ||
        (buffer_view_state && !buffer_view_state->live)) {
        if (!slot->stale) {
            slot->stale = true;
            ++set->stale_slots;
        }
        count(kDescriptorDeadResourcesWritten, 1);
    }
    set->last_update_sequence = sequence;
}

static void descriptor_slot_coverage(
    const DescriptorSetState* set, uint32_t* known,
    uint32_t* unknown, bool* layout_known) {
    *known = 0;
    *unknown = 0;
    *layout_known = false;
    DescriptorSetLayoutState* layout =
        set ? find_set_layout(set->layout, false) : NULL;
    if (!layout || !layout->live) {
        return;
    }
    *layout_known = true;
    for (uint32_t binding_index = 0;
         binding_index < layout->binding_count; ++binding_index) {
        const LayoutBindingState* binding =
            &layout->bindings[binding_index];
        for (uint32_t element = 0; element < binding->count; ++element) {
            DescriptorSlotState* slot = find_slot(
                set->handle, binding->binding, element, false);
            if (slot && slot->valid && slot->known) {
                ++*known;
            } else {
                ++*unknown;
            }
        }
    }
}

static uint64_t descriptor_content_hash(const DescriptorSetState* set) {
    DescriptorSetLayoutState* layout =
        set ? find_set_layout(set->layout, false) : NULL;
    uint64_t hash = UINT64_C(1469598103934665603);
    if (!layout || !layout->live) {
        return 0;
    }
    for (uint32_t binding_index = 0;
         binding_index < layout->binding_count; ++binding_index) {
        const LayoutBindingState* binding =
            &layout->bindings[binding_index];
        for (uint32_t element = 0; element < binding->count; ++element) {
            DescriptorSlotState* slot = find_slot(
                set->handle, binding->binding, element, false);
            const uint64_t values[] = {
                binding->binding,
                element,
                slot && slot->valid ? slot->type : UINT64_MAX,
                slot && slot->known ? (uint64_t)(uintptr_t)slot->view : 0,
                slot && slot->known ? (uint64_t)(uintptr_t)slot->sampler : 0,
                slot && slot->known ? (uint64_t)(uintptr_t)slot->buffer : 0,
                slot && slot->known ? (uint64_t)slot->offset : 0,
                slot && slot->known ? (uint64_t)slot->range : 0,
                slot && slot->known
                    ? (uint64_t)(uintptr_t)slot->buffer_view : 0,
                slot && slot->known ? (uint64_t)slot->image_layout : 0,
            };
            for (size_t index = 0;
                 index < sizeof(values) / sizeof(values[0]); ++index) {
                hash ^= values[index];
                hash *= UINT64_C(1099511628211);
            }
        }
    }
    return hash;
}

static void sample_descriptor_slots(
    VkCommandBuffer command_buffer, uint32_t set_index,
    const DescriptorSetState* set) {
    DescriptorSetLayoutState* layout =
        set ? find_set_layout(set->layout, false) : NULL;
    if (!layout || !layout->live) {
        return;
    }
    for (uint32_t binding_index = 0;
         binding_index < layout->binding_count; ++binding_index) {
        const LayoutBindingState* binding =
            &layout->bindings[binding_index];
        for (uint32_t element = 0; element < binding->count; ++element) {
            if (g_descriptor_slot_samples >=
                kDescriptorSlotSampleLimit) {
                return;
            }
            DescriptorSlotState* slot = find_slot(
                set->handle, binding->binding, element, false);
            ResourceState* buffer_state =
                slot && slot->buffer != VK_NULL_HANDLE
                    ? find_resource(
                          g_buffers, kMaxBuffers,
                          (uintptr_t)slot->buffer, false, "buffers")
                    : NULL;
            ResourceState* sampler_state =
                slot && slot->sampler != VK_NULL_HANDLE
                    ? find_resource(
                          g_samplers, kMaxSamplers,
                          (uintptr_t)slot->sampler, false, "samplers")
                    : NULL;
            ResourceState* buffer_view_state =
                slot && slot->buffer_view != VK_NULL_HANDLE
                    ? find_resource(
                          g_buffer_views, kMaxBufferViews,
                          (uintptr_t)slot->buffer_view, false,
                          "buffer_views")
                    : NULL;
            ++g_descriptor_slot_samples;
            audit_log(
                "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                " type=descriptor_slot command_buffer=%p set_index=%u"
                " set=%p binding=%u element=%u expected_type=%d"
                " known=%s stale=%s actual_type=%d view=%p sampler=%p"
                " buffer=%p offset=%" PRIu64 " range=%" PRIu64
                " buffer_live=%s sampler_live=%s"
                " buffer_view=%p buffer_view_live=%s image_layout=%d",
                g_sequence, (void*)command_buffer, set_index,
                (void*)set->handle, binding->binding, element,
                binding->type,
                slot && slot->valid && slot->known ? "yes" : "no",
                slot && slot->valid && slot->stale ? "yes" : "no",
                slot && slot->valid ? slot->type : -1,
                slot && slot->valid ? (void*)slot->view : NULL,
                slot && slot->valid ? (void*)slot->sampler : NULL,
                slot && slot->valid ? (void*)slot->buffer : NULL,
                slot && slot->valid ? (uint64_t)slot->offset : 0,
                slot && slot->valid ? (uint64_t)slot->range : 0,
                slot && slot->buffer != VK_NULL_HANDLE
                    ? (buffer_state && buffer_state->live ? "yes" : "no")
                    : "n/a",
                slot && slot->sampler != VK_NULL_HANDLE
                    ? (sampler_state && sampler_state->live ? "yes" : "no")
                    : "n/a",
                slot && slot->valid ? (void*)slot->buffer_view : NULL,
                slot && slot->buffer_view != VK_NULL_HANDLE
                    ? (buffer_view_state && buffer_view_state->live
                           ? "yes" : "no")
                    : "n/a",
                slot && slot->valid ? slot->image_layout : -1);
        }
    }
}

void teso4m4_render_audit_reset(void) {
    pthread_mutex_lock(&g_lock);
    g_logger = NULL;
    g_mirror_enabled = false;
    g_mirror_start_sequence = 0;
    g_active = false;
    g_complete = false;
    g_sequence = 0;
    g_audit_start_sequence = 0;
    g_mirror_overflows = 0;
    g_samples = 0;
    g_pass_tail_samples = 0;
    g_descriptor_slot_samples = 0;
    g_command_samples = 0;
    g_barrier_samples = 0;
    g_anomalies = 0;
    memset(g_counters, 0, sizeof(g_counters));
    memset(g_images, 0, sizeof(g_images));
    memset(g_views, 0, sizeof(g_views));
    memset(g_buffers, 0, sizeof(g_buffers));
    memset(g_buffer_views, 0, sizeof(g_buffer_views));
    memset(g_samplers, 0, sizeof(g_samplers));
    memset(g_set_layouts, 0, sizeof(g_set_layouts));
    memset(g_sets, 0, sizeof(g_sets));
    memset(g_slots, 0, sizeof(g_slots));
    memset(g_render_passes, 0, sizeof(g_render_passes));
    memset(g_framebuffers, 0, sizeof(g_framebuffers));
    memset(g_pipelines, 0, sizeof(g_pipelines));
    memset(g_command_buffers, 0, sizeof(g_command_buffers));
    memset(g_subresources, 0, sizeof(g_subresources));
    memset(g_semaphores, 0, sizeof(g_semaphores));
    memset(g_fences, 0, sizeof(g_fences));
    memset(g_queues, 0, sizeof(g_queues));
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_set_logger(
    Teso4m4RenderAuditLogFunction logger) {
    pthread_mutex_lock(&g_lock);
    g_logger = logger;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_enable_mirror(void) {
    pthread_mutex_lock(&g_lock);
    if (!g_mirror_enabled) {
        g_mirror_enabled = true;
        g_mirror_start_sequence = next_sequence();
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_begin(void) {
    pthread_mutex_lock(&g_lock);
    if (!g_active && !g_complete) {
        memset(g_counters, 0, sizeof(g_counters));
        g_counters[kStateOverflows] = g_mirror_overflows;
        g_samples = 0;
        g_pass_tail_samples = 0;
        g_descriptor_slot_samples = 0;
        g_command_samples = 0;
        g_barrier_samples = 0;
        g_anomalies = 0;
        g_audit_start_sequence = next_sequence();
        g_active = true;
        audit_log(
            "RENDER_AUDIT_BEGIN: mirror=%s sample_limit=%u anomaly_limit=%u"
            " pass_tail_limit=%u slot_capacity=%u"
            " mirror_start_sequence=%" PRIu64
            " audit_start_sequence=%" PRIu64,
            g_mirror_enabled ? "enabled" : "disabled", kSampleLimit,
            kAnomalyLimit, kPassTailLimit, kMaxDescriptorSlots,
            g_mirror_start_sequence, g_audit_start_sequence);
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_finish(const char* reason) {
    pthread_mutex_lock(&g_lock);
    if (!g_complete) {
        g_active = false;
        g_complete = true;
        audit_log(
            "RENDER_AUDIT_SUMMARY: reason=%s complete=yes samples=%u"
            " pass_tails=%u descriptor_slot_samples=%u anomalies=%u",
            reason ? reason : "unknown", g_samples, g_pass_tail_samples,
            g_descriptor_slot_samples, g_anomalies);
        for (size_t index = 0; index < kAuditCounterCount; ++index) {
            audit_log(
                "RENDER_AUDIT_COUNT: name=%s value=%" PRIu64,
                kCounterNames[index], g_counters[index]);
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_image(
    const VkImageCreateInfo* info, VkImage image) {
    if (!info || image == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ImageState* state = find_image(image, true);
    if (state) {
        state->live = true;
        state->flags = info->flags;
        state->format = info->format;
        state->extent = info->extent;
        state->mip_levels = info->mipLevels;
        state->array_layers = info->arrayLayers;
        state->layout = info->initialLayout;
        state->initial_layout = info->initialLayout;
        state->layout_known = true;
        state->memory = VK_NULL_HANDLE;
        state->offset = 0;
        state->size = 0;
        state->size_known = false;
        state->bound = false;
        state->destroy_sequence = 0;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_image(VkImage image) {
    pthread_mutex_lock(&g_lock);
    ImageState* state = find_image(image, false);
    if (state) {
        state->live = false;
        state->destroy_sequence = next_sequence();
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_image_requirements(
    VkImage image, const VkMemoryRequirements* requirements) {
    if (!requirements) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ImageState* state = find_image(image, true);
    if (state) {
        state->size = requirements->size;
        state->size_known = true;
    }
    pthread_mutex_unlock(&g_lock);
}

static bool ranges_overlap(
    VkDeviceSize first_offset, VkDeviceSize first_size,
    VkDeviceSize second_offset, VkDeviceSize second_size) {
    return first_size > 0 && second_size > 0 &&
           first_offset < second_offset + second_size &&
           second_offset < first_offset + first_size;
}

void teso4m4_render_audit_bind_image(
    VkImage image, VkDeviceMemory memory, VkDeviceSize offset) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    ImageState* state = find_image(image, true);
    count(kImageBinds, 1);
    if (state) {
        state->memory = memory;
        state->offset = offset;
        state->bound = true;
        if (g_active && state->size_known) {
            for (size_t index = 0; index < kMaxImages; ++index) {
                ImageState* other = &g_images[index];
                if (!other->occupied || !other->bound ||
                    !other->size_known || other == state ||
                    other->memory != memory ||
                    !ranges_overlap(
                        offset, state->size, other->offset, other->size)) {
                    continue;
                }
                if (!other->live) {
                    count(kImageDeadRangeReuses, 1);
                    if (g_samples++ < kSampleLimit) {
                        audit_log(
                            "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                            " type=dead_image_range_reuse image=%p old_image=%p"
                            " memory=%p offset=%" PRIu64 " size=%" PRIu64
                            " old_offset=%" PRIu64 " old_size=%" PRIu64
                            " old_destroy_sequence=%" PRIu64,
                            sequence, (void*)image, (void*)other->handle,
                            (void*)memory, (uint64_t)offset,
                            (uint64_t)state->size, (uint64_t)other->offset,
                            (uint64_t)other->size,
                            other->destroy_sequence);
                    }
                    continue;
                }
                count(kImageLiveOverlaps, 1);
                const bool declared =
                    (state->flags & VK_IMAGE_CREATE_ALIAS_BIT) != 0 &&
                    (other->flags & VK_IMAGE_CREATE_ALIAS_BIT) != 0;
                if (!declared) {
                    count(kImageUndeclaredOverlaps, 1);
                }
                if (g_anomalies++ < kAnomalyLimit) {
                    audit_log(
                        "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                        " type=live_image_memory_overlap image=%p other=%p"
                        " memory=%p offset=%" PRIu64 " size=%" PRIu64
                        " other_offset=%" PRIu64 " other_size=%" PRIu64
                        " declared_alias=%s",
                        sequence, (void*)image, (void*)other->handle,
                        (void*)memory, (uint64_t)offset,
                        (uint64_t)state->size, (uint64_t)other->offset,
                        (uint64_t)other->size, declared ? "yes" : "no");
                }
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_image_view(
    const VkImageViewCreateInfo* info, VkImageView view) {
    if (!info || view == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ViewState* state = find_view(view, true);
    if (state) {
        state->live = true;
        state->image = info->image;
        state->range = info->subresourceRange;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_image_view(VkImageView view) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    ViewState* state = find_view(view, false);
    if (state) {
        state->live = false;
    }
    if (g_mirror_enabled) {
        for (size_t index = 0; index < kMaxDescriptorSlots; ++index) {
            DescriptorSlotState* slot = &g_slots[index];
            if (!slot->occupied || !slot->valid || slot->stale ||
                slot->view != view) {
                continue;
            }
            slot->stale = true;
            DescriptorSetState* set = find_set(slot->set, false);
            if (set && set->live) {
                ++set->stale_slots;
            }
            count(kDescriptorViewsDestroyedWhileReferenced, 1);
            if (g_active && g_anomalies++ < kAnomalyLimit) {
                audit_log(
                    "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                    " type=descriptor_view_destroyed set=%p binding=%u"
                    " element=%u view=%p",
                    sequence, (void*)slot->set, slot->binding,
                    slot->element, (void*)view);
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

static void destroy_descriptor_resource(
    uintptr_t handle, uint32_t kind) {
    ResourceState* states =
        kind == 0 ? g_buffers
                  : (kind == 1 ? g_samplers : g_buffer_views);
    const size_t capacity =
        kind == 0 ? kMaxBuffers
                  : (kind == 1 ? kMaxSamplers : kMaxBufferViews);
    ResourceState* state = find_resource(
        states, capacity, handle, false,
        kind == 0 ? "buffers"
                  : (kind == 1 ? "samplers" : "buffer_views"));
    if (state) {
        state->live = false;
    }
    if (!g_mirror_enabled) {
        return;
    }
    for (size_t index = 0; index < kMaxDescriptorSlots; ++index) {
        DescriptorSlotState* slot = &g_slots[index];
        const uintptr_t slot_handle =
            kind == 0 ? (uintptr_t)slot->buffer
                      : (kind == 1 ? (uintptr_t)slot->sampler
                                   : (uintptr_t)slot->buffer_view);
        if (!slot->occupied || !slot->valid || slot->stale ||
            slot_handle != handle) {
            continue;
        }
        slot->stale = true;
        DescriptorSetState* set = find_set(slot->set, false);
        if (set && set->live) {
            ++set->stale_slots;
        }
        count(kDescriptorResourcesDestroyedWhileReferenced, 1);
    }
}

void teso4m4_render_audit_create_buffer(VkBuffer buffer) {
    if (buffer == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ResourceState* state = find_resource(
        g_buffers, kMaxBuffers, (uintptr_t)buffer, true, "buffers");
    if (state) {
        state->live = true;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_buffer(VkBuffer buffer) {
    if (buffer == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    destroy_descriptor_resource((uintptr_t)buffer, 0);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_buffer_view(VkBufferView buffer_view) {
    if (buffer_view == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ResourceState* state = find_resource(
        g_buffer_views, kMaxBufferViews, (uintptr_t)buffer_view,
        true, "buffer_views");
    if (state) {
        state->live = true;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_buffer_view(VkBufferView buffer_view) {
    if (buffer_view == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    destroy_descriptor_resource((uintptr_t)buffer_view, 2);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_sampler(VkSampler sampler) {
    if (sampler == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    ResourceState* state = find_resource(
        g_samplers, kMaxSamplers, (uintptr_t)sampler, true, "samplers");
    if (state) {
        state->live = true;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_sampler(VkSampler sampler) {
    if (sampler == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    destroy_descriptor_resource((uintptr_t)sampler, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_descriptor_set_layout(
    const VkDescriptorSetLayoutCreateInfo* info,
    VkDescriptorSetLayout layout) {
    if (!info || layout == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    DescriptorSetLayoutState* state = find_set_layout(layout, true);
    if (state) {
        state->live = true;
        state->binding_count = info->bindingCount;
        state->total_slots = 0;
        state->immutable_sampler_count = 0;
        if (state->binding_count > kMaxLayoutBindings) {
            state->binding_count = kMaxLayoutBindings;
            overflow("descriptor_layout_bindings");
        }
        for (uint32_t index = 0; index < state->binding_count; ++index) {
            const VkDescriptorSetLayoutBinding* binding =
                &info->pBindings[index];
            LayoutBindingState* destination =
                &state->bindings[index];
            *destination = (LayoutBindingState){
                .binding = binding->binding,
                .type = binding->descriptorType,
                .count = binding->descriptorCount,
            };
            if (binding->pImmutableSamplers) {
                destination->has_immutable_samplers = true;
                destination->immutable_sampler_offset =
                    state->immutable_sampler_count;
                if (state->immutable_sampler_count +
                        binding->descriptorCount >
                    kMaxImmutableSamplersPerLayout) {
                    overflow("immutable_samplers");
                    destination->has_immutable_samplers = false;
                } else {
                    memcpy(
                        &state->immutable_samplers[
                            state->immutable_sampler_count],
                        binding->pImmutableSamplers,
                        binding->descriptorCount *
                            sizeof(state->immutable_samplers[0]));
                    state->immutable_sampler_count +=
                        binding->descriptorCount;
                }
            }
            state->total_slots += binding->descriptorCount;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_descriptor_set_layout(
    VkDescriptorSetLayout layout) {
    pthread_mutex_lock(&g_lock);
    DescriptorSetLayoutState* state = find_set_layout(layout, false);
    if (state) {
        state->live = false;
    }
    pthread_mutex_unlock(&g_lock);
}

enum {
    kAttachmentRoleInput = 1u << 0,
    kAttachmentRoleColor = 1u << 1,
    kAttachmentRoleResolve = 1u << 2,
    kAttachmentRoleDepthStencil = 1u << 3,
    kAttachmentRolePreserve = 1u << 4,
};

static void mark_attachment_role(
    RenderPassState* state, const VkAttachmentReference* reference,
    uint32_t role, uint32_t subpass) {
    if (reference && reference->attachment != VK_ATTACHMENT_UNUSED &&
        reference->attachment < state->attachment_count) {
        state->attachment_roles[reference->attachment] |= role;
        if (state->attachment_first_subpass[reference->attachment] ==
            UINT32_MAX) {
            state->attachment_first_subpass[reference->attachment] =
                subpass;
        }
        state->attachment_last_subpass[reference->attachment] = subpass;
        if (!state->attachment_use_layout_known[reference->attachment]) {
            state->attachment_use_layouts[reference->attachment] =
                reference->layout;
            state->attachment_use_layout_known[reference->attachment] = true;
        }
    }
}

void teso4m4_render_audit_create_render_pass(
    const VkRenderPassCreateInfo* info, VkRenderPass render_pass) {
    if (!info || render_pass == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    RenderPassState* state = find_render_pass(render_pass, true);
    if (state) {
        state->live = true;
        state->attachment_count = info->attachmentCount;
        if (state->attachment_count > kMaxAttachments) {
            state->attachment_count = kMaxAttachments;
            overflow("render_pass_attachments");
        }
        if (state->attachment_count > 0 && info->pAttachments) {
            memcpy(
                state->attachments, info->pAttachments,
                state->attachment_count * sizeof(state->attachments[0]));
        }
        memset(state->attachment_roles, 0, sizeof(state->attachment_roles));
        for (uint32_t index = 0; index < kMaxAttachments; ++index) {
            state->attachment_first_subpass[index] = UINT32_MAX;
            state->attachment_last_subpass[index] = UINT32_MAX;
        }
        memset(
            state->attachment_use_layout_known, 0,
            sizeof(state->attachment_use_layout_known));
        for (uint32_t subpass_index = 0;
             subpass_index < info->subpassCount; ++subpass_index) {
            const VkSubpassDescription* subpass =
                &info->pSubpasses[subpass_index];
            for (uint32_t index = 0;
                 index < subpass->inputAttachmentCount; ++index) {
                mark_attachment_role(
                    state, &subpass->pInputAttachments[index],
                    kAttachmentRoleInput, subpass_index);
            }
            for (uint32_t index = 0;
                 index < subpass->colorAttachmentCount; ++index) {
                mark_attachment_role(
                    state, &subpass->pColorAttachments[index],
                    kAttachmentRoleColor, subpass_index);
                if (subpass->pResolveAttachments) {
                    mark_attachment_role(
                        state, &subpass->pResolveAttachments[index],
                        kAttachmentRoleResolve, subpass_index);
                }
            }
            mark_attachment_role(
                state, subpass->pDepthStencilAttachment,
                kAttachmentRoleDepthStencil, subpass_index);
            for (uint32_t index = 0;
                 index < subpass->preserveAttachmentCount; ++index) {
                const uint32_t attachment =
                    subpass->pPreserveAttachments[index];
                if (attachment < state->attachment_count) {
                    state->attachment_roles[attachment] |=
                        kAttachmentRolePreserve;
                    if (state->attachment_first_subpass[attachment] ==
                        UINT32_MAX) {
                        state->attachment_first_subpass[attachment] =
                            subpass_index;
                    }
                    state->attachment_last_subpass[attachment] =
                        subpass_index;
                }
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_render_pass(VkRenderPass render_pass) {
    pthread_mutex_lock(&g_lock);
    RenderPassState* state = find_render_pass(render_pass, false);
    if (state) {
        state->live = false;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_framebuffer(
    const VkFramebufferCreateInfo* info, VkFramebuffer framebuffer) {
    if (!info || framebuffer == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    FramebufferState* state = find_framebuffer(framebuffer, true);
    if (state) {
        state->live = true;
        state->render_pass = info->renderPass;
        state->attachment_count = info->attachmentCount;
        if (state->attachment_count > kMaxAttachments) {
            state->attachment_count = kMaxAttachments;
            overflow("framebuffer_attachments");
        }
        if (state->attachment_count > 0 && info->pAttachments) {
            memcpy(
                state->attachments, info->pAttachments,
                state->attachment_count * sizeof(state->attachments[0]));
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_framebuffer(
    VkFramebuffer framebuffer) {
    pthread_mutex_lock(&g_lock);
    FramebufferState* state = find_framebuffer(framebuffer, false);
    if (state) {
        state->live = false;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_allocate_descriptor_sets(
    const VkDescriptorSetAllocateInfo* info, const VkDescriptorSet* sets) {
    if (!info || !sets) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    bool invalidated_live_set = false;
    for (uint32_t index = 0; index < info->descriptorSetCount; ++index) {
        DescriptorSetState* state = find_set(sets[index], true);
        if (state && state->live) {
            invalidate_set_record(state);
            invalidated_live_set = true;
        }
    }
    if (invalidated_live_set) {
        invalidate_dead_slots();
    }
    for (uint32_t index = 0; index < info->descriptorSetCount; ++index) {
        DescriptorSetState* state = find_set(sets[index], false);
        if (state) {
            state->live = true;
            state->pool = info->descriptorPool;
            state->layout = info->pSetLayouts
                ? info->pSetLayouts[index] : VK_NULL_HANDLE;
            state->stale_slots = 0;
            state->last_update_sequence = 0;
            DescriptorSetLayoutState* layout =
                find_set_layout(state->layout, false);
            if (layout && layout->live) {
                for (uint32_t binding_index = 0;
                     binding_index < layout->binding_count;
                     ++binding_index) {
                    const LayoutBindingState* binding =
                        &layout->bindings[binding_index];
                    if (!binding->has_immutable_samplers) {
                        continue;
                    }
                    for (uint32_t element = 0;
                         element < binding->count; ++element) {
                        const bool sampler_only =
                            binding->type ==
                            VK_DESCRIPTOR_TYPE_SAMPLER;
                        write_slot(
                            state->handle, binding->binding, element,
                            binding->type, VK_NULL_HANDLE,
                            layout->immutable_samplers[
                                binding->immutable_sampler_offset +
                                element],
                            VK_NULL_HANDLE, 0, 0, VK_NULL_HANDLE,
                            VK_IMAGE_LAYOUT_UNDEFINED, sampler_only, 0);
                    }
                }
                state->last_update_sequence = 0;
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_free_descriptor_sets(
    VkDescriptorPool pool, uint32_t count_value,
    const VkDescriptorSet* sets) {
    (void)pool;
    if (!sets) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    for (uint32_t index = 0; index < count_value; ++index) {
        invalidate_set_record(find_set(sets[index], false));
    }
    invalidate_dead_slots();
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_reset_descriptor_pool(VkDescriptorPool pool) {
    pthread_mutex_lock(&g_lock);
    for (size_t index = 0; index < kMaxDescriptorSets; ++index) {
        if (g_sets[index].occupied && g_sets[index].live &&
            g_sets[index].pool == pool) {
            invalidate_set_record(&g_sets[index]);
        }
    }
    invalidate_dead_slots();
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_descriptor_pool(VkDescriptorPool pool) {
    teso4m4_render_audit_reset_descriptor_pool(pool);
}

void teso4m4_render_audit_update_descriptor_sets(
    uint32_t write_count, const VkWriteDescriptorSet* writes,
    uint32_t copy_count, const VkCopyDescriptorSet* copies) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    count(kDescriptorUpdateCalls, 1);
    if (!g_mirror_enabled) {
        pthread_mutex_unlock(&g_lock);
        return;
    }
    for (uint32_t write_index = 0; write_index < write_count; ++write_index) {
        const VkWriteDescriptorSet* write = &writes[write_index];
        DescriptorSetState* destination =
            find_set(write->dstSet, true);
        if (image_descriptor_type(write->descriptorType)) {
            count(kDescriptorImageWrites, write->descriptorCount);
        }
        if (write->descriptorCount > 1) {
            count(kDescriptorMultiWrites, 1);
        }
        for (uint32_t element = 0; element < write->descriptorCount;
             ++element) {
            uint32_t binding = write->dstBinding;
            uint32_t array_element =
                write->dstArrayElement + element;
            const bool location_known = resolve_descriptor_location(
                destination, write->dstBinding,
                write->dstArrayElement, element,
                &binding, &array_element);
            const VkDescriptorImageInfo* image =
                write->pImageInfo ? &write->pImageInfo[element] : NULL;
            const VkDescriptorBufferInfo* buffer =
                write->pBufferInfo ? &write->pBufferInfo[element] : NULL;
            const VkBufferView buffer_view =
                write->pTexelBufferView
                    ? write->pTexelBufferView[element]
                    : VK_NULL_HANDLE;
            const bool content_known =
                location_known &&
                (image || buffer || write->pTexelBufferView);
            write_slot(
                write->dstSet, binding, array_element,
                write->descriptorType,
                image ? image->imageView : VK_NULL_HANDLE,
                image && image->sampler != VK_NULL_HANDLE
                    ? image->sampler
                    : immutable_sampler_for_slot(
                          destination, binding, array_element),
                buffer ? buffer->buffer : VK_NULL_HANDLE,
                buffer ? buffer->offset : 0,
                buffer ? buffer->range : 0,
                buffer_view,
                image ? image->imageLayout : VK_IMAGE_LAYOUT_UNDEFINED,
                content_known, sequence);
        }
    }
    for (uint32_t copy_index = 0; copy_index < copy_count; ++copy_index) {
        const VkCopyDescriptorSet* copy = &copies[copy_index];
        count(kDescriptorCopies, copy->descriptorCount);
        DescriptorSetState* source_set = find_set(copy->srcSet, false);
        DescriptorSetState* destination_set =
            find_set(copy->dstSet, true);
        for (uint32_t element = 0; element < copy->descriptorCount;
             ++element) {
            uint32_t source_binding = copy->srcBinding;
            uint32_t source_element =
                copy->srcArrayElement + element;
            uint32_t destination_binding = copy->dstBinding;
            uint32_t destination_element =
                copy->dstArrayElement + element;
            const bool source_location = resolve_descriptor_location(
                source_set, copy->srcBinding, copy->srcArrayElement,
                element, &source_binding, &source_element);
            const bool destination_location =
                resolve_descriptor_location(
                    destination_set, copy->dstBinding,
                    copy->dstArrayElement, element,
                    &destination_binding, &destination_element);
            DescriptorSlotState* source = find_slot(
                copy->srcSet, source_binding, source_element, false);
            if (source_location && destination_location &&
                source && source->valid) {
                write_slot(
                    copy->dstSet, destination_binding,
                    destination_element, source->type,
                    source->view, source->sampler, source->buffer,
                    source->offset, source->range, source->buffer_view,
                    source->image_layout, source->known, sequence);
            } else {
                write_slot(
                    copy->dstSet, destination_binding,
                    destination_element, VK_DESCRIPTOR_TYPE_MAX_ENUM,
                    VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
                    0, 0, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
                    false, sequence);
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_graphics_pipelines(
    VkPipelineCache cache, uint32_t count_value,
    const VkGraphicsPipelineCreateInfo* infos,
    const VkPipeline* pipelines) {
    if (!infos || !pipelines) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    count(kGraphicsPipelinesCreated, count_value);
    if (cache != VK_NULL_HANDLE) {
        count(kGraphicsPipelinesWithCache, count_value);
    }
    for (uint32_t index = 0; index < count_value; ++index) {
        if (pipelines[index] == VK_NULL_HANDLE) {
            continue;
        }
        PipelineState* state = find_pipeline(pipelines[index], true);
        if (state) {
            state->live = true;
            state->created_during_audit = g_active;
            state->layout = infos[index].layout;
            state->render_pass = infos[index].renderPass;
            state->subpass = infos[index].subpass;
            state->cache = cache;
        }
        if (g_active) {
            count(kGraphicsPipelinesCreatedDuringAudit, 1);
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_pipeline(VkPipeline pipeline) {
    pthread_mutex_lock(&g_lock);
    PipelineState* state = find_pipeline(pipeline, false);
    if (state) {
        state->live = false;
    }
    pthread_mutex_unlock(&g_lock);
}

static uint32_t apply_subresource_layout(
    VkCommandBuffer command_buffer, ImageState* image,
    const VkImageSubresourceRange* range, VkImageLayout expected_old,
    VkImageLayout new_layout, VkPipelineStageFlags source_stage,
    VkPipelineStageFlags destination_stage, VkAccessFlags source_access,
    VkAccessFlags destination_access, uint64_t sequence,
    bool check_expected) {
    if (!image || !range) {
        return 0;
    }
    const uint32_t level_count =
        range->levelCount == VK_REMAINING_MIP_LEVELS
            ? image->mip_levels - range->baseMipLevel
            : range->levelCount;
    const uint32_t layer_count =
        range->layerCount == VK_REMAINING_ARRAY_LAYERS
            ? image->array_layers - range->baseArrayLayer
            : range->layerCount;
    uint32_t updated = 0;
    VkImageAspectFlags aspects = range->aspectMask;
    while (aspects != 0) {
        const VkImageAspectFlags low_bit = aspects & (~aspects + 1u);
        aspects &= ~low_bit;
        for (uint32_t mip = 0; mip < level_count; ++mip) {
            for (uint32_t layer = 0; layer < layer_count; ++layer) {
                SubresourceState* state = find_subresource(
                    image->handle, (VkImageAspectFlagBits)low_bit,
                    range->baseMipLevel + mip,
                    range->baseArrayLayer + layer, true);
                if (!state) {
                    continue;
                }
                if (!state->layout_known) {
                    state->layout = image->initial_layout;
                    state->layout_known = true;
                }
                if (check_expected && g_active && state->layout_known &&
                    expected_old != VK_IMAGE_LAYOUT_UNDEFINED &&
                    state->layout != expected_old) {
                    count(kLayoutMismatches, 1);
                    if (g_anomalies++ < kAnomalyLimit) {
                        audit_log(
                            "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                            " type=subresource_layout_mismatch"
                            " command_buffer=%p image=%p aspect=0x%x"
                            " mip=%u layer=%u tracked=%d declared_old=%d"
                            " declared_new=%d",
                            sequence, (void*)command_buffer,
                            (void*)image->handle, low_bit,
                            range->baseMipLevel + mip,
                            range->baseArrayLayer + layer,
                            state->layout, expected_old, new_layout);
                    }
                }
                state->layout = new_layout;
                state->layout_known = true;
                state->source_stage = source_stage;
                state->destination_stage = destination_stage;
                state->source_access = source_access;
                state->destination_access = destination_access;
                state->sequence = sequence;
                ++updated;
            }
        }
    }
    return updated;
}

static void record_transition(
    CommandBufferState* command, VkImage image,
    const VkImageSubresourceRange* range, VkImageLayout old_layout,
    VkImageLayout new_layout, VkPipelineStageFlags source_stage,
    VkPipelineStageFlags destination_stage, VkAccessFlags source_access,
    VkAccessFlags destination_access, uint64_t sequence,
    bool explicit_barrier) {
    if (!command || !range) {
        overflow("transition_without_command_buffer");
        return;
    }
    if (command->transition_count >= kMaxTransitionsPerCommand) {
        overflow("command_transitions");
        return;
    }
    command->transitions[command->transition_count++] = (TransitionState){
        .image = image,
        .range = *range,
        .old_layout = old_layout,
        .new_layout = new_layout,
        .source_stage = source_stage,
        .destination_stage = destination_stage,
        .source_access = source_access,
        .destination_access = destination_access,
        .record_sequence = sequence,
        .explicit_barrier = explicit_barrier,
    };
    command->last_record_sequence = sequence;
}

static void reset_command_state(
    CommandBufferState* command, uint64_t sequence) {
    if (!command) {
        return;
    }
    ++command->generation;
    command->recording = false;
    command->executable = false;
    command->inside_render_pass = false;
    command->reset_sequence = sequence;
    command->last_record_sequence = sequence;
    command->render_pass = VK_NULL_HANDLE;
    command->framebuffer = VK_NULL_HANDLE;
    command->last_pipeline = VK_NULL_HANDLE;
    command->last_first_set = 0;
    command->last_set_count = 0;
    memset(command->last_sets, 0, sizeof(command->last_sets));
    command->descriptor_bind_calls = 0;
    command->last_descriptor_sequence = 0;
    command->max_bound_set_update_sequence = 0;
    command->bound_set_count = 0;
    command->transition_count = 0;
    memset(command->bound_sets, 0, sizeof(command->bound_sets));
    memset(
        command->bound_set_update_sequences, 0,
        sizeof(command->bound_set_update_sequences));
}

void teso4m4_render_audit_allocate_command_buffers(
    const VkCommandBufferAllocateInfo* info,
    const VkCommandBuffer* command_buffers) {
    if (!info || !command_buffers) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    for (uint32_t index = 0; index < info->commandBufferCount; ++index) {
        CommandBufferState* command =
            find_command_buffer(command_buffers[index], true);
        if (command) {
            memset(command, 0, sizeof(*command));
            command->occupied = true;
            command->live = true;
            command->handle = command_buffers[index];
            command->pool = info->commandPool;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_free_command_buffers(
    VkCommandPool pool, uint32_t count_value,
    const VkCommandBuffer* command_buffers) {
    (void)pool;
    if (!command_buffers) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    for (uint32_t index = 0; index < count_value; ++index) {
        CommandBufferState* command =
            find_command_buffer(command_buffers[index], false);
        if (command) {
            command->live = false;
            reset_command_state(command, next_sequence());
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_reset_command_buffer(
    VkCommandBuffer command_buffer) {
    pthread_mutex_lock(&g_lock);
    reset_command_state(
        find_command_buffer(command_buffer, true), next_sequence());
    count(kCommandBufferResets, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_reset_command_pool(VkCommandPool command_pool) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    count(kCommandPoolResets, 1);
    for (size_t index = 0; index < kMaxCommandBuffers; ++index) {
        CommandBufferState* command = &g_command_buffers[index];
        if (command->occupied && command->live &&
            command->pool == command_pool) {
            reset_command_state(command, sequence);
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_begin_command_buffer(
    VkCommandBuffer command_buffer) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    CommandBufferState* command =
        find_command_buffer(command_buffer, true);
    if (command) {
        if (!command->live) {
            command->live = true;
        }
        ++command->generation;
        command->recording = true;
        command->executable = false;
        command->inside_render_pass = false;
        command->begin_sequence = sequence;
        command->last_record_sequence = sequence;
        command->max_bound_set_update_sequence = 0;
        command->max_submitted_set_update_sequence = 0;
        command->bound_set_count = 0;
        command->transition_count = 0;
        memset(command->bound_sets, 0, sizeof(command->bound_sets));
        memset(
            command->bound_set_update_sequences, 0,
            sizeof(command->bound_set_update_sequences));
    }
    count(kCommandBufferBegins, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_end_command_buffer(
    VkCommandBuffer command_buffer) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    CommandBufferState* command =
        find_command_buffer(command_buffer, true);
    if (command) {
        command->recording = false;
        command->executable = true;
        command->end_sequence = sequence;
        command->last_record_sequence = sequence;
    }
    count(kCommandBufferEnds, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_semaphore(VkSemaphore semaphore) {
    if (semaphore == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    SyncState* state = find_sync(
        g_semaphores, (uintptr_t)semaphore, true, "semaphores");
    if (state) {
        state->live = true;
        state->signaled = false;
        state->signal_sequence = 0;
        state->wait_sequence = 0;
        state->submit_sequence = 0;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_semaphore(VkSemaphore semaphore) {
    pthread_mutex_lock(&g_lock);
    SyncState* state = find_sync(
        g_semaphores, (uintptr_t)semaphore, false, "semaphores");
    if (state) {
        state->live = false;
        state->signaled = false;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_create_fence(
    VkFence fence, VkFenceCreateFlags flags) {
    if (fence == VK_NULL_HANDLE) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    SyncState* state =
        find_sync(g_fences, (uintptr_t)fence, true, "fences");
    if (state) {
        state->live = true;
        state->signaled =
            (flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
        state->signal_sequence =
            state->signaled ? next_sequence() : 0;
        state->wait_sequence = 0;
        state->submit_sequence = 0;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_destroy_fence(VkFence fence) {
    pthread_mutex_lock(&g_lock);
    SyncState* state =
        find_sync(g_fences, (uintptr_t)fence, false, "fences");
    if (state) {
        state->live = false;
        state->signaled = false;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_reset_fences(
    uint32_t fence_count, const VkFence* fences) {
    if (!fences) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    count(kFenceResets, fence_count);
    for (uint32_t index = 0; index < fence_count; ++index) {
        SyncState* state =
            find_sync(g_fences, (uintptr_t)fences[index], true, "fences");
        if (state) {
            state->signaled = false;
            state->signal_sequence = 0;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_wait_for_fences(
    uint32_t fence_count, const VkFence* fences, VkResult result) {
    if (!fences) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    count(kFenceWaits, fence_count);
    for (uint32_t index = 0; index < fence_count; ++index) {
        SyncState* state =
            find_sync(g_fences, (uintptr_t)fences[index], false, "fences");
        if (!state || state->submit_sequence == 0) {
            count(kFenceWaitsWithoutSubmit, 1);
        }
        if (state && result == VK_SUCCESS) {
            state->signaled = true;
            state->wait_sequence = sequence;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_acquire_next_image(
    VkSemaphore semaphore, VkFence fence, VkResult result) {
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    if (semaphore != VK_NULL_HANDLE) {
        SyncState* state = find_sync(
            g_semaphores, (uintptr_t)semaphore, true, "semaphores");
        if (state) {
            state->live = true;
            state->signaled = true;
            state->signal_sequence = sequence;
        }
    }
    if (fence != VK_NULL_HANDLE) {
        SyncState* state =
            find_sync(g_fences, (uintptr_t)fence, true, "fences");
        if (state) {
            state->live = true;
            state->signaled = true;
            state->signal_sequence = sequence;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_queue_submit(
    VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
    VkFence fence) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    QueueState* queue_state = find_queue(queue, true);
    if (queue_state) {
        ++queue_state->submit_ordinal;
        queue_state->last_submit_sequence = sequence;
    }
    count(kQueueSubmits, 1);
    for (uint32_t submit_index = 0;
         submits && submit_index < submit_count; ++submit_index) {
        const VkSubmitInfo* submit = &submits[submit_index];
        VkPipelineStageFlags wait_stage_or = 0;
        for (uint32_t index = 0;
             submit->pWaitDstStageMask &&
             index < submit->waitSemaphoreCount; ++index) {
            wait_stage_or |= submit->pWaitDstStageMask[index];
        }
        count(kSubmitWaitSemaphores, submit->waitSemaphoreCount);
        count(kSubmitSignalSemaphores, submit->signalSemaphoreCount);
        for (uint32_t index = 0;
             index < submit->waitSemaphoreCount; ++index) {
            SyncState* state = find_sync(
                g_semaphores,
                (uintptr_t)submit->pWaitSemaphores[index],
                false, "semaphores");
            if (!state || !state->live || !state->signaled) {
                count(kUnknownSemaphoreWaits, 1);
            }
            if (state) {
                state->signaled = false;
                state->wait_sequence = sequence;
            }
        }
        for (uint32_t index = 0;
             index < submit->commandBufferCount; ++index) {
            CommandBufferState* command = find_command_buffer(
                submit->pCommandBuffers[index], false);
            count(kCommandBufferSubmits, 1);
            if (!command || !command->live || !command->executable ||
                command->end_sequence <= command->reset_sequence) {
                count(kCommandBufferSubmitInvalidGeneration, 1);
                continue;
            }
            for (uint32_t transition_index = 0;
                 transition_index < command->transition_count;
                 ++transition_index) {
                const TransitionState* transition =
                    &command->transitions[transition_index];
                ImageState* image =
                    find_image(transition->image, false);
                const uint32_t subresources = apply_subresource_layout(
                    command->handle, image, &transition->range,
                    transition->old_layout, transition->new_layout,
                    transition->source_stage,
                    transition->destination_stage,
                    transition->source_access,
                    transition->destination_access,
                    transition->record_sequence, true);
                if (transition->explicit_barrier) {
                    count(kSubresourceBarriers, subresources);
                }
                if (image) {
                    image->layout = transition->new_layout;
                    image->layout_known = true;
                }
            }
            command->submit_sequence = sequence;
            command->max_submitted_set_update_sequence = 0;
            for (uint32_t bound = 0;
                 bound < command->bound_set_count; ++bound) {
                DescriptorSetState* set =
                    find_set(command->bound_sets[bound], false);
                if (!set || !set->live) {
                    continue;
                }
                if (set->last_update_sequence >
                    command->max_submitted_set_update_sequence) {
                    command->max_submitted_set_update_sequence =
                        set->last_update_sequence;
                }
                if (set->last_update_sequence !=
                    command->bound_set_update_sequences[bound]) {
                    count(kDescriptorUpdatesAfterRecord, 1);
                }
            }
            if (g_active && g_command_samples++ < kCommandSampleLimit) {
                audit_log(
                    "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                    " type=command_submit queue=%p queue_ordinal=%" PRIu64
                    " command_buffer=%p generation=%" PRIu64
                    " reset_sequence=%" PRIu64
                    " begin_sequence=%" PRIu64
                    " end_sequence=%" PRIu64
                    " last_bind_sequence=%" PRIu64
                    " recorded_descriptor_update_sequence=%" PRIu64
                    " submit_descriptor_update_sequence=%" PRIu64
                    " transitions=%u wait_stage_or=0x%x waits=%u signals=%u",
                    sequence, (void*)queue,
                    queue_state ? queue_state->submit_ordinal : 0,
                    (void*)command->handle, command->generation,
                    command->reset_sequence, command->begin_sequence,
                    command->end_sequence,
                    command->last_descriptor_sequence,
                    command->max_bound_set_update_sequence,
                    command->max_submitted_set_update_sequence,
                    command->transition_count, wait_stage_or,
                    submit->waitSemaphoreCount,
                    submit->signalSemaphoreCount);
            }
        }
        for (uint32_t index = 0;
             index < submit->signalSemaphoreCount; ++index) {
            SyncState* state = find_sync(
                g_semaphores,
                (uintptr_t)submit->pSignalSemaphores[index],
                true, "semaphores");
            if (state) {
                state->live = true;
                state->signaled = true;
                state->signal_sequence = sequence;
                state->submit_sequence = sequence;
            }
        }
    }
    if (fence != VK_NULL_HANDLE) {
        SyncState* state =
            find_sync(g_fences, (uintptr_t)fence, true, "fences");
        if (state) {
            state->live = true;
            state->signaled = false;
            state->submit_sequence = sequence;
            count(kFenceSubmits, 1);
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_queue_present(
    VkQueue queue, const VkPresentInfoKHR* present_info, VkResult result) {
    (void)queue;
    (void)result;
    if (!present_info) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    for (uint32_t index = 0;
         index < present_info->waitSemaphoreCount; ++index) {
        SyncState* state = find_sync(
            g_semaphores,
            (uintptr_t)present_info->pWaitSemaphores[index],
            false, "semaphores");
        if (!state || !state->signaled) {
            count(kUnknownSemaphoreWaits, 1);
        }
        if (state) {
            state->signaled = false;
            state->wait_sequence = sequence;
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_begin_render_pass(
    VkCommandBuffer command_buffer, const VkRenderPassBeginInfo* info) {
    if (!info) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    CommandBufferState* command =
        find_command_buffer(command_buffer, true);
    if (command) {
        command->inside_render_pass = true;
        command->render_pass = info->renderPass;
        command->framebuffer = info->framebuffer;
        command->descriptor_bind_calls = 0;
    }
    count(kRenderPassBegins, 1);
    if (g_active) {
        RenderPassState* render_pass =
            find_render_pass(info->renderPass, false);
        FramebufferState* framebuffer =
            find_framebuffer(info->framebuffer, false);
        uint32_t attachment_count =
            render_pass && framebuffer
                ? render_pass->attachment_count < framebuffer->attachment_count
                    ? render_pass->attachment_count
                    : framebuffer->attachment_count
                : 0;
        for (uint32_t index = 0; index < attachment_count; ++index) {
            VkImageView view = framebuffer->attachments[index];
            ViewState* view_state = find_view(view, false);
            ImageState* image_state =
                view_state ? find_image(view_state->image, false) : NULL;
            count(kAttachmentSamples, 1);
            if (!view_state) {
                count(kUnknownAttachmentViews, 1);
            } else if (!view_state->live) {
                count(kDeadAttachmentViews, 1);
            }
            const VkImageLayout initial =
                render_pass->attachments[index].initialLayout;
            const VkImageLayout final =
                render_pass->attachments[index].finalLayout;
            const VkAttachmentLoadOp load_op =
                render_pass->attachments[index].loadOp;
            const VkAttachmentStoreOp store_op =
                render_pass->attachments[index].storeOp;
            if (load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                count(kAttachmentLoadClear, 1);
                if (index >= info->clearValueCount) {
                    count(kAttachmentMissingClearValues, 1);
                }
            } else if (load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
                count(kAttachmentLoadLoad, 1);
            } else {
                count(kAttachmentLoadDontCare, 1);
            }
            if (render_pass->attachments[index].stencilLoadOp ==
                    VK_ATTACHMENT_LOAD_OP_CLEAR &&
                index >= info->clearValueCount) {
                count(kAttachmentMissingClearValues, 1);
            }
            if (store_op == VK_ATTACHMENT_STORE_OP_STORE) {
                count(kAttachmentStoreStore, 1);
            } else {
                count(kAttachmentStoreDontCare, 1);
            }
            if (image_state && view_state) {
                record_transition(
                    command, image_state->handle, &view_state->range,
                    initial,
                    render_pass->attachment_use_layout_known[index]
                        ? render_pass->attachment_use_layouts[index]
                        : initial,
                    0, 0, 0, 0, sequence, false);
            }
            if (g_samples++ < kSampleLimit) {
                audit_log(
                    "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                    " type=attachment command_buffer=%p render_pass=%p"
                    " framebuffer=%p attachment=%u view=%p image=%p"
                    " view_live=%s tracked_layout=%d initial_layout=%d"
                    " final_layout=%d load_op=%d store_op=%d clear=%s"
                    " stencil_load_op=%d stencil_store_op=%d role_mask=0x%x"
                    " first_subpass=%u last_subpass=%u",
                    sequence, (void*)command_buffer, (void*)info->renderPass,
                    (void*)info->framebuffer, index, (void*)view,
                    image_state ? (void*)image_state->handle : NULL,
                    view_state && view_state->live ? "yes" : "no",
                    image_state && image_state->layout_known
                        ? image_state->layout : -1,
                    initial, final, load_op, store_op,
                    load_op == VK_ATTACHMENT_LOAD_OP_CLEAR &&
                            index < info->clearValueCount
                        ? "yes" : "no",
                    render_pass->attachments[index].stencilLoadOp,
                    render_pass->attachments[index].stencilStoreOp,
                    render_pass->attachment_roles[index],
                    render_pass->attachment_first_subpass[index],
                    render_pass->attachment_last_subpass[index]);
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_end_render_pass(
    VkCommandBuffer command_buffer) {
    pthread_mutex_lock(&g_lock);
    CommandBufferState* command =
        find_command_buffer(command_buffer, false);
    if (command && command->inside_render_pass) {
        RenderPassState* render_pass =
            find_render_pass(command->render_pass, false);
        FramebufferState* framebuffer =
            find_framebuffer(command->framebuffer, false);
        uint32_t attachment_count =
            render_pass && framebuffer
                ? render_pass->attachment_count < framebuffer->attachment_count
                    ? render_pass->attachment_count
                    : framebuffer->attachment_count
                : 0;
        for (uint32_t index = 0; index < attachment_count; ++index) {
            ViewState* view =
                find_view(framebuffer->attachments[index], false);
            ImageState* image = view ? find_image(view->image, false) : NULL;
            if (image && view) {
                record_transition(
                    command, image->handle, &view->range,
                    render_pass->attachment_use_layout_known[index]
                        ? render_pass->attachment_use_layouts[index]
                        : render_pass->attachments[index].initialLayout,
                    render_pass->attachments[index].finalLayout,
                    0, 0, 0, 0, next_sequence(), false);
            }
        }
        if (g_active && g_pass_tail_samples < kPassTailLimit) {
            ++g_pass_tail_samples;
            audit_log(
                "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                " type=render_pass_tail command_buffer=%p render_pass=%p"
                " framebuffer=%p pipeline=%p descriptor_bind_calls=%u"
                " last_descriptor_sequence=%" PRIu64 " first_set=%u"
                " set_count=%u",
                next_sequence(), (void*)command_buffer,
                (void*)command->render_pass, (void*)command->framebuffer,
                (void*)command->last_pipeline,
                command->descriptor_bind_calls,
                command->last_descriptor_sequence,
                command->last_first_set, command->last_set_count);
            for (uint32_t index = 0;
                 index < command->last_set_count &&
                 index < sizeof(command->last_sets) /
                             sizeof(command->last_sets[0]);
                 ++index) {
                DescriptorSetState* set =
                    find_set(command->last_sets[index], false);
                uint32_t known_slots = 0;
                uint32_t unknown_slots = 0;
                bool layout_known = false;
                descriptor_slot_coverage(
                    set, &known_slots, &unknown_slots, &layout_known);
                audit_log(
                    "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                    " type=render_pass_tail_set command_buffer=%p"
                    " set_index=%u set=%p live=%s stale_slots=%u"
                    " known_slots=%u unknown_slots=%u layout_known=%s"
                    " content=%s lifetime=%s content_hash=%016" PRIx64
                    " last_update_sequence=%" PRIu64,
                    g_sequence, (void*)command_buffer,
                    command->last_first_set + index,
                    (void*)command->last_sets[index],
                    set && set->live ? "yes" : "no",
                    set && set->live ? set->stale_slots : 0,
                    known_slots, unknown_slots,
                    layout_known ? "yes" : "no",
                    set && set->live && set->last_update_sequence != 0
                        ? (unknown_slots == 0 ? "known" : "partial")
                        : "unknown",
                    set && set->live &&
                            set->last_update_sequence != 0 &&
                            set->last_update_sequence <
                                g_audit_start_sequence
                        ? "pre-window"
                        : "window",
                    descriptor_content_hash(set),
                    set && set->live ? set->last_update_sequence : 0);
                if (set && set->live &&
                    (set->last_update_sequence <
                         g_audit_start_sequence ||
                     unknown_slots != 0)) {
                    sample_descriptor_slots(
                        command_buffer,
                        command->last_first_set + index, set);
                }
            }
        }
        command->inside_render_pass = false;
        command->render_pass = VK_NULL_HANDLE;
        command->framebuffer = VK_NULL_HANDLE;
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_bind_pipeline(
    VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
    VkPipeline pipeline) {
    if (bind_point != VK_PIPELINE_BIND_POINT_GRAPHICS) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    count(kPipelineBinds, 1);
    if (g_active) {
        next_sequence();
        PipelineState* state = find_pipeline(pipeline, false);
        CommandBufferState* command =
            find_command_buffer(command_buffer, false);
        if (command && command->inside_render_pass) {
            command->last_pipeline = pipeline;
        }
        if (state) {
            count(kPipelineBindsKnown, 1);
            if (state->created_during_audit) {
                count(kPipelineBindsCreatedDuringAudit, 1);
            }
            if (command && command->inside_render_pass) {
                if (state->render_pass == command->render_pass) {
                    count(kPipelineRenderPassExact, 1);
                } else {
                    count(kPipelineRenderPassDifferent, 1);
                }
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_bind_descriptor_sets(
    VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
    uint32_t first_set, uint32_t set_count,
    const VkDescriptorSet* sets) {
    (void)bind_point;
    (void)first_set;
    if (!sets) {
        return;
    }
    pthread_mutex_lock(&g_lock);
    count(kDescriptorSetBindCalls, 1);
    count(kDescriptorSetsBound, set_count);
    const uint64_t sequence = next_sequence();
    if (g_active) {
        CommandBufferState* command =
            find_command_buffer(command_buffer, false);
        if (command && command->inside_render_pass) {
            command->last_first_set = first_set;
            command->last_set_count =
                set_count < sizeof(command->last_sets) /
                                sizeof(command->last_sets[0])
                    ? set_count
                    : (uint32_t)(sizeof(command->last_sets) /
                                 sizeof(command->last_sets[0]));
            memcpy(
                command->last_sets, sets,
                command->last_set_count * sizeof(command->last_sets[0]));
            ++command->descriptor_bind_calls;
            command->last_descriptor_sequence = sequence;
        }
        for (uint32_t index = 0; index < set_count; ++index) {
            DescriptorSetState* set = find_set(sets[index], false);
            if (!set || !set->live) {
                count(kDescriptorUnknownSetsBound, 1);
                continue;
            }
            uint32_t known_slots = 0;
            uint32_t unknown_slots = 0;
            bool layout_known = false;
            descriptor_slot_coverage(
                set, &known_slots, &unknown_slots, &layout_known);
            count(kDescriptorKnownSlotsBound, known_slots);
            count(kDescriptorUnknownSlotsBound, unknown_slots);
            if (!layout_known) {
                count(kDescriptorUnknownLayoutSetsBound, 1);
            }
            if (set->last_update_sequence == 0) {
                count(kDescriptorZeroUpdateSetsBound, 1);
            }
            if (command && set->last_update_sequence >
                    command->max_bound_set_update_sequence) {
                command->max_bound_set_update_sequence =
                    set->last_update_sequence;
            }
            if (command) {
                uint32_t slot = command->bound_set_count;
                for (uint32_t bound = 0;
                     bound < command->bound_set_count; ++bound) {
                    if (command->bound_sets[bound] == sets[index]) {
                        slot = bound;
                        break;
                    }
                }
                if (slot == command->bound_set_count) {
                    if (slot < kMaxBoundSetsPerCommand) {
                        ++command->bound_set_count;
                    } else {
                        overflow("command_bound_descriptor_sets");
                        slot = kMaxBoundSetsPerCommand;
                    }
                }
                if (slot < kMaxBoundSetsPerCommand) {
                    command->bound_sets[slot] = sets[index];
                    command->bound_set_update_sequences[slot] =
                        set->last_update_sequence;
                }
            }
            if (set->stale_slots == 0) {
                continue;
            }
            count(kDescriptorStaleSetsBound, 1);
            count(kDescriptorStaleSlotsBound, set->stale_slots);
            if (g_anomalies++ < kAnomalyLimit) {
                audit_log(
                    "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                    " type=stale_descriptor_set_bound command_buffer=%p"
                    " set=%p stale_slots=%u last_update_sequence=%" PRIu64,
                    sequence, (void*)command_buffer, (void*)sets[index],
                    set->stale_slots, set->last_update_sequence);
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_pipeline_barrier(
    VkCommandBuffer command_buffer, VkPipelineStageFlags source_stage_mask,
    VkPipelineStageFlags destination_stage_mask,
    uint32_t image_barrier_count,
    const VkImageMemoryBarrier* image_barriers) {
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    count(kPipelineBarrierCalls, 1);
    count(kImageBarriers, image_barrier_count);
    CommandBufferState* command =
        find_command_buffer(command_buffer, false);
    if (image_barriers) {
        for (uint32_t index = 0; index < image_barrier_count; ++index) {
            const VkImageMemoryBarrier* barrier = &image_barriers[index];
            ImageState* image = find_image(barrier->image, false);
            if (image) {
                record_transition(
                    command, image->handle, &barrier->subresourceRange,
                    barrier->oldLayout, barrier->newLayout,
                    source_stage_mask, destination_stage_mask,
                    barrier->srcAccessMask, barrier->dstAccessMask,
                    sequence, true);
                if (g_active && g_barrier_samples++ < kBarrierSampleLimit) {
                    count(kBarrierStageAccessSamples, 1);
                    audit_log(
                        "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                        " type=subresource_barrier command_buffer=%p"
                        " image=%p aspect=0x%x base_mip=%u levels=%u"
                        " base_layer=%u layers=%u source_stage=0x%x"
                        " destination_stage=0x%x source_access=0x%x"
                        " destination_access=0x%x old_layout=%d new_layout=%d"
                        " execution=deferred-to-submit",
                        sequence, (void*)command_buffer,
                        (void*)barrier->image,
                        barrier->subresourceRange.aspectMask,
                        barrier->subresourceRange.baseMipLevel,
                        barrier->subresourceRange.levelCount,
                        barrier->subresourceRange.baseArrayLayer,
                        barrier->subresourceRange.layerCount,
                        source_stage_mask, destination_stage_mask,
                        barrier->srcAccessMask, barrier->dstAccessMask,
                        barrier->oldLayout, barrier->newLayout);
                }
            }
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_copy_image(void) {
    pthread_mutex_lock(&g_lock);
    count(kCopyImageCalls, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_blit_image(void) {
    pthread_mutex_lock(&g_lock);
    count(kBlitImageCalls, 1);
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_cmd_resolve_image(void) {
    pthread_mutex_lock(&g_lock);
    count(kResolveImageCalls, 1);
    pthread_mutex_unlock(&g_lock);
}
