#define VK_USE_PLATFORM_MACOS_MVK 1
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan.h>

#include "mvk_compat.h"

static bool has_extension(const VkExtensionProperties* properties,
                          uint32_t count,
                          const char* name) {
    for (uint32_t index = 0; index < count; ++index) {
        if (strcmp(properties[index].extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

static void probe_compat_log(const char* message) {
    printf("compat %s\n", message);
}

static bool has_eso_hdr_pair(const VkSurfaceFormatKHR* formats,
                             uint32_t count) {
    for (uint32_t index = 0; index < count; ++index) {
        if (formats[index].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            formats[index].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    @autoreleasepool {
#ifdef TESO4M4_STATIC_MOLTENVK
        if (argc != 1) {
            fprintf(stderr, "usage: %s\n", argv[0]);
            return 2;
        }
        PFN_vkGetInstanceProcAddr get_proc = vkGetInstanceProcAddr;
#else
        if (argc != 2) {
            fprintf(stderr, "usage: %s libMoltenVK.dylib\n", argv[0]);
            return 2;
        }
        void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
        if (!library) {
            fprintf(stderr, "dlopen: %s\n", dlerror());
            return 1;
        }
        PFN_vkGetInstanceProcAddr get_proc =
            (PFN_vkGetInstanceProcAddr)dlsym(library,
                                             "vkGetInstanceProcAddr");
#endif
        if (!get_proc) {
            fprintf(stderr, "vkGetInstanceProcAddr is unavailable\n");
            return 1;
        }

        PFN_vkEnumerateInstanceExtensionProperties enumerate_extensions =
            (PFN_vkEnumerateInstanceExtensionProperties)get_proc(
                VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");
        PFN_vkCreateInstance create_instance =
            (PFN_vkCreateInstance)get_proc(VK_NULL_HANDLE, "vkCreateInstance");
        if (!enumerate_extensions || !create_instance) {
            fprintf(stderr, "required global Vulkan functions are unavailable\n");
            return 1;
        }

        uint32_t extension_count = 0;
        VkResult result =
            enumerate_extensions(NULL, &extension_count, NULL);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "instance extension count failed: %d\n", result);
            return 1;
        }
        VkExtensionProperties* extensions =
            calloc(extension_count, sizeof(*extensions));
        if (!extensions) {
            return 1;
        }
        result = enumerate_extensions(NULL, &extension_count, extensions);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "instance extension data failed: %d\n", result);
            free(extensions);
            return 1;
        }
        const bool has_surface = has_extension(
            extensions, extension_count, VK_KHR_SURFACE_EXTENSION_NAME);
        const bool has_macos_surface = has_extension(
            extensions, extension_count, VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
        printf("instance VK_KHR_surface=%s VK_MVK_macos_surface=%s\n",
               has_surface ? "yes" : "NO",
               has_macos_surface ? "yes" : "NO");
        free(extensions);
        if (!has_surface || !has_macos_surface) {
            return 1;
        }

        const char* enabled_extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
        };
        VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "teso4m4 surface-format probe",
            .apiVersion = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &application_info,
            .enabledExtensionCount = 2,
            .ppEnabledExtensionNames = enabled_extensions,
        };
        VkInstance instance = VK_NULL_HANDLE;
        result = create_instance(&instance_info, NULL, &instance);
        printf("vkCreateInstance=%d\n", result);
        if (result != VK_SUCCESS) {
            return 1;
        }

        PFN_vkEnumeratePhysicalDevices enumerate_devices =
            (PFN_vkEnumeratePhysicalDevices)get_proc(
                instance, "vkEnumeratePhysicalDevices");
        PFN_vkCreateMacOSSurfaceMVK create_surface =
            (PFN_vkCreateMacOSSurfaceMVK)get_proc(
                instance, "vkCreateMacOSSurfaceMVK");
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats =
            (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)get_proc(
                instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        PFN_vkDestroySurfaceKHR destroy_surface =
            (PFN_vkDestroySurfaceKHR)get_proc(instance, "vkDestroySurfaceKHR");
        PFN_vkDestroyInstance destroy_instance =
            (PFN_vkDestroyInstance)get_proc(instance, "vkDestroyInstance");
        if (!enumerate_devices || !create_surface || !get_surface_formats ||
            !destroy_surface || !destroy_instance) {
            fprintf(stderr, "required instance Vulkan functions are unavailable\n");
            return 1;
        }
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR raw_get_surface_formats =
            get_surface_formats;
        const bool use_hdr_filter =
            getenv("TESO4M4_PROBE_HDR_FILTER") != NULL;
        if (use_hdr_filter) {
            teso4m4_compat_reset();
            teso4m4_compat_set_logger(&probe_compat_log);
            teso4m4_compat_set_get_surface_formats(raw_get_surface_formats);
            get_surface_formats =
                &teso4m4_get_physical_device_surface_formats;
        }

        uint32_t device_count = 0;
        result = enumerate_devices(instance, &device_count, NULL);
        if (result != VK_SUCCESS || device_count == 0) {
            fprintf(stderr, "physical device count failed: %d count=%u\n",
                    result, device_count);
            destroy_instance(instance, NULL);
            return 1;
        }
        VkPhysicalDevice* devices = calloc(device_count, sizeof(*devices));
        if (!devices) {
            destroy_instance(instance, NULL);
            return 1;
        }
        result = enumerate_devices(instance, &device_count, devices);
        if (result != VK_SUCCESS || device_count == 0) {
            fprintf(stderr, "physical device data failed: %d count=%u\n",
                    result, device_count);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }

        NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 64, 64)];
        [view setWantsLayer:YES];
        [view setLayer:[CAMetalLayer layer]];
        VkMacOSSurfaceCreateInfoMVK surface_info = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .pView = (__bridge void*)view,
        };
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        result = create_surface(instance, &surface_info, NULL, &surface);
        printf("vkCreateMacOSSurfaceMVK=%d surface=%s\n", result,
               surface ? "non-null" : "NULL");
        if (result != VK_SUCCESS || !surface) {
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }

        uint32_t raw_format_count = 0;
        result = raw_get_surface_formats(
            devices[0], surface, &raw_format_count, NULL);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "raw surface format count failed: %d\n", result);
            destroy_surface(instance, surface, NULL);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }
        VkSurfaceFormatKHR* raw_formats =
            calloc(raw_format_count, sizeof(*raw_formats));
        if (!raw_formats) {
            destroy_surface(instance, surface, NULL);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }
        result = raw_get_surface_formats(
            devices[0], surface, &raw_format_count, raw_formats);
        const bool raw_eso_hdr_pair =
            result == VK_SUCCESS &&
            has_eso_hdr_pair(raw_formats, raw_format_count);
        printf("raw surface formats result=%d count=%u eso-hdr-pair=%s\n",
               result, raw_format_count, raw_eso_hdr_pair ? "yes" : "no");
        free(raw_formats);
        if (result != VK_SUCCESS) {
            destroy_surface(instance, surface, NULL);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }

        uint32_t format_count = 0;
        result = get_surface_formats(devices[0], surface, &format_count, NULL);
        printf("surface format count result=%d count=%u\n", result,
               format_count);
        if (result != VK_SUCCESS) {
            destroy_surface(instance, surface, NULL);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }
        VkSurfaceFormatKHR* formats = calloc(format_count, sizeof(*formats));
        if (!formats) {
            destroy_surface(instance, surface, NULL);
            free(devices);
            destroy_instance(instance, NULL);
            return 1;
        }
        result = get_surface_formats(
            devices[0], surface, &format_count, formats);
        bool eso_hdr_pair = false;
        for (uint32_t index = 0; index < format_count; ++index) {
            const bool is_eso_hdr_pair =
                formats[index].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                formats[index].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT;
            eso_hdr_pair |= is_eso_hdr_pair;
            printf("surface format index=%u format=%d colorSpace=%d%s\n",
                   index, formats[index].format, formats[index].colorSpace,
                   is_eso_hdr_pair ? " ESO_HDR_PAIR" : "");
        }
        printf("surface format data result=%d count=%u eso-hdr-pair=%s\n",
               result, format_count, eso_hdr_pair ? "yes" : "no");
        if (use_hdr_filter) {
            printf("surface filter validation: %s\n",
                   raw_eso_hdr_pair && !eso_hdr_pair ? "PASS" : "FAIL");
        }

        free(formats);
        destroy_surface(instance, surface, NULL);
        free(devices);
        destroy_instance(instance, NULL);
        return result == VK_SUCCESS &&
                       (!use_hdr_filter ||
                        (raw_eso_hdr_pair && !eso_hdr_pair))
                   ? 0
                   : 1;
    }
}
