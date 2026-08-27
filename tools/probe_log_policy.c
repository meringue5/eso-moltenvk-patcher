#include <stdio.h>

#include "mvk_log_policy.h"

static int check(const char* message, Teso4m4LogLevel expected) {
    const Teso4m4LogLevel actual = teso4m4_classify_log_message(message);
    if (actual != expected) {
        fprintf(stderr, "log policy mismatch: %s expected=%d actual=%d\n",
                message, expected, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    int failed = 0;
    failed |= check("STARTUP_COLOR_AUDIT_BEGIN: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COLOR_AUDIT_FINISH: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_PRESENT_PIXEL_SUMMARY: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_PRESENT_DRAW_INPUT: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_DRAW_AUDIT_BEGIN: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_INPUT_AUDIT_BEGIN: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COMPOSITOR_AUDIT_BEGIN: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COMPOSITOR_IMAGE_SUMMARY: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: bounded", TESO4M4_LOG_INFO);
    failed |= check("STARTUP_COLOR_CLEAR: detail", TESO4M4_LOG_TRACE);
    failed |= check("GIPA: proc detail", TESO4M4_LOG_TRACE);
    failed |= check("unclassified lifecycle detail", TESO4M4_LOG_DEBUG);
    failed |= check("SKIP: safe stop", TESO4M4_LOG_WARN);
    failed |= check("STARTUP_PRESENT_PIXEL_ERROR: failure", TESO4M4_LOG_ERROR);
    if (failed) {
        return 1;
    }
    puts("Log policy smoke: PASS bounded_audit=info detail=trace generic=debug");
    return 0;
}
