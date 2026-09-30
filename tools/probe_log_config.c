#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mvk_log_config.h"

static int fail(const char* message) {
    fprintf(stderr, "log config probe failed: %s\n", message);
    return 1;
}

static int make_parents(char* path) {
    for (char* cursor = path + 1; *cursor; ++cursor) {
        if (*cursor != '/') {
            continue;
        }
        *cursor = '\0';
        if (mkdir(path, 0700) != 0 && access(path, F_OK) != 0) {
            return 1;
        }
        *cursor = '/';
    }
    return mkdir(path, 0700) != 0 && access(path, F_OK) != 0;
}

int main(void) {
    char root[] = "/private/tmp/teso4m4-log-config.XXXXXX";
    if (!mkdtemp(root)) {
        return fail("mkdtemp");
    }
    if (setenv("HOME", root, 1) != 0) {
        return fail("setenv");
    }
    char bridge_directory[4096];
    snprintf(bridge_directory, sizeof(bridge_directory),
             "%s/Custom ESO/eso.app/Contents/MacOS", root);
    char config[4096];
    if (!teso4m4_log_level_config_path(bridge_directory, config,
                                       sizeof(config))) {
        return fail("config path");
    }
    Teso4m4LogLevel level = TESO4M4_LOG_INFO;
    if (teso4m4_read_log_level_config(bridge_directory, &level) !=
        TESO4M4_LOG_CONFIG_MISSING) {
        return fail("missing config");
    }
    char parent[4096];
    snprintf(parent, sizeof(parent), "%s", config);
    char* slash = strrchr(parent, '/');
    if (!slash) {
        return fail("config parent");
    }
    *slash = '\0';
    if (make_parents(parent) != 0) {
        return fail("make parents");
    }
    FILE* file = fopen(config, "w");
    if (!file || fputs("level=debug\n", file) == EOF || fclose(file) != 0 ||
        chmod(config, 0600) != 0) {
        return fail("debug config");
    }
    if (teso4m4_read_log_level_config(bridge_directory, &level) !=
            TESO4M4_LOG_CONFIG_APPLIED ||
        level != TESO4M4_LOG_DEBUG) {
        return fail("debug level");
    }
    file = fopen(config, "w");
    if (!file || fputs("level=trace\n", file) == EOF || fclose(file) != 0 ||
        chmod(config, 0600) != 0 ||
        teso4m4_read_log_level_config(bridge_directory, &level) !=
            TESO4M4_LOG_CONFIG_INVALID) {
        return fail("trace rejection");
    }
    file = fopen(config, "w");
    if (!file || fputs("level=info\nextra=1\n", file) == EOF ||
        fclose(file) != 0 || chmod(config, 0600) != 0 ||
        teso4m4_read_log_level_config(bridge_directory, &level) !=
            TESO4M4_LOG_CONFIG_INVALID) {
        return fail("strict format");
    }
    unlink(config);
    rmdir(parent);
    puts("Log config smoke: PASS default=info debug=config trace=source-only");
    return 0;
}
