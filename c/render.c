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

    if (need_trunc || di > 0) {
        if (di > 0 && (unsigned char)dst[di - 1] >= 0xC0) {
            di--;
            if (visible > 0) visible--;
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
    int result_cap = 8;
    char **result = malloc(result_cap * sizeof(char *));
    if (!result) return -1;
    int count = 0;

    size_t seg_cap = line_len + 128;
    char *seg = malloc(seg_cap);
    if (!seg) { free(result); return -1; }
    size_t seg_len = 0;

    char active_sgr[256];
    size_t active_sgr_len = 0;

    char pending[1024];
    size_t pending_len = 0;

    size_t visible = 0;
    size_t i = 0;

    while (i < line_len) {
        unsigned char c = (unsigned char)line[i];

        if (c == '\x1b') {
            size_t seq_start = i;
            i++;
            if (i < line_len && (unsigned char)line[i] == '[') {
                i++;
                while (i < line_len) {
                    unsigned char sc = (unsigned char)line[i];
                    if (sc >= '@' && sc <= '~') {
                        if (sc == 'm') {
                            size_t slen = i - seq_start + 1;
                            if (slen == 4 && line[seq_start + 2] == '0') {
                                active_sgr_len = 0;
                            } else if (slen < sizeof(active_sgr)) {
                                memcpy(active_sgr, line + seq_start, slen);
                                active_sgr_len = slen;
                            }
                        }
                        i++;
                        break;
                    }
                    i++;
                }
            } else if (i < line_len && (unsigned char)line[i] == ']') {
                i++;
                while (i < line_len) {
                    if ((unsigned char)line[i] == '\x07') { i++; break; }
                    i++;
                }
            } else {
                if (i < line_len) i++;
            }
            size_t seq_len = i - seq_start;
            if (pending_len + seq_len < sizeof(pending)) {
                memcpy(pending + pending_len, line + seq_start, seq_len);
                pending_len += seq_len;
            }
            continue;
        }

        if ((c & 0xC0) != 0x80 && visible >= width) {
            if (active_sgr_len > 0 && seg_len + 4 < seg_cap) {
                memcpy(seg + seg_len, "\x1b[0m", 4);
                seg_len += 4;
            }
            seg[seg_len] = '\0';
            if (count + 1 >= result_cap) {
                result_cap *= 2;
                char **nr = realloc(result, result_cap * sizeof(char *));
                if (!nr) { free(seg); for (int j = 0; j < count; j++) free(result[j]); free(result); return -1; }
                result = nr;
            }
            result[count++] = seg;

            seg = malloc(seg_cap);
            if (!seg) { for (int j = 0; j < count; j++) free(result[j]); free(result); return -1; }
            seg_len = 0;
            if (active_sgr_len > 0) {
                memcpy(seg, active_sgr, active_sgr_len);
                seg_len = active_sgr_len;
            }
            if (pending_len > 0 && seg_len + pending_len < seg_cap) {
                memcpy(seg + seg_len, pending, pending_len);
                seg_len += pending_len;
            }
            pending_len = 0;
            visible = 0;
        }

        if (pending_len > 0 && (c & 0xC0) != 0x80) {
            if (seg_len + pending_len < seg_cap) {
                memcpy(seg + seg_len, pending, pending_len);
                seg_len += pending_len;
            }
            pending_len = 0;
        }

        if (seg_len + 1 < seg_cap) seg[seg_len++] = line[i];
        if ((c & 0xC0) != 0x80) visible++;
        i++;
    }

    if (pending_len > 0 && seg_len + pending_len < seg_cap) {
        memcpy(seg + seg_len, pending, pending_len);
        seg_len += pending_len;
    }

    if (seg_len > 0 || count == 0) {
        seg[seg_len] = '\0';
        if (count + 1 >= result_cap) {
            result_cap *= 2;
            char **nr = realloc(result, result_cap * sizeof(char *));
            if (!nr) { free(seg); for (int j = 0; j < count; j++) free(result[j]); free(result); return -1; }
            result = nr;
        }
        result[count++] = seg;
    } else {
        free(seg);
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
