#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <MoltenVK/mvk_private_api.h>

static bool clear_controlled_environment(void) {
    const char* names[] = {
        "MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES",
        "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS",
        "MVK_CONFIG_USE_MTLHEAP",
        "MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS",
        "MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION",
        "MVK_CONFIG_USE_COMMAND_POOLING",
        "MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS",
    };
    for (size_t index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (unsetenv(names[index]) != 0) {
            perror("unsetenv");
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 3 ||
        (strcmp(argv[2], "default") != 0 &&
         strcmp(argv[2], "descriptor-compat") != 0 &&
         strcmp(argv[2], "legacy-allocation") != 0 &&
         strcmp(argv[2], "reset-resource-trace") != 0 &&
         strcmp(argv[2], "no-command-pooling") != 0 &&
         strcmp(argv[2], "render-audit") != 0 &&
         strcmp(argv[2], "reset-no-pipeline-cache") != 0 &&
         strcmp(argv[2], "full-lifetime-audit") != 0 &&
         strcmp(argv[2], "performance-safe") != 0 &&
         strcmp(argv[2], "performance-aggressive") != 0 &&
         strcmp(argv[2], "startup-color-audit") != 0 &&
         strcmp(argv[2], "startup-fx-neutralize") != 0 &&
         strcmp(argv[2], "startup-present-pixel-audit") != 0 &&
         strcmp(argv[2], "startup-draw-audit") != 0 &&
         strcmp(argv[2], "startup-input-audit") != 0 &&
         strcmp(argv[2], "startup-compositor-audit") != 0 &&
         strcmp(argv[2], "startup-compositor-neutralize") != 0 &&
         strcmp(argv[2], "startup-pipeline-timing-control") != 0 &&
         strcmp(argv[2], "startup-inactive-pacing-bypass") != 0)) {
        fprintf(
            stderr,
            "usage: %s libMoltenVK.dylib "
            "default|descriptor-compat|legacy-allocation|reset-resource-trace|"
            "no-command-pooling|render-audit|reset-no-pipeline-cache|"
            "full-lifetime-audit|"
            "performance-safe|performance-aggressive|startup-color-audit|"
            "startup-fx-neutralize|startup-present-pixel-audit|"
            "startup-draw-audit|startup-input-audit|"
            "startup-compositor-audit|startup-compositor-neutralize|"
            "startup-pipeline-timing-control|"
            "startup-inactive-pacing-bypass\n",
            argv[0]);
        return 2;
    }
    if (!clear_controlled_environment()) {
        return 1;
    }

    const bool legacy_allocation =
        strcmp(argv[2], "legacy-allocation") == 0;
    const bool descriptor_compat =
        strcmp(argv[2], "descriptor-compat") == 0 ||
        strcmp(argv[2], "reset-resource-trace") == 0 ||
        strcmp(argv[2], "no-command-pooling") == 0 ||
        strcmp(argv[2], "render-audit") == 0 ||
        strcmp(argv[2], "reset-no-pipeline-cache") == 0 ||
        strcmp(argv[2], "full-lifetime-audit") == 0 ||
        strcmp(argv[2], "performance-safe") == 0 ||
        strcmp(argv[2], "performance-aggressive") == 0 ||
        strcmp(argv[2], "startup-color-audit") == 0 ||
        strcmp(argv[2], "startup-fx-neutralize") == 0 ||
        strcmp(argv[2], "startup-present-pixel-audit") == 0 ||
        strcmp(argv[2], "startup-draw-audit") == 0 ||
        strcmp(argv[2], "startup-input-audit") == 0 ||
        strcmp(argv[2], "startup-compositor-audit") == 0 ||
        strcmp(argv[2], "startup-compositor-neutralize") == 0 ||
        strcmp(argv[2], "startup-pipeline-timing-control") == 0 ||
        strcmp(argv[2], "startup-inactive-pacing-bypass") == 0 ||
        legacy_allocation;
    const bool no_command_pooling =
        strcmp(argv[2], "no-command-pooling") == 0;
    const bool performance_safe =
        strcmp(argv[2], "performance-safe") == 0;
    const bool performance_aggressive =
        strcmp(argv[2], "performance-aggressive") == 0 ||
        strcmp(argv[2], "startup-color-audit") == 0 ||
        strcmp(argv[2], "startup-fx-neutralize") == 0 ||
        strcmp(argv[2], "startup-present-pixel-audit") == 0 ||
        strcmp(argv[2], "startup-draw-audit") == 0 ||
        strcmp(argv[2], "startup-input-audit") == 0 ||
        strcmp(argv[2], "startup-compositor-audit") == 0 ||
        strcmp(argv[2], "startup-compositor-neutralize") == 0 ||
        strcmp(argv[2], "startup-pipeline-timing-control") == 0 ||
        strcmp(argv[2], "startup-inactive-pacing-bypass") == 0;
    const bool performance_mode =
        performance_safe || performance_aggressive;
    const bool nonmaximized_compilation =
        strcmp(argv[2], "startup-compositor-neutralize") == 0 ||
        strcmp(argv[2], "startup-pipeline-timing-control") == 0 ||
        strcmp(argv[2], "startup-inactive-pacing-bypass") == 0;
    if (descriptor_compat &&
        (setenv(
             "MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES",
             performance_aggressive ? "0" : "1", 1) != 0 ||
         setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1) != 0 ||
         (legacy_allocation &&
          setenv("MVK_CONFIG_USE_MTLHEAP", "0", 1) != 0) ||
         (no_command_pooling &&
          setenv("MVK_CONFIG_USE_COMMAND_POOLING", "0", 1) != 0) ||
         (performance_mode &&
          (setenv("MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS", "0", 1) != 0 ||
           setenv(
               "MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION",
               nonmaximized_compilation ? "0" : "1", 1) != 0)))) {
        perror("setenv");
        return 1;
    }

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    PFN_vkGetMoltenVKConfigurationMVK get_configuration =
        (PFN_vkGetMoltenVKConfigurationMVK)dlsym(
            library, "vkGetMoltenVKConfigurationMVK");
    if (!get_configuration) {
        fprintf(stderr, "missing vkGetMoltenVKConfigurationMVK\n");
        dlclose(library);
        return 1;
    }

    MVKConfiguration configuration = {0};
    size_t configuration_size = sizeof(configuration);
    VkResult result = get_configuration(
        VK_NULL_HANDLE, &configuration, &configuration_size);
    printf(
        "mode=%s result=%d size=%zu live_resources=%u "
        "metal_argument_buffers=%u use_mtlheap=%d "
        "synchronous_queue_submits=%u command_pooling=%u prefill=%d "
        "maximize_concurrent_compilation=%u\n",
        argv[2], result, configuration_size,
        configuration.liveCheckAllResources,
        configuration.useMetalArgumentBuffers,
        configuration.useMTLHeap,
        configuration.synchronousQueueSubmits,
        configuration.useCommandPooling,
        configuration.prefillMetalCommandBuffers,
        configuration.shouldMaximizeConcurrentCompilation);

    const VkBool32 expected_live =
        descriptor_compat && !performance_aggressive
            ? VK_TRUE
            : VK_FALSE;
    const VkBool32 expected_argument_buffers =
        descriptor_compat ? VK_FALSE : VK_TRUE;
    const MVKConfigUseMTLHeap expected_mtlheap =
        legacy_allocation
            ? MVK_CONFIG_USE_MTLHEAP_NEVER
            : MVK_CONFIG_USE_MTLHEAP_WHERE_SAFE;
    const bool passed =
        result == VK_SUCCESS && configuration_size == sizeof(configuration) &&
        configuration.liveCheckAllResources == expected_live &&
        configuration.useMetalArgumentBuffers == expected_argument_buffers &&
        configuration.useMTLHeap == expected_mtlheap &&
        configuration.synchronousQueueSubmits ==
            (performance_mode ? VK_FALSE : VK_TRUE) &&
        configuration.useCommandPooling ==
            (no_command_pooling ? VK_FALSE : VK_TRUE) &&
        configuration.prefillMetalCommandBuffers ==
            MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS_STYLE_NO_PREFILL &&
        configuration.shouldMaximizeConcurrentCompilation ==
            (performance_mode && !nonmaximized_compilation
                 ? VK_TRUE
                 : VK_FALSE);
    printf("MoltenVK configuration probe: %s\n", passed ? "PASS" : "FAIL");
    dlclose(library);
    return passed ? 0 : 1;
}
