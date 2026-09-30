#pragma once

#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__GNUC__)
#define TESO4M4_INACTIVE_PACING_HIDDEN __attribute__((visibility("hidden")))
#else
#define TESO4M4_INACTIVE_PACING_HIDDEN
#endif

enum {
  TESO4M4_INACTIVE_PACING_PATCH_SIZE = 12,
};

typedef struct {
  uint64_t image_offset;
  uint8_t expected[TESO4M4_INACTIVE_PACING_PATCH_SIZE];
  uint64_t active_flag_offset;
} Teso4m4InactivePacingTarget;

typedef void (*Teso4m4InactivePacingLogFunction)(const char *message);

TESO4M4_INACTIVE_PACING_HIDDEN void teso4m4_inactive_pacing_reset(void);
TESO4M4_INACTIVE_PACING_HIDDEN void
teso4m4_inactive_pacing_set_logger(Teso4m4InactivePacingLogFunction logger);
TESO4M4_INACTIVE_PACING_HIDDEN bool teso4m4_inactive_pacing_prepare(
    const struct mach_header_64 *header,
    const Teso4m4InactivePacingTarget *target,
    uint8_t patch[TESO4M4_INACTIVE_PACING_PATCH_SIZE]);
TESO4M4_INACTIVE_PACING_HIDDEN void teso4m4_inactive_pacing_did_install(void);
/* Observation only: records a bounded, timestamped external event together
   with ESO's active byte. It never changes ESO or AppKit state. */
TESO4M4_INACTIVE_PACING_HIDDEN void
teso4m4_inactive_pacing_note_event(const char *name, const char *front);
/* -1 before installation, otherwise ESO's active byte (0 or 1). */
TESO4M4_INACTIVE_PACING_HIDDEN int teso4m4_inactive_pacing_active_byte(void);
TESO4M4_INACTIVE_PACING_HIDDEN unsigned long long
teso4m4_inactive_pacing_elapsed_ms(void);
