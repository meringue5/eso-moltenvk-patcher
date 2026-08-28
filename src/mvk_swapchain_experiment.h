#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#define TESO4M4_SWAPCHAIN_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_SWAPCHAIN_HIDDEN
#endif

typedef void (*Teso4m4SwapchainLogFunction)(const char* message);

typedef enum {
    TESO4M4_SWAPCHAIN_EXPERIMENT_DISABLED = 0,
    TESO4M4_SWAPCHAIN_EXPERIMENT_CONTROL,
    TESO4M4_SWAPCHAIN_EXPERIMENT_TRIPLE_BUFFER,
} Teso4m4SwapchainExperimentMode;

TESO4M4_SWAPCHAIN_HIDDEN void teso4m4_swapchain_experiment_reset(void);
TESO4M4_SWAPCHAIN_HIDDEN void teso4m4_swapchain_experiment_configure(
    Teso4m4SwapchainExperimentMode mode,
    Teso4m4SwapchainLogFunction logger);
TESO4M4_SWAPCHAIN_HIDDEN PFN_vkVoidFunction
teso4m4_swapchain_experiment_intercept(
    const char* name,
    PFN_vkVoidFunction next_function);
TESO4M4_SWAPCHAIN_HIDDEN void teso4m4_swapchain_experiment_log_summary(void);
