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
    kMaxDescriptorSets = 16384,
    kMaxDescriptorSlots = 131072,
    kMaxRenderPasses = 2048,
    kMaxFramebuffers = 4096,
    kMaxPipelines = 16384,
    kMaxCommandBuffers = 512,
    kMaxAttachments = 12,
    kSampleLimit = 64,
    kPassTailLimit = 128,
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
    kDescriptorSetBindCalls,
    kDescriptorSetsBound,
    kDescriptorUnknownSetsBound,
    kDescriptorStaleSetsBound,
    kDescriptorStaleSlotsBound,
    kImageBinds,
    kImageDeadRangeReuses,
    kImageLiveOverlaps,
    kImageUndeclaredOverlaps,
    kPipelineBarrierCalls,
    kImageBarriers,
    kLayoutMismatches,
    kRenderPassBegins,
    kAttachmentSamples,
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
    [kDescriptorSetBindCalls] = "descriptor_set_bind_calls",
    [kDescriptorSetsBound] = "descriptor_sets_bound",
    [kDescriptorUnknownSetsBound] = "descriptor_unknown_sets_bound",
    [kDescriptorStaleSetsBound] = "descriptor_stale_sets_bound",
    [kDescriptorStaleSlotsBound] = "descriptor_stale_slots_bound",
    [kImageBinds] = "image_binds",
    [kImageDeadRangeReuses] = "image_dead_range_reuses",
    [kImageLiveOverlaps] = "image_live_overlaps",
    [kImageUndeclaredOverlaps] = "image_undeclared_overlaps",
    [kPipelineBarrierCalls] = "pipeline_barrier_calls",
    [kImageBarriers] = "image_barriers",
    [kLayoutMismatches] = "layout_mismatches",
    [kRenderPassBegins] = "render_pass_begins",
    [kAttachmentSamples] = "attachment_samples",
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
    VkImageLayout layout;
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
} ViewState;

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
    VkDescriptorSet set;
    uint32_t binding;
    uint32_t element;
    VkDescriptorType type;
    VkImageView view;
} DescriptorSlotState;

typedef struct {
    bool occupied;
    bool live;
    VkRenderPass handle;
    uint32_t attachment_count;
    VkAttachmentDescription attachments[kMaxAttachments];
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
    bool occupied;
    bool inside_render_pass;
    VkCommandBuffer handle;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    VkPipeline last_pipeline;
    uint32_t last_first_set;
    uint32_t last_set_count;
    VkDescriptorSet last_sets[4];
    uint32_t descriptor_bind_calls;
    uint64_t last_descriptor_sequence;
} CommandBufferState;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static Teso4m4RenderAuditLogFunction g_logger;
static bool g_mirror_enabled;
static bool g_active;
static bool g_complete;
static uint64_t g_sequence;
static uint64_t g_mirror_overflows;
static uint64_t g_counters[kAuditCounterCount];
static uint32_t g_samples;
static uint32_t g_pass_tail_samples;
static uint32_t g_anomalies;
static ImageState g_images[kMaxImages];
static ViewState g_views[kMaxViews];
static DescriptorSetState g_sets[kMaxDescriptorSets];
static DescriptorSlotState g_slots[kMaxDescriptorSlots];
static RenderPassState g_render_passes[kMaxRenderPasses];
static FramebufferState g_framebuffers[kMaxFramebuffers];
static PipelineState g_pipelines[kMaxPipelines];
static CommandBufferState g_command_buffers[kMaxCommandBuffers];

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

static bool image_descriptor_type(VkDescriptorType type) {
    return type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
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
    VkDescriptorType type, VkImageView view, uint64_t sequence) {
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
    slot->type = type;
    slot->view = view;
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
    set->last_update_sequence = sequence;
}

void teso4m4_render_audit_reset(void) {
    pthread_mutex_lock(&g_lock);
    g_logger = NULL;
    g_mirror_enabled = false;
    g_active = false;
    g_complete = false;
    g_sequence = 0;
    g_mirror_overflows = 0;
    g_samples = 0;
    g_pass_tail_samples = 0;
    g_anomalies = 0;
    memset(g_counters, 0, sizeof(g_counters));
    memset(g_images, 0, sizeof(g_images));
    memset(g_views, 0, sizeof(g_views));
    memset(g_sets, 0, sizeof(g_sets));
    memset(g_slots, 0, sizeof(g_slots));
    memset(g_render_passes, 0, sizeof(g_render_passes));
    memset(g_framebuffers, 0, sizeof(g_framebuffers));
    memset(g_pipelines, 0, sizeof(g_pipelines));
    memset(g_command_buffers, 0, sizeof(g_command_buffers));
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
    g_mirror_enabled = true;
    pthread_mutex_unlock(&g_lock);
}

void teso4m4_render_audit_begin(void) {
    pthread_mutex_lock(&g_lock);
    if (!g_active && !g_complete) {
        memset(g_counters, 0, sizeof(g_counters));
        g_counters[kStateOverflows] = g_mirror_overflows;
        g_samples = 0;
        g_pass_tail_samples = 0;
        g_anomalies = 0;
        g_active = true;
        audit_log(
            "RENDER_AUDIT_BEGIN: mirror=%s sample_limit=%u anomaly_limit=%u"
            " pass_tail_limit=%u slot_capacity=%u",
            g_mirror_enabled ? "enabled" : "disabled", kSampleLimit,
            kAnomalyLimit, kPassTailLimit, kMaxDescriptorSlots);
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
            " pass_tails=%u anomalies=%u",
            reason ? reason : "unknown", g_samples, g_pass_tail_samples,
            g_anomalies);
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
        state->layout = info->initialLayout;
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
        if (!image_descriptor_type(write->descriptorType) ||
            !write->pImageInfo) {
            continue;
        }
        count(kDescriptorImageWrites, write->descriptorCount);
        if (write->descriptorCount > 1) {
            count(kDescriptorMultiWrites, 1);
        }
        for (uint32_t element = 0; element < write->descriptorCount;
             ++element) {
            write_slot(
                write->dstSet, write->dstBinding,
                write->dstArrayElement + element, write->descriptorType,
                write->pImageInfo[element].imageView, sequence);
        }
    }
    for (uint32_t copy_index = 0; copy_index < copy_count; ++copy_index) {
        const VkCopyDescriptorSet* copy = &copies[copy_index];
        count(kDescriptorCopies, copy->descriptorCount);
        for (uint32_t element = 0; element < copy->descriptorCount;
             ++element) {
            DescriptorSlotState* source = find_slot(
                copy->srcSet, copy->srcBinding,
                copy->srcArrayElement + element, false);
            if (source && source->valid) {
                write_slot(
                    copy->dstSet, copy->dstBinding,
                    copy->dstArrayElement + element, source->type,
                    source->view, sequence);
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
        command->last_pipeline = VK_NULL_HANDLE;
        command->last_first_set = 0;
        command->last_set_count = 0;
        memset(command->last_sets, 0, sizeof(command->last_sets));
        command->descriptor_bind_calls = 0;
        command->last_descriptor_sequence = 0;
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
            if (image_state && image_state->layout_known &&
                initial != VK_IMAGE_LAYOUT_UNDEFINED &&
                image_state->layout != initial) {
                count(kLayoutMismatches, 1);
                if (g_anomalies++ < kAnomalyLimit) {
                    audit_log(
                        "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                        " type=render_pass_initial_layout_mismatch"
                        " command_buffer=%p render_pass=%p framebuffer=%p"
                        " attachment=%u image=%p tracked=%d declared=%d",
                        sequence, (void*)command_buffer,
                        (void*)info->renderPass, (void*)info->framebuffer,
                        index, (void*)image_state->handle,
                        image_state->layout, initial);
                }
            }
            if (g_samples++ < kSampleLimit) {
                audit_log(
                    "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                    " type=attachment command_buffer=%p render_pass=%p"
                    " framebuffer=%p attachment=%u view=%p image=%p"
                    " view_live=%s tracked_layout=%d initial_layout=%d"
                    " final_layout=%d",
                    sequence, (void*)command_buffer, (void*)info->renderPass,
                    (void*)info->framebuffer, index, (void*)view,
                    image_state ? (void*)image_state->handle : NULL,
                    view_state && view_state->live ? "yes" : "no",
                    image_state && image_state->layout_known
                        ? image_state->layout : -1,
                    initial, final);
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
            if (image) {
                image->layout =
                    render_pass->attachments[index].finalLayout;
                image->layout_known = true;
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
                audit_log(
                    "RENDER_AUDIT_SAMPLE: sequence=%" PRIu64
                    " type=render_pass_tail_set command_buffer=%p"
                    " set_index=%u set=%p live=%s stale_slots=%u"
                    " last_update_sequence=%" PRIu64,
                    g_sequence, (void*)command_buffer,
                    command->last_first_set + index,
                    (void*)command->last_sets[index],
                    set && set->live ? "yes" : "no",
                    set && set->live ? set->stale_slots : 0,
                    set && set->live ? set->last_update_sequence : 0);
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
    VkCommandBuffer command_buffer, uint32_t image_barrier_count,
    const VkImageMemoryBarrier* image_barriers) {
    (void)command_buffer;
    pthread_mutex_lock(&g_lock);
    const uint64_t sequence = next_sequence();
    count(kPipelineBarrierCalls, 1);
    count(kImageBarriers, image_barrier_count);
    if (image_barriers) {
        for (uint32_t index = 0; index < image_barrier_count; ++index) {
            const VkImageMemoryBarrier* barrier = &image_barriers[index];
            ImageState* image = find_image(barrier->image, false);
            if (g_active && image && image->layout_known &&
                barrier->oldLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
                image->layout != barrier->oldLayout) {
                count(kLayoutMismatches, 1);
                if (g_anomalies++ < kAnomalyLimit) {
                    audit_log(
                        "RENDER_AUDIT_ANOMALY: sequence=%" PRIu64
                        " type=barrier_old_layout_mismatch image=%p"
                        " tracked=%d declared_old=%d declared_new=%d",
                        sequence, (void*)barrier->image, image->layout,
                        barrier->oldLayout, barrier->newLayout);
                }
            }
            if (image) {
                image->layout = barrier->newLayout;
                image->layout_known = true;
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
