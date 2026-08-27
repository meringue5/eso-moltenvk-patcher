#include <stdbool.h>
#include <string.h>

#include "mvk_log_policy.h"

static bool starts_with(const char* message, const char* prefix) {
    return strncmp(message, prefix, strlen(prefix)) == 0;
}

Teso4m4LogLevel teso4m4_classify_log_message(const char* message) {
    if (starts_with(message, "ERROR:") || starts_with(message, "FATAL:") ||
        starts_with(message, "GIPA_ERROR:") ||
        starts_with(message, "GDPA_ERROR:") ||
        strstr(message, "_ERROR:") != NULL) {
        return TESO4M4_LOG_ERROR;
    }
    if (starts_with(message, "SKIP:")) {
        return TESO4M4_LOG_WARN;
    }
    if (starts_with(message, "STARTUP_COLOR_AUDIT_BEGIN:") ||
        starts_with(message, "STARTUP_COLOR_AUDIT_FINISH:") ||
        starts_with(message, "STARTUP_PRESENT_") ||
        starts_with(message, "STARTUP_DRAW_AUDIT_BEGIN:") ||
        starts_with(message, "STARTUP_INPUT_AUDIT_BEGIN:") ||
        starts_with(message, "STARTUP_COMPOSITOR_AUDIT_BEGIN:") ||
        starts_with(message, "STARTUP_COMPOSITOR_IMAGE_") ||
        starts_with(message, "STARTUP_COMPOSITOR_NEUTRALIZE_")) {
        return TESO4M4_LOG_INFO;
    }
    if (starts_with(message, "GIPA:") || starts_with(message, "GDPA:") ||
        starts_with(message, "STARTUP_COLOR_")) {
        return TESO4M4_LOG_TRACE;
    }
    if (starts_with(message, "RUN_START:") || starts_with(message, "MODE:") ||
        starts_with(message, "MOLTENVK_CONFIG:") ||
        starts_with(message, "MOLTENVK:") || starts_with(message, "HDR_") ||
        starts_with(message, "ACTIVE:") ||
        starts_with(message, "INACTIVE_PACING_") ||
        starts_with(message, "RUNTIME_READINESS:") ||
        starts_with(message, "STARTUP_PIPELINE_") ||
        starts_with(message, "ESO SHA-256:")) {
        return TESO4M4_LOG_INFO;
    }
    return TESO4M4_LOG_DEBUG;
}
