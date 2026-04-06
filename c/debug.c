#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

static FILE *debug_fp = NULL;
static int debug_enabled = 0;

void debug_init(const char *log_path, int enabled) {
    debug_enabled = enabled;
    if (!enabled) return;

    const char *path = log_path;
    char default_path[256];

    if (!path || path[0] == '\0') {
        snprintf(default_path, sizeof(default_path), "/tmp/slit-%d.log", getpid());
        path = default_path;
    }

    debug_fp = fopen(path, "w");
}

void debug_log(const char *fmt, ...) {
    if (!debug_enabled || !debug_fp) return;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    localtime_r(&ts.tv_sec, &tm_buf);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_buf);
    fprintf(debug_fp, "[%s.%03ld] ", time_buf, ts.tv_nsec / 1000000);

    va_list args;
    va_start(args, fmt);
    vfprintf(debug_fp, fmt, args);
    va_end(args);

    fputc('\n', debug_fp);
    fflush(debug_fp);
}

void debug_close(void) {
    if (debug_fp) {
        fclose(debug_fp);
        debug_fp = NULL;
    }
}
