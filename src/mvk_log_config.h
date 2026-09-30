#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "mvk_log_policy.h"

typedef enum {
    TESO4M4_LOG_CONFIG_MISSING = 0,
    TESO4M4_LOG_CONFIG_APPLIED,
    TESO4M4_LOG_CONFIG_INVALID,
} Teso4m4LogConfigResult;

bool teso4m4_log_level_from_name(const char* name, Teso4m4LogLevel* output);
bool teso4m4_log_level_config_path(const char* bridge_directory, char* output,
                                   size_t output_size);
Teso4m4LogConfigResult teso4m4_read_log_level_config(
    const char* bridge_directory, Teso4m4LogLevel* output);
