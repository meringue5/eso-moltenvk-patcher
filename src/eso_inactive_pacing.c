#include "eso_inactive_pacing.h"

#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static volatile const uint8_t *g_active_flag;
static Teso4m4InactivePacingLogFunction g_logger;
static atomic_int g_last_state;
static atomic_uint g_transition_count;
static atomic_bool g_installed;

static void pacing_log(const char *format, ...) {
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

__attribute__((noinline)) static void inactive_pacing_hook(void) {
  if (!g_active_flag) {
    return;
  }
  const int state = *g_active_flag != 0 ? 1 : 0;
  int previous = atomic_load_explicit(&g_last_state, memory_order_relaxed);
  while (previous != state) {
    if (atomic_compare_exchange_weak_explicit(&g_last_state, &previous, state,
                                              memory_order_relaxed,
                                              memory_order_relaxed)) {
      const unsigned transition =
          atomic_fetch_add_explicit(&g_transition_count, 1,
                                    memory_order_relaxed) +
          1;
      if (transition <= 16) {
        pacing_log("INACTIVE_PACING_STATE: transition=%u active=%s action=%s",
                   transition, state ? "yes" : "no",
                   state ? "forward" : "sleep-bypassed");
      } else if (transition == 17) {
        pacing_log("INACTIVE_PACING_STATE_LIMIT: retained=16 "
                   "further_transitions=unlogged");
      }
      break;
    }
  }
}

void teso4m4_inactive_pacing_reset(void) {
  g_active_flag = NULL;
  g_logger = NULL;
  atomic_store(&g_last_state, -1);
  atomic_store(&g_transition_count, 0);
  atomic_store(&g_installed, false);
}

void teso4m4_inactive_pacing_set_logger(
    Teso4m4InactivePacingLogFunction logger) {
  g_logger = logger;
}

bool teso4m4_inactive_pacing_prepare(
    const struct mach_header_64 *header,
    const Teso4m4InactivePacingTarget *target,
    uint8_t patch[TESO4M4_INACTIVE_PACING_PATCH_SIZE]) {
  if (!header || !target || !patch || target->image_offset == 0 ||
      target->active_flag_offset == 0 || atomic_load(&g_installed)) {
    pacing_log("ERROR: inactive pacing patch arguments are invalid");
    return false;
  }

  uint32_t delay = 0;
  memcpy(&delay, target->expected + 3, sizeof(delay));
  if (target->expected[0] != 0x75 || target->expected[1] != 0x27 ||
      target->expected[2] != 0xbf || delay != 100000 ||
      target->expected[7] != 0xe8) {
    pacing_log("ERROR: inactive pacing profile is not the 100ms sleep branch");
    return false;
  }

  const uint8_t *address = (const uint8_t *)header + target->image_offset;
  if (memcmp(address, target->expected, sizeof(target->expected)) != 0) {
    pacing_log("ERROR: inactive pacing original bytes differ");
    return false;
  }

  memset(patch, 0, TESO4M4_INACTIVE_PACING_PATCH_SIZE);
  patch[0] = 0x48;
  patch[1] = 0xb8;
  const uint64_t hook = (uint64_t)(uintptr_t)&inactive_pacing_hook;
  memcpy(patch + 2, &hook, sizeof(hook));
  patch[10] = 0xff;
  patch[11] = 0xd0;

  g_active_flag = (volatile const uint8_t *)header + target->active_flag_offset;
  atomic_store(&g_last_state, -1);
  atomic_store(&g_transition_count, 0);
  pacing_log(
      "INACTIVE_PACING_READY: branch_offset=0x%llx active_flag_offset=0x%llx "
      "original_delay_us=100000 replacement=observe-and-return",
      (unsigned long long)target->image_offset,
      (unsigned long long)target->active_flag_offset);
  return true;
}

void teso4m4_inactive_pacing_did_install(void) {
  atomic_store(&g_installed, true);
  pacing_log("INACTIVE_PACING_ACTIVE: inactive_100ms_sleep=bypassed "
             "focus_event_propagation=unchanged transition_log_limit=16");
}
