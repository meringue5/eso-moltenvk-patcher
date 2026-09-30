#ifndef TESO4M4_MVK_LOG_FILE_H
#define TESO4M4_MVK_LOG_FILE_H

#include <stddef.h>
#include <stdio.h>

#define TESO4M4_PRODUCTION_LOG_ROTATION_BYTES (1024U * 1024U)

/* Source-maintenance override. When set (even to an unusable value), the
 * bridge writes only to <dir>/bridge.log and never falls back to the
 * production or legacy temporary log. Build probes use it so a non-game load
 * cannot append runs to the player's production log. */
#define TESO4M4_LOG_DIR_ENV "TESO4M4_LOG_DIR"

FILE* teso4m4_open_log_file(const char* path, size_t rotation_bytes);
FILE* teso4m4_open_production_log(void);

#endif
