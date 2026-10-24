#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "eso_inactive_pacing.h"

#define TESO_UPDATE_TARGET_COUNT 17
#define TESO_UPDATE_PROFILE_LIMIT 4096
typedef struct {
    uint64_t offset;
    uint8_t expected[12];
} TesoUpdateTarget;
typedef struct {
    char executable_sha256[65];
    uint8_t uuid[16];
    char client_version[32], client_databuild[32];
    uint64_t placement, gipa_slot, gdpa_slot;
    TesoUpdateTarget targets[TESO_UPDATE_TARGET_COUNT];
    Teso4m4InactivePacingTarget pacing;
} TesoUpdateProfile;

#pragma GCC visibility push(hidden)

/* Discovery uses immutable compiled rules. Profile files never supply addresses. */
bool teso_update_resolve(const char *executable, const char *archive,
                         TesoUpdateProfile *profile);
size_t teso_update_render(const TesoUpdateProfile *profile, char *output,
                          size_t capacity);
bool teso_update_verify_file(const TesoUpdateProfile *profile, const char *path,
                             char digest[65]);
bool teso_update_file_sha256(const char *path, char output[65]);
bool teso_update_payloads_match(const char *runtime, const char *renamed_original);

#pragma GCC visibility pop
