#include "mvk_log_config.h"

#include <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool hex_digest(const uint8_t digest[CC_SHA256_DIGEST_LENGTH],
                       char output[65]) {
    static const char digits[] = "0123456789abcdef";
    for (size_t index = 0; index < CC_SHA256_DIGEST_LENGTH; ++index) {
        output[index * 2] = digits[digest[index] >> 4];
        output[index * 2 + 1] = digits[digest[index] & 0x0f];
    }
    output[64] = '\0';
    return true;
}

bool teso4m4_log_level_from_name(const char* name, Teso4m4LogLevel* output) {
    if (!name || !output) {
        return false;
    }
    if (strcmp(name, "error") == 0) {
        *output = TESO4M4_LOG_ERROR;
    } else if (strcmp(name, "warn") == 0) {
        *output = TESO4M4_LOG_WARN;
    } else if (strcmp(name, "info") == 0) {
        *output = TESO4M4_LOG_INFO;
    } else if (strcmp(name, "debug") == 0) {
        *output = TESO4M4_LOG_DEBUG;
    } else if (strcmp(name, "trace") == 0) {
        *output = TESO4M4_LOG_TRACE;
    } else {
        return false;
    }
    return true;
}

bool teso4m4_log_level_config_path(const char* bridge_directory, char* output,
                                   size_t output_size) {
    static const char suffix[] = "/Contents/MacOS";
    const char* home = getenv("HOME");
    if (!bridge_directory || !home || home[0] == '\0' || !output ||
        output_size == 0) {
        return false;
    }
    const size_t directory_length = strlen(bridge_directory);
    const size_t suffix_length = sizeof(suffix) - 1;
    if (directory_length <= suffix_length ||
        strcmp(bridge_directory + directory_length - suffix_length, suffix) !=
            0) {
        return false;
    }
    char app_path[4096];
    const size_t app_length = directory_length - suffix_length;
    if (app_length >= sizeof(app_path)) {
        return false;
    }
    memcpy(app_path, bridge_directory, app_length);
    app_path[app_length] = '\0';
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(app_path, (CC_LONG)app_length, digest);
    char identifier[65];
    hex_digest(digest, identifier);
    const int written = snprintf(
        output, output_size,
        "%s/Library/Application Support/ESO MoltenVK Patcher/Installations/%s/logging.env",
        home, identifier);
    return written >= 0 && (size_t)written < output_size;
}

Teso4m4LogConfigResult teso4m4_read_log_level_config(
    const char* bridge_directory, Teso4m4LogLevel* output) {
    if (!output) {
        return TESO4M4_LOG_CONFIG_INVALID;
    }
    char path[4096];
    if (!teso4m4_log_level_config_path(bridge_directory, path, sizeof(path))) {
        return TESO4M4_LOG_CONFIG_INVALID;
    }
    int flags = O_RDONLY | O_CLOEXEC;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    const int descriptor = open(path, flags);
    if (descriptor < 0) {
        return errno == ENOENT ? TESO4M4_LOG_CONFIG_MISSING
                               : TESO4M4_LOG_CONFIG_INVALID;
    }
    struct stat status = {0};
    if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_uid != getuid() || (status.st_mode & 0022) != 0) {
        close(descriptor);
        return TESO4M4_LOG_CONFIG_INVALID;
    }
    char contents[32] = {0};
    const ssize_t count = read(descriptor, contents, sizeof(contents) - 1);
    close(descriptor);
    if (count <= 0 || (size_t)count == sizeof(contents) - 1) {
        return TESO4M4_LOG_CONFIG_INVALID;
    }
    contents[count] = '\0';
    size_t length = (size_t)count;
    if (contents[length - 1] == '\n') {
        contents[--length] = '\0';
    }
    if (strchr(contents, '\n') || strncmp(contents, "level=", 6) != 0 ||
        !teso4m4_log_level_from_name(contents + 6, output) ||
        *output == TESO4M4_LOG_TRACE) {
        return TESO4M4_LOG_CONFIG_INVALID;
    }
    return TESO4M4_LOG_CONFIG_APPLIED;
}
