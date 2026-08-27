#pragma once

typedef enum {
    TESO4M4_LOG_ERROR = 0,
    TESO4M4_LOG_WARN,
    TESO4M4_LOG_INFO,
    TESO4M4_LOG_DEBUG,
    TESO4M4_LOG_TRACE,
} Teso4m4LogLevel;

Teso4m4LogLevel teso4m4_classify_log_message(const char* message);
