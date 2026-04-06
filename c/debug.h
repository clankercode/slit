#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

void debug_init(const char *log_path, int enabled);
void debug_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void debug_close(void);

#endif
