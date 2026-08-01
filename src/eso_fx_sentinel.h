#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <mach-o/loader.h>

#if defined(__GNUC__)
#define TESO4M4_FX_SENTINEL_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_FX_SENTINEL_HIDDEN
#endif

enum {
    TESO4M4_FX_SENTINEL_PATCH_SIZE = 17,
    TESO4M4_FX_SENTINEL_CONSTANT_SIZE = 16,
    TESO4M4_FX_SENTINEL_CALLER_COUNT = 2,
};

typedef struct {
    uint64_t image_offset;
    uint8_t expected[TESO4M4_FX_SENTINEL_PATCH_SIZE];
    uint64_t first_constant_offset;
    uint8_t first_constant_expected[TESO4M4_FX_SENTINEL_CONSTANT_SIZE];
    uint64_t caller_return_offsets[TESO4M4_FX_SENTINEL_CALLER_COUNT];
} Teso4m4FxSentinelTarget;

typedef void (*Teso4m4FxSentinelLogFunction)(const char* message);
typedef bool (*Teso4m4FxSentinelWindowFunction)(void);

TESO4M4_FX_SENTINEL_HIDDEN void teso4m4_fx_sentinel_reset(void);
TESO4M4_FX_SENTINEL_HIDDEN void teso4m4_fx_sentinel_set_logger(
    Teso4m4FxSentinelLogFunction logger);
TESO4M4_FX_SENTINEL_HIDDEN void teso4m4_fx_sentinel_set_window_function(
    Teso4m4FxSentinelWindowFunction window_function);
TESO4M4_FX_SENTINEL_HIDDEN bool teso4m4_fx_sentinel_install(
    const struct mach_header_64* header,
    const Teso4m4FxSentinelTarget* target);
TESO4M4_FX_SENTINEL_HIDDEN bool teso4m4_fx_sentinel_neutralize_for_probe(
    void* material);
