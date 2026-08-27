#include "mvk_log_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool format_path(char* output, size_t output_size, const char* format,
                        const char* value) {
    const int written = snprintf(output, output_size, format, value);
    return written >= 0 && (size_t)written < output_size;
}

static void rotate_log_if_needed(const char* path, size_t rotation_bytes) {
    struct stat status = {0};
    if (rotation_bytes == 0 || stat(path, &status) != 0 ||
        status.st_size < (off_t)rotation_bytes) {
        return;
    }

    char previous[4096];
    if (!format_path(previous, sizeof(previous), "%s.1", path)) {
        return;
    }
    if (rename(path, previous) == 0) {
        (void)chmod(previous, 0600);
    }
}

FILE* teso4m4_open_log_file(const char* path, size_t rotation_bytes) {
    if (!path || path[0] == '\0') {
        return NULL;
    }
    rotate_log_if_needed(path, rotation_bytes);
    const int descriptor =
        open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (descriptor < 0) {
        return NULL;
    }
    if (fchmod(descriptor, 0600) != 0) {
        close(descriptor);
        return NULL;
    }
    FILE* file = fdopen(descriptor, "a");
    if (!file) {
        close(descriptor);
    }
    return file;
}

FILE* teso4m4_open_production_log(void) {
    const char* home = getenv("HOME");
    if (home && home[0] != '\0') {
        char library[4096];
        char logs[4096];
        char product[4096];
        char path[4096];
        if (format_path(library, sizeof(library), "%s/Library", home) &&
            format_path(logs, sizeof(logs), "%s/Logs", library) &&
            format_path(product, sizeof(product),
                        "%s/ESO MoltenVK Patcher", logs) &&
            format_path(path, sizeof(path), "%s/bridge.log", product) &&
            (mkdir(library, 0700) == 0 || errno == EEXIST) &&
            (mkdir(logs, 0700) == 0 || errno == EEXIST) &&
            (mkdir(product, 0700) == 0 || errno == EEXIST)) {
            FILE* file = teso4m4_open_log_file(
                path, TESO4M4_PRODUCTION_LOG_ROTATION_BYTES);
            if (file) {
                return file;
            }
        }
    }
    return teso4m4_open_log_file("/tmp/teso4m4.log",
                                 TESO4M4_PRODUCTION_LOG_ROTATION_BYTES);
}
