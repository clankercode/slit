#include "render.h"
#include <stdlib.h>
#include <string.h>

size_t visible_strlen(const char *s) {
    size_t len = 0;
    int state = 0;
    for (const char *p = s; *p; p++) {
        if (state == 0) {
            if (*p == '\x1b') {
                state = 1;
            } else if ((*p & 0xC0) != 0x80) {
                len++;
            }
        } else if (state == 1) {
            if (*p == '[') {
                state = 2;
            } else if (*p == ']') {
                state = 3;
            } else {
                state = 0;
            }
        } else if (state == 2) {
            if (*p >= '@' && *p <= '~') {
                state = 0;
            }
        } else if (state == 3) {
            if (*p == '\x07') {
                state = 0;
            }
        }
    }
    return len;
}

size_t strip_ansi(const char *src, char *dst, size_t dst_size) {
    size_t di = 0;
    int state = 0;
    for (const char *p = src; *p && di + 1 < dst_size; p++) {
        if (state == 0) {
            if (*p == '\x1b') {
                state = 1;
            } else {
                dst[di++] = *p;
            }
        } else if (state == 1) {
            if (*p == '[') {
                state = 2;
            } else if (*p == ']') {
                state = 3;
            } else {
                state = 0;
            }
        } else if (state == 2) {
            if (*p >= '@' && *p <= '~') {
                state = 0;
            }
        } else if (state == 3) {
            if (*p == '\x07') {
                state = 0;
            }
        }
    }
    dst[di] = '\0';
    return di;
}

size_t trim_line(const char *src, char *dst, size_t dst_size, size_t width, const char *trunc_char) {
    size_t trunc_visible = visible_strlen(trunc_char);
    size_t max_visible = width;
    int need_trunc = 0;

    size_t visible = 0;
    size_t di = 0;
    int state = 0;
    size_t src_len = strlen(src);

    for (size_t si = 0; si < src_len; si++) {
        unsigned char c = (unsigned char)src[si];

        if (state == 0) {
            if (c == '\x1b') {
                state = 1;
                if (di + 1 < dst_size) dst[di++] = src[si];
            } else {
                if (visible < max_visible) {
                    if (di + 1 < dst_size) dst[di++] = src[si];
                    if ((c & 0xC0) != 0x80) {
                        visible++;
                        if (visible >= max_visible) {
                            size_t remaining = src_len - si - 1;
                            if (remaining > 0) {
                                need_trunc = 1;
                                break;
                            }
                        }
                    }
                } else {
                    need_trunc = 1;
                    break;
                }
            }
        } else if (state == 1) {
            if (c == '[') {
                state = 2;
            } else if (c == ']') {
                state = 3;
            } else {
                state = 0;
            }
            if (di + 1 < dst_size) dst[di++] = src[si];
        } else if (state == 2) {
            if (di + 1 < dst_size) dst[di++] = src[si];
            if (c >= '@' && c <= '~') {
                state = 0;
            }
        } else if (state == 3) {
            if (di + 1 < dst_size) dst[di++] = src[si];
            if (c == '\x07') {
                state = 0;
            }
        }
    }

    if (need_trunc) {
        if (visible + trunc_visible > max_visible) {
            size_t remove_bytes = 0;
            size_t remove_visible = 0;
            while (visible - remove_visible + trunc_visible > max_visible && di > remove_bytes) {
                di--;
                if (((unsigned char)dst[di] & 0xC0) != 0x80) {
                    remove_visible++;
                }
                remove_bytes++;
            }
        }
        size_t trunc_len = strlen(trunc_char);
        for (size_t i = 0; i < trunc_len && di + 1 < dst_size; i++) {
            dst[di++] = trunc_char[i];
        }
    }

    dst[di] = '\0';
    return di;
}

int wrap_line(const char *line, size_t width, char ***out_lines, int *out_count) {
    if (!line || !out_lines || !out_count) return -1;
    if (width == 0) width = 1;

    size_t line_len = strlen(line);
    int capacity = 8;
    char **result = malloc(capacity * sizeof(char *));
    if (!result) return -1;
    int count = 0;

    size_t seg_start = 0;
    size_t visible = 0;
    int state = 0;

    for (size_t i = 0; i <= line_len; i++) {
        int at_end = (i == line_len);
        int should_break = 0;

        if (!at_end) {
            unsigned char c = (unsigned char)line[i];
            if (state == 0) {
                if (c == '\x1b') {
                    state = 1;
                } else if ((c & 0xC0) != 0x80) {
                    visible++;
                    if (visible > width) {
                        should_break = 1;
                    }
                }
            } else if (state == 1) {
                if (c == '[') state = 2;
                else if (c == ']') state = 3;
                else state = 0;
            } else if (state == 2) {
                if (c >= '@' && c <= '~') state = 0;
            } else if (state == 3) {
                if (c == '\x07') state = 0;
            }
        } else {
            should_break = 1;
        }

        if (should_break) {
            size_t seg_end = at_end ? i : i;
            size_t seg_len = seg_end - seg_start;

            char *seg = malloc(seg_len + 1);
            if (!seg) {
                for (int j = 0; j < count; j++) free(result[j]);
                free(result);
                return -1;
            }
            memcpy(seg, line + seg_start, seg_len);
            seg[seg_len] = '\0';

            if (count + 1 >= capacity) {
                capacity *= 2;
                char **new_result = realloc(result, capacity * sizeof(char *));
                if (!new_result) {
                    free(seg);
                    for (int j = 0; j < count; j++) free(result[j]);
                    free(result);
                    return -1;
                }
                result = new_result;
            }
            result[count++] = seg;

            if (!at_end) {
                seg_start = i;
                visible = 1;
            }
        }
    }

    if (count == 0) {
        char *empty = malloc(1);
        if (!empty) { free(result); return -1; }
        empty[0] = '\0';
        result[count++] = empty;
    }

    *out_lines = result;
    *out_count = count;
    return 0;
}

void wrap_lines_free(char **lines, int count) {
    if (!lines) return;
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}
