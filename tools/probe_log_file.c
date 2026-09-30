#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mvk_log_file.h"

static int fail(const char* message) {
    fprintf(stderr, "log file probe failed: %s\n", message);
    return 1;
}

static int write_bytes(const char* path, char value, size_t count) {
    FILE* file = fopen(path, "w");
    if (!file) {
        return 1;
    }
    for (size_t index = 0; index < count; index++) {
        if (fputc(value, file) == EOF) {
            fclose(file);
            return 1;
        }
    }
    return fclose(file) != 0;
}

int main(void) {
    char directory[] = "/private/tmp/teso4m4-log-file.XXXXXX";
    if (!mkdtemp(directory)) {
        return fail("mkdtemp");
    }
    char path[1024];
    char previous[1024];
    snprintf(path, sizeof(path), "%s/bridge.log", directory);
    snprintf(previous, sizeof(previous), "%s.1", path);

    FILE* file = teso4m4_open_log_file(path, 64);
    if (!file || fputs("small", file) == EOF || fclose(file) != 0) {
        return fail("small log open");
    }
    struct stat status = {0};
    if (stat(path, &status) != 0 || (status.st_mode & 0777) != 0600 ||
        access(previous, F_OK) == 0) {
        return fail("small log permissions or unexpected rotation");
    }
    if (chmod(path, 0644) != 0) {
        return fail("legacy permission fixture");
    }
    file = teso4m4_open_log_file(path, 64);
    if (!file || fclose(file) != 0 || stat(path, &status) != 0 ||
        (status.st_mode & 0777) != 0600) {
        return fail("legacy permissions were not tightened");
    }

    if (write_bytes(path, 'A', 64) || write_bytes(previous, 'B', 8)) {
        return fail("threshold fixture");
    }
    file = teso4m4_open_log_file(path, 64);
    if (!file || fclose(file) != 0) {
        return fail("rotated log open");
    }
    struct stat current_status = {0};
    struct stat previous_status = {0};
    if (stat(path, &current_status) != 0 || current_status.st_size != 0 ||
        (current_status.st_mode & 0777) != 0600 ||
        stat(previous, &previous_status) != 0 || previous_status.st_size != 64 ||
        (previous_status.st_mode & 0777) != 0600) {
        return fail("threshold rotation");
    }
    file = fopen(previous, "r");
    if (!file || fgetc(file) != 'A' || fclose(file) != 0) {
        return fail("previous generation replacement");
    }
    if (teso4m4_open_log_file("/missing-parent/bridge.log", 64) != NULL) {
        return fail("invalid path");
    }

    unlink(path);
    unlink(previous);

    // A source-maintenance log directory must capture the production log
    // without touching HOME, and an unusable override must disable logging
    // rather than fall back to the player's production or /tmp log.
    char home[1024];
    char home_library[1024];
    char override_path[1024];
    snprintf(home, sizeof(home), "%s/home", directory);
    snprintf(home_library, sizeof(home_library), "%s/Library", home);
    snprintf(override_path, sizeof(override_path), "%s/bridge.log", directory);
    if (mkdir(home, 0700) != 0 || setenv("HOME", home, 1) != 0 ||
        setenv(TESO4M4_LOG_DIR_ENV, directory, 1) != 0) {
        return fail("override fixture");
    }
    file = teso4m4_open_production_log();
    if (!file || fputs("override", file) == EOF || fclose(file) != 0 ||
        stat(override_path, &status) != 0 || status.st_size != 8 ||
        (status.st_mode & 0777) != 0600 || access(home_library, F_OK) == 0) {
        return fail("override log directory");
    }
    unlink(override_path);
    const char* unusable[] = {"", "relative/dir", "/missing-parent/logs",
                              override_path};
    if (write_bytes(override_path, 'C', 1)) {
        return fail("override file fixture");
    }
    for (size_t index = 0; index < sizeof(unusable) / sizeof(unusable[0]);
         index++) {
        if (setenv(TESO4M4_LOG_DIR_ENV, unusable[index], 1) != 0) {
            return fail("override fixture");
        }
        file = teso4m4_open_production_log();
        if (file) {
            fclose(file);
            return fail("unusable override fell back to another log");
        }
    }
    if (access(home_library, F_OK) == 0) {
        return fail("unusable override created the production log");
    }
    unsetenv(TESO4M4_LOG_DIR_ENV);

    unlink(override_path);
    rmdir(home);
    rmdir(directory);
    puts("Log file smoke: PASS rotation=1MiB generations=1 mode=0600 "
         "override=isolated fallback=none");
    return 0;
}
