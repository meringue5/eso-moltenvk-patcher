#include "eso_fx_sentinel.h"

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef void (*FxInitializerFunction)(void* material);

static const uint8_t kDisplacedPrefix[] = {
    0x55, 0x48, 0x89, 0xe5, 0x0f, 0x57, 0xc0, 0x0f, 0x11, 0x07,
};
static const uint32_t kMagentaTransparent[4] = {
    0x3f800000, 0x00000000, 0x3f800000, 0x00000000,
};
static const uint32_t kMagentaOpaque[4] = {
    0x3f800000, 0x00000000, 0x3f800000, 0x3f800000,
};
static const uint32_t kBlackTransparent[4] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static const uint32_t kBlackOpaque[4] = {
    0x00000000, 0x00000000, 0x00000000, 0x3f800000,
};

static const struct mach_header_64* g_header;
static Teso4m4FxSentinelTarget g_target;
static FxInitializerFunction g_original_initializer;
static Teso4m4FxSentinelLogFunction g_logger;
static Teso4m4FxSentinelWindowFunction g_window_function;
static atomic_uint_fast64_t g_call_count;
static atomic_uint_fast64_t g_match_count;

static void sentinel_log(const char* format, ...) {
    if (!g_logger) {
        return;
    }
    char message[512];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    g_logger(message);
}

static bool neutralize_material(void* material) {
    if (!material) {
        return false;
    }
    uint8_t* bytes = material;
    if (memcmp(bytes + 0x10, kMagentaTransparent,
               sizeof(kMagentaTransparent)) != 0 ||
        memcmp(bytes + 0x20, kMagentaTransparent,
               sizeof(kMagentaTransparent)) != 0 ||
        memcmp(bytes + 0x30, kMagentaOpaque, sizeof(kMagentaOpaque)) != 0) {
        return false;
    }
    memcpy(bytes + 0x10, kBlackTransparent, sizeof(kBlackTransparent));
    memcpy(bytes + 0x20, kBlackTransparent, sizeof(kBlackTransparent));
    memcpy(bytes + 0x30, kBlackOpaque, sizeof(kBlackOpaque));
    return true;
}

bool teso4m4_fx_sentinel_neutralize_for_probe(void* material) {
    return neutralize_material(material);
}

static void fx_initializer_hook(void* material) {
    g_original_initializer(material);
    if (!g_window_function || !g_window_function()) {
        return;
    }
    const uint64_t call = atomic_fetch_add(&g_call_count, 1) + 1;
    const bool matched = neutralize_material(material);
    const uint64_t match = matched
        ? atomic_fetch_add(&g_match_count, 1) + 1
        : atomic_load(&g_match_count);
    if (call <= 8) {
        const uintptr_t return_address =
            (uintptr_t)__builtin_return_address(0);
        const uint64_t return_offset =
            g_header && return_address >= (uintptr_t)g_header
                ? return_address - (uintptr_t)g_header
                : UINT64_MAX;
        const char* caller = "other";
        if (return_offset == g_target.caller_return_offsets[0]) {
            caller = "fx-material";
        } else if (return_offset == g_target.caller_return_offsets[1]) {
            caller = "fx-material-transparent";
        }
        sentinel_log(
            "STARTUP_FX_SENTINEL: call=%llu match=%s match_count=%llu "
            "caller=%s return_offset=0x%llx",
            (unsigned long long)call, matched ? "yes" : "no",
            (unsigned long long)match, caller,
            (unsigned long long)return_offset);
    } else if (call == 9) {
        sentinel_log(
            "STARTUP_FX_SENTINEL_DETAIL_CAP: logged=8 further_calls=unlogged");
    }
}

void teso4m4_fx_sentinel_reset(void) {
    g_header = NULL;
    memset(&g_target, 0, sizeof(g_target));
    g_original_initializer = NULL;
    g_logger = NULL;
    g_window_function = NULL;
    atomic_store(&g_call_count, 0);
    atomic_store(&g_match_count, 0);
}

void teso4m4_fx_sentinel_set_logger(
    Teso4m4FxSentinelLogFunction logger) {
    g_logger = logger;
}

void teso4m4_fx_sentinel_set_window_function(
    Teso4m4FxSentinelWindowFunction window_function) {
    g_window_function = window_function;
}

static bool protect_rx(mach_vm_address_t address, mach_vm_size_t size) {
    const kern_return_t result = mach_vm_protect(
        mach_task_self(), address, size, FALSE,
        VM_PROT_READ | VM_PROT_EXECUTE);
    if (result != KERN_SUCCESS) {
        sentinel_log("ERROR: FX sentinel could not set RX protection: %s",
                     mach_error_string(result));
        return false;
    }
    return true;
}

bool teso4m4_fx_sentinel_install(
    const struct mach_header_64* header,
    const Teso4m4FxSentinelTarget* target) {
    if (!header || !target || g_original_initializer) {
        sentinel_log("ERROR: FX sentinel install arguments are invalid");
        return false;
    }
    uint8_t* address = (uint8_t*)header + target->image_offset;
    const uint8_t* constant =
        (const uint8_t*)header + target->first_constant_offset;
    if (memcmp(target->expected, kDisplacedPrefix,
               sizeof(kDisplacedPrefix)) != 0 ||
        memcmp(address, target->expected, sizeof(target->expected)) != 0) {
        sentinel_log("ERROR: FX sentinel initializer bytes differ");
        return false;
    }
    if (memcmp(constant, target->first_constant_expected,
               sizeof(target->first_constant_expected)) != 0) {
        sentinel_log("ERROR: FX sentinel constant bytes differ");
        return false;
    }

    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        sentinel_log("ERROR: FX sentinel page size is unavailable");
        return false;
    }
    const uintptr_t patch_page =
        (uintptr_t)address & ~((uintptr_t)page_size - 1);
    if ((uintptr_t)address + sizeof(target->expected) >
        patch_page + (uintptr_t)page_size) {
        sentinel_log("ERROR: FX sentinel patch crosses a code page");
        return false;
    }

    mach_vm_address_t trampoline = 0;
    kern_return_t result = mach_vm_allocate(
        mach_task_self(), &trampoline, (mach_vm_size_t)page_size,
        VM_FLAGS_ANYWHERE);
    if (result != KERN_SUCCESS) {
        sentinel_log("ERROR: FX sentinel trampoline allocation failed: %s",
                     mach_error_string(result));
        return false;
    }
    uint8_t code[35] = {0};
    size_t cursor = 0;
    memcpy(code + cursor, target->expected, sizeof(kDisplacedPrefix));
    cursor += sizeof(kDisplacedPrefix);
    code[cursor++] = 0x48;
    code[cursor++] = 0xb8;
    const uint64_t constant_address = (uint64_t)(uintptr_t)constant;
    memcpy(code + cursor, &constant_address, sizeof(constant_address));
    cursor += sizeof(constant_address);
    code[cursor++] = 0x0f;
    code[cursor++] = 0x28;
    code[cursor++] = 0x08;
    code[cursor++] = 0x48;
    code[cursor++] = 0xb8;
    const uint64_t continuation =
        (uint64_t)(uintptr_t)(address + sizeof(target->expected));
    memcpy(code + cursor, &continuation, sizeof(continuation));
    cursor += sizeof(continuation);
    code[cursor++] = 0xff;
    code[cursor++] = 0xe0;
    if (cursor != sizeof(code)) {
        mach_vm_deallocate(
            mach_task_self(), trampoline, (mach_vm_size_t)page_size);
        sentinel_log("ERROR: FX sentinel trampoline size is invalid");
        return false;
    }
    memcpy((void*)(uintptr_t)trampoline, code, sizeof(code));
    __builtin___clear_cache(
        (char*)(uintptr_t)trampoline,
        (char*)(uintptr_t)trampoline + sizeof(code));
    if (!protect_rx(trampoline, (mach_vm_size_t)page_size)) {
        mach_vm_deallocate(
            mach_task_self(), trampoline, (mach_vm_size_t)page_size);
        return false;
    }

    result = mach_vm_protect(
        mach_task_self(), patch_page, (mach_vm_size_t)page_size, FALSE,
        VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (result != KERN_SUCCESS) {
        sentinel_log("ERROR: FX sentinel code page is not writable: %s",
                     mach_error_string(result));
        mach_vm_deallocate(
            mach_task_self(), trampoline, (mach_vm_size_t)page_size);
        return false;
    }
    uint8_t patch[TESO4M4_FX_SENTINEL_PATCH_SIZE];
    memset(patch, 0x90, sizeof(patch));
    patch[0] = 0x48;
    patch[1] = 0xb8;
    const uint64_t hook_address =
        (uint64_t)(uintptr_t)&fx_initializer_hook;
    memcpy(patch + 2, &hook_address, sizeof(hook_address));
    patch[10] = 0xff;
    patch[11] = 0xe0;
    memcpy(address, patch, sizeof(patch));
    __builtin___clear_cache((char*)address, (char*)address + sizeof(patch));
    if (!protect_rx(patch_page, (mach_vm_size_t)page_size)) {
        sentinel_log(
            "FATAL: FX sentinel code-page permissions were not restored; exiting");
        _exit(126);
    }

    g_header = header;
    g_target = *target;
    g_original_initializer = (FxInitializerFunction)(uintptr_t)trampoline;
    sentinel_log(
        "STARTUP_FX_SENTINEL_BEGIN: initializer_offset=0x%llx "
        "window=generation-2-present-180 vectors=0x10,0x20,0x30 "
        "replacement=black-preserve-alpha",
        (unsigned long long)target->image_offset);
    return true;
}
