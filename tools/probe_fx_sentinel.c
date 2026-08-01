#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "eso_fx_sentinel.h"

static bool g_window_open = true;
static unsigned g_log_lines;

static bool window_open(void) {
    return g_window_open;
}

static void test_log(const char* message) {
    if (strstr(message, "STARTUP_FX_SENTINEL") != NULL) {
        ++g_log_lines;
    }
}

static bool vector_equals(const uint8_t* bytes, size_t offset,
                          const uint32_t expected[4]) {
    return memcmp(bytes + offset, expected, 16) == 0;
}

static bool fail(const char* message) {
    fprintf(stderr, "FX sentinel smoke failed: %s\n", message);
    return false;
}

int main(void) {
    const uint32_t magenta_transparent[4] = {
        0x3f800000, 0, 0x3f800000, 0,
    };
    const uint32_t magenta_opaque[4] = {
        0x3f800000, 0, 0x3f800000, 0x3f800000,
    };
    const uint32_t black_transparent[4] = {0, 0, 0, 0};
    const uint32_t black_opaque[4] = {0, 0, 0, 0x3f800000};

    uint8_t material[0x80] = {0};
    memcpy(material + 0x10, magenta_transparent, 16);
    memcpy(material + 0x20, magenta_transparent, 16);
    memcpy(material + 0x30, magenta_opaque, 16);
    if (!teso4m4_fx_sentinel_neutralize_for_probe(material) ||
        !vector_equals(material, 0x10, black_transparent) ||
        !vector_equals(material, 0x20, black_transparent) ||
        !vector_equals(material, 0x30, black_opaque)) {
        return fail("exact magenta vectors were not neutralized") ? 0 : 1;
    }
    material[0x10] = 1;
    if (teso4m4_fx_sentinel_neutralize_for_probe(material)) {
        return fail("non-matching material was modified") ? 0 : 1;
    }

    const long page_size = sysconf(_SC_PAGESIZE);
    mach_vm_address_t page = 0;
    if (page_size <= 0 ||
        mach_vm_allocate(mach_task_self(), &page, (mach_vm_size_t)page_size,
                         VM_FLAGS_ANYWHERE) != KERN_SUCCESS) {
        return fail("could not allocate synthetic code page") ? 0 : 1;
    }
    const size_t initializer_offset = 0x100;
    const size_t constant_offset = 0x200;
    uint8_t* initializer = (uint8_t*)(uintptr_t)page + initializer_offset;
    const uint8_t entry[] = {
        0x55, 0x48, 0x89, 0xe5, 0x0f, 0x57, 0xc0, 0x0f, 0x11, 0x07,
        0x0f, 0x28, 0x0d, 0x00, 0x00, 0x00, 0x00,
        0x0f, 0x11, 0x4f, 0x10,
        0x0f, 0x11, 0x4f, 0x20,
        0x0f, 0x11, 0x4f, 0x30,
        0xc7, 0x47, 0x3c, 0x00, 0x00, 0x80, 0x3f,
        0x5d, 0xc3,
    };
    memcpy(initializer, entry, sizeof(entry));
    memcpy((void*)(uintptr_t)(page + constant_offset),
           magenta_transparent, 16);

    Teso4m4FxSentinelTarget target = {
        .image_offset = initializer_offset,
        .first_constant_offset = constant_offset,
    };
    memcpy(target.expected, entry, sizeof(target.expected));
    memcpy(target.first_constant_expected, magenta_transparent, 16);
    if (mach_vm_protect(
            mach_task_self(), page, (mach_vm_size_t)page_size, FALSE,
            VM_PROT_READ | VM_PROT_EXECUTE) != KERN_SUCCESS) {
        return fail("could not protect synthetic code page") ? 0 : 1;
    }

    teso4m4_fx_sentinel_reset();
    teso4m4_fx_sentinel_set_logger(&test_log);
    teso4m4_fx_sentinel_set_window_function(&window_open);
    if (!teso4m4_fx_sentinel_install(
            (const struct mach_header_64*)(uintptr_t)page, &target)) {
        return fail("synthetic trampoline install failed") ? 0 : 1;
    }

    typedef void (*Initializer)(void*);
    Initializer initialize = (Initializer)(void*)initializer;
    memset(material, 0xcc, sizeof(material));
    initialize(material);
    if (!vector_equals(material, 0x10, black_transparent) ||
        !vector_equals(material, 0x20, black_transparent) ||
        !vector_equals(material, 0x30, black_opaque)) {
        return fail("installed hook did not neutralize initializer output") ? 0 : 1;
    }

    g_window_open = false;
    memset(material, 0xcc, sizeof(material));
    initialize(material);
    if (!vector_equals(material, 0x10, magenta_transparent) ||
        !vector_equals(material, 0x20, magenta_transparent) ||
        !vector_equals(material, 0x30, magenta_opaque)) {
        return fail("hook modified output after bounded window") ? 0 : 1;
    }
    if (g_log_lines < 2) {
        return fail("install and match records were not logged") ? 0 : 1;
    }
    puts("FX sentinel smoke: PASS exact_match=1 bounded_window=1 trampoline=1");
    return 0;
}
