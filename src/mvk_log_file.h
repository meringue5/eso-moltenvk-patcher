#ifndef TESO4M4_MVK_LOG_FILE_H
#define TESO4M4_MVK_LOG_FILE_H

#include <stddef.h>
#include <stdio.h>

#define TESO4M4_PRODUCTION_LOG_ROTATION_BYTES (1024U * 1024U)

FILE* teso4m4_open_log_file(const char* path, size_t rotation_bytes);
FILE* teso4m4_open_production_log(void);

#endif
