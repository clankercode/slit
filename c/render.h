#ifndef RENDER_H
#define RENDER_H

#include <stddef.h>

size_t visible_strlen(const char *s);
size_t strip_ansi(const char *src, char *dst, size_t dst_size);
size_t trim_line(const char *src, char *dst, size_t dst_size, size_t width, const char *trunc_char);
int wrap_line(const char *line, size_t width, char ***out_lines, int *out_count);
void wrap_lines_free(char **lines, int count);

#endif
