#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "eso_inactive_pacing.h"

static unsigned g_ready_logs;
static unsigned g_active_logs;
static unsigned g_false_logs;
static unsigned g_true_logs;
static unsigned g_error_logs;

static void test_log(const char *message) {
  g_ready_logs += strstr(message, "INACTIVE_PACING_READY:") != NULL;
  g_active_logs += strstr(message, "INACTIVE_PACING_ACTIVE:") != NULL;
  g_false_logs += strstr(message, "active=no action=sleep-bypassed") != NULL;
  g_true_logs += strstr(message, "active=yes action=forward") != NULL;
  g_error_logs += strstr(message, "ERROR:") != NULL;
}

static bool fail(const char *message) {
  fprintf(stderr, "Inactive pacing smoke failed: %s\n", message);
  return false;
}

int main(void) {
  const long page_size = sysconf(_SC_PAGESIZE);
  mach_vm_address_t allocation = 0;
  if (page_size <= 0 || mach_vm_allocate(mach_task_self(), &allocation,
                                         (mach_vm_size_t)page_size * 2,
                                         VM_FLAGS_ANYWHERE) != KERN_SUCCESS) {
    return fail("could not allocate synthetic code and state pages") ? 0 : 1;
  }

  const size_t function_offset = 0x100;
  const size_t patch_offset = function_offset + 4;
  const size_t active_flag_offset = (size_t)page_size + 0x20;
  const uint8_t original_patch[TESO4M4_INACTIVE_PACING_PATCH_SIZE] = {
      0x75, 0x27, 0xbf, 0xa0, 0x86, 0x01, 0x00, 0xe8, 0x8a, 0x78, 0x8c, 0x03,
  };
  const uint8_t prologue[] = {0x55, 0x48, 0x89, 0xe5};
  const uint8_t continuation[] = {
      0xb8, 0x07, 0x00, 0x00, 0x00, 0x5d, 0xc3,
  };
  uint8_t *const code = (uint8_t *)(uintptr_t)allocation;
  volatile uint8_t *const active_flag =
      (volatile uint8_t *)(uintptr_t)(allocation + active_flag_offset);
  memcpy(code + function_offset, prologue, sizeof(prologue));
  memcpy(code + patch_offset, original_patch, sizeof(original_patch));
  memcpy(code + patch_offset + sizeof(original_patch), continuation,
         sizeof(continuation));
  code[patch_offset + sizeof(original_patch) - 1] ^= 0x01;
  *active_flag = 0;

  if (mach_vm_protect(mach_task_self(), allocation, (mach_vm_size_t)page_size,
                      FALSE, VM_PROT_READ | VM_PROT_EXECUTE) != KERN_SUCCESS) {
    return fail("could not protect synthetic code page") ? 0 : 1;
  }

  Teso4m4InactivePacingTarget target = {
      .image_offset = patch_offset,
      .active_flag_offset = active_flag_offset,
  };
  memcpy(target.expected, original_patch, sizeof(original_patch));
  uint8_t patch[TESO4M4_INACTIVE_PACING_PATCH_SIZE] = {0};
  teso4m4_inactive_pacing_reset();
  teso4m4_inactive_pacing_set_logger(&test_log);
  if (teso4m4_inactive_pacing_prepare(
          (const struct mach_header_64 *)(uintptr_t)allocation, &target,
          patch)) {
    return fail("mismatched original bytes were accepted") ? 0 : 1;
  }

  kern_return_t result =
      mach_vm_protect(mach_task_self(), allocation, (mach_vm_size_t)page_size,
                      FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
  if (result != KERN_SUCCESS) {
    fprintf(stderr, "mach_vm_protect RW+COW: %s\n", mach_error_string(result));
    return 1;
  }
  memcpy(code + patch_offset, original_patch, sizeof(original_patch));
  result =
      mach_vm_protect(mach_task_self(), allocation, (mach_vm_size_t)page_size,
                      FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
  if (result != KERN_SUCCESS ||
      !teso4m4_inactive_pacing_prepare(
          (const struct mach_header_64 *)(uintptr_t)allocation, &target,
          patch)) {
    return fail("exact synthetic target was rejected") ? 0 : 1;
  }
  if (patch[0] != 0x48 || patch[1] != 0xb8 || patch[10] != 0xff ||
      patch[11] != 0xd0) {
    return fail("prepared patch is not an absolute call") ? 0 : 1;
  }

  result =
      mach_vm_protect(mach_task_self(), allocation, (mach_vm_size_t)page_size,
                      FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
  if (result != KERN_SUCCESS) {
    return fail("synthetic code page was not writable") ? 0 : 1;
  }
  memcpy(code + patch_offset, patch, sizeof(patch));
  __builtin___clear_cache((char *)code + patch_offset,
                          (char *)code + patch_offset + sizeof(patch));
  result =
      mach_vm_protect(mach_task_self(), allocation, (mach_vm_size_t)page_size,
                      FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
  if (result != KERN_SUCCESS) {
    return fail("synthetic code page RX restore failed") ? 0 : 1;
  }
  teso4m4_inactive_pacing_did_install();

  typedef int (*SyntheticLoop)(void);
  SyntheticLoop loop = (SyntheticLoop)(void *)(code + function_offset);
  if (loop() != 7 || g_false_logs != 1) {
    return fail("inactive state was not bypassed and recorded") ? 0 : 1;
  }
  *active_flag = 1;
  if (loop() != 7 || loop() != 7 || g_true_logs != 1) {
    return fail("active state transition was not forwarded and bounded") ? 0
                                                                         : 1;
  }
  if (g_ready_logs != 1 || g_active_logs != 1 || g_error_logs != 1) {
    return fail("install or rejection evidence was incomplete") ? 0 : 1;
  }
  if (teso4m4_inactive_pacing_prepare(
          (const struct mach_header_64 *)(uintptr_t)allocation, &target,
          patch)) {
    return fail("a second install was accepted") ? 0 : 1;
  }

  puts("Inactive pacing smoke: PASS mismatch_rejected=1 absolute_call=1 "
       "inactive_bypassed=1 active_forwarded=1 rx_restored=1");
  return 0;
}
