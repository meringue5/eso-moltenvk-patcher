#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#define TESO4M4_RESET_TRACE_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_RESET_TRACE_HIDDEN
#endif

typedef void (*Teso4m4ResetTraceLogFunction)(const char* message);

TESO4M4_RESET_TRACE_HIDDEN void teso4m4_reset_trace_reset(void);
TESO4M4_RESET_TRACE_HIDDEN void teso4m4_reset_trace_set_logger(
    Teso4m4ResetTraceLogFunction logger);
TESO4M4_RESET_TRACE_HIDDEN void
teso4m4_reset_trace_set_pipeline_cache_bypass(bool enabled);
TESO4M4_RESET_TRACE_HIDDEN void
teso4m4_reset_trace_set_full_lifetime_audit(bool enabled);
TESO4M4_RESET_TRACE_HIDDEN PFN_vkVoidFunction
teso4m4_reset_trace_intercept(
    const char* name,
    PFN_vkVoidFunction next_function);
