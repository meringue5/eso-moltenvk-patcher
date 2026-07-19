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
         strcmp(argv[2], "descriptor-compat") != 0)) {
        fprintf(stderr, "usage: %s libMoltenVK.dylib default|descriptor-compat\n",
                argv[0]);
        return 2;
    }
    if (!clear_controlled_environment()) {
        return 1;
    }

    const bool descriptor_compat = strcmp(argv[2], "descriptor-compat") == 0;
    if (descriptor_compat &&
        (setenv("MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES", "1", 1) != 0 ||
         setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1) != 0)) {
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
        "synchronous_queue_submits=%u command_pooling=%u prefill=%d\n",
        argv[2], result, configuration_size,
        configuration.liveCheckAllResources,
        configuration.useMetalArgumentBuffers,
        configuration.useMTLHeap,
        configuration.synchronousQueueSubmits,
        configuration.useCommandPooling,
        configuration.prefillMetalCommandBuffers);

    const VkBool32 expected_live = descriptor_compat ? VK_TRUE : VK_FALSE;
    const VkBool32 expected_argument_buffers =
        descriptor_compat ? VK_FALSE : VK_TRUE;
    const bool passed =
        result == VK_SUCCESS && configuration_size == sizeof(configuration) &&
        configuration.liveCheckAllResources == expected_live &&
        configuration.useMetalArgumentBuffers == expected_argument_buffers &&
        configuration.useMTLHeap == MVK_CONFIG_USE_MTLHEAP_WHERE_SAFE &&
        configuration.synchronousQueueSubmits == VK_TRUE &&
        configuration.useCommandPooling == VK_TRUE &&
        configuration.prefillMetalCommandBuffers ==
            MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS_STYLE_NO_PREFILL;
    printf("MoltenVK configuration probe: %s\n", passed ? "PASS" : "FAIL");
    dlclose(library);
    return passed ? 0 : 1;
}
