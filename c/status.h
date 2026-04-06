#ifndef STATUS_H
#define STATUS_H

#include "slit.h"
#include <stddef.h>

char *format_status_line(enum spinner_type spinner, int frame, int eof,
                         size_t line_count, size_t total_bytes,
                         long file_size, int width);
void format_bytes_human(size_t bytes, char *buf, size_t buf_size);

#endif
