#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include "render.h"
#include <stdlib.h>
#include <string.h>

static int decode_utf8(const char *s, size_t len, unsigned int *cp) {
    if (len < 1) return -1;
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xE0) == 0xC0) {
        if (len < 2) return -1;
        *cp = (unsigned int)((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0) {
        if (len < 3) return -1;
        *cp = (unsigned int)((c & 0x0F) << 12) | ((unsigned int)((unsigned char)s[1] & 0x3F) << 6) | ((unsigned char)s[2] & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0) {
        if (len < 4) return -1;
        *cp = (unsigned int)((c & 0x07) << 18) | ((unsigned int)((unsigned char)s[1] & 0x3F) << 12) | ((unsigned int)((unsigned char)s[2] & 0x3F) << 6) | ((unsigned char)s[3] & 0x3F);
        return 4;
    }
    return -1;
}

static int east_asian_width(unsigned int cp) {
    if (cp < 0x1100) return 1;
    if (cp >= 0x1100 && cp <= 0x115F) return 2;
    if (cp >= 0x231A && cp <= 0x231B) return 2;
    if (cp >= 0x2329 && cp <= 0x232A) return 2;
    if (cp >= 0x23E9 && cp <= 0x23EC) return 2;
    if (cp >= 0x23F0 && cp <= 0x23F3) return 2;
    if (cp >= 0x25FD && cp <= 0x25FE) return 2;
    if (cp >= 0x2614 && cp <= 0x2615) return 2;
    if (cp >= 0x2648 && cp <= 0x2653) return 2;
    if (cp >= 0x267F && cp <= 0x267F) return 2;
    if (cp >= 0x2693 && cp <= 0x2693) return 2;
    if (cp >= 0x26A1 && cp <= 0x26A1) return 2;
    if (cp >= 0x26AA && cp <= 0x26AB) return 2;
    if (cp >= 0x26BD && cp <= 0x26BE) return 2;
    if (cp >= 0x26C4 && cp <= 0x26C5) return 2;
    if (cp >= 0x26CE && cp <= 0x26CE) return 2;
    if (cp >= 0x26D4 && cp <= 0x26D4) return 2;
    if (cp >= 0x26EA && cp <= 0x26EA) return 2;
    if (cp >= 0x26F2 && cp <= 0x26F3) return 2;
    if (cp >= 0x26F5 && cp <= 0x26F5) return 2;
    if (cp >= 0x26FA && cp <= 0x26FA) return 2;
    if (cp >= 0x26FD && cp <= 0x26FD) return 2;
    if (cp >= 0x2702 && cp <= 0x2702) return 2;
    if (cp >= 0x2705 && cp <= 0x2705) return 2;
    if (cp >= 0x2708 && cp <= 0x270D) return 2;
    if (cp >= 0x270F && cp <= 0x270F) return 2;
    if (cp >= 0x2712 && cp <= 0x2712) return 2;
    if (cp >= 0x2714 && cp <= 0x2714) return 2;
    if (cp >= 0x2716 && cp <= 0x2716) return 2;
    if (cp >= 0x271D && cp <= 0x271D) return 2;
    if (cp >= 0x2721 && cp <= 0x2721) return 2;
    if (cp >= 0x2728 && cp <= 0x2728) return 2;
    if (cp >= 0x2733 && cp <= 0x2734) return 2;
    if (cp >= 0x2744 && cp <= 0x2744) return 2;
    if (cp >= 0x2747 && cp <= 0x2747) return 2;
    if (cp >= 0x274C && cp <= 0x274C) return 2;
    if (cp >= 0x274E && cp <= 0x274E) return 2;
    if (cp >= 0x2753 && cp <= 0x2755) return 2;
    if (cp >= 0x2757 && cp <= 0x2757) return 2;
    if (cp >= 0x2763 && cp <= 0x2764) return 2;
    if (cp >= 0x2795 && cp <= 0x2797) return 2;
    if (cp >= 0x27A1 && cp <= 0x27A1) return 2;
    if (cp >= 0x27B0 && cp <= 0x27B0) return 2;
    if (cp >= 0x27BF && cp <= 0x27BF) return 2;
    if (cp >= 0x2B1B && cp <= 0x2B1C) return 2;
    if (cp >= 0x2B50 && cp <= 0x2B50) return 2;
    if (cp >= 0x2B55 && cp <= 0x2B55) return 2;
    if (cp >= 0x2E80 && cp <= 0x2E99) return 2;
    if (cp >= 0x2E9B && cp <= 0x2EF3) return 2;
    if (cp >= 0x2F00 && cp <= 0x2FD5) return 2;
    if (cp >= 0x2FF0 && cp <= 0x2FFB) return 2;
    if (cp >= 0x3000 && cp <= 0x303E) return 2;
    if (cp >= 0x3041 && cp <= 0x3096) return 2;
    if (cp >= 0x3099 && cp <= 0x30FF) return 2;
    if (cp >= 0x3105 && cp <= 0x312D) return 2;
    if (cp >= 0x3131 && cp <= 0x318E) return 2;
    if (cp >= 0x3190 && cp <= 0x31BA) return 2;
    if (cp >= 0x31C0 && cp <= 0x31E3) return 2;
    if (cp >= 0x31F0 && cp <= 0x321E) return 2;
    if (cp >= 0x3220 && cp <= 0x3247) return 2;
    if (cp >= 0x3250 && cp <= 0x32FE) return 2;
    if (cp >= 0x3300 && cp <= 0x4DBF) return 2;
    if (cp >= 0x4E00 && cp <= 0x9FFC) return 2;
    if (cp >= 0xA000 && cp <= 0xA48C) return 2;
    if (cp >= 0xA490 && cp <= 0xA4C6) return 2;
    if (cp >= 0xA960 && cp <= 0xA97C) return 2;
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 2;
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;
    if (cp >= 0xFE10 && cp <= 0xFE19) return 2;
    if (cp >= 0xFE30 && cp <= 0xFE52) return 2;
    if (cp >= 0xFE54 && cp <= 0xFE66) return 2;
    if (cp >= 0xFE68 && cp <= 0xFE6B) return 2;
    if (cp >= 0xFF01 && cp <= 0xFF60) return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;
    if (cp >= 0x16FE0 && cp <= 0x16FE1) return 2;
    if (cp >= 0x17000 && cp <= 0x187EC) return 2;
    if (cp >= 0x18800 && cp <= 0x18AF2) return 2;
    if (cp >= 0x1B000 && cp <= 0x1B11E) return 2;
    if (cp >= 0x1B150 && cp <= 0x1B152) return 2;
    if (cp >= 0x1B164 && cp <= 0x1B167) return 2;
    if (cp >= 0x1B170 && cp <= 0x1B2FB) return 2;
    if (cp >= 0x1F200 && cp <= 0x1F200) return 2;
    if (cp >= 0x1F210 && cp <= 0x1F219) return 2;
    if (cp >= 0x1F21B && cp <= 0x1F22E) return 2;
    if (cp >= 0x1F230 && cp <= 0x1F231) return 2;
    if (cp >= 0x1F23B && cp <= 0x1F23B) return 2;
    if (cp >= 0x1F300 && cp <= 0x1F321) return 2;
    if (cp >= 0x1F324 && cp <= 0x1F393) return 2;
    if (cp >= 0x1F396 && cp <= 0x1F397) return 2;
    if (cp >= 0x1F399 && cp <= 0x1F39B) return 2;
    if (cp >= 0x1F39E && cp <= 0x1F3F0) return 2;
    if (cp >= 0x1F3F3 && cp <= 0x1F3F5) return 2;
    if (cp >= 0x1F3F7 && cp <= 0x1F4FD) return 2;
    if (cp >= 0x1F4FF && cp <= 0x1F53D) return 2;
    if (cp >= 0x1F549 && cp <= 0x1F54E) return 2;
    if (cp >= 0x1F550 && cp <= 0x1F567) return 2;
    if (cp >= 0x1F56F && cp <= 0x1F570) return 2;
    if (cp >= 0x1F573 && cp <= 0x1F57A) return 2;
    if (cp >= 0x1F587 && cp <= 0x1F587) return 2;
    if (cp >= 0x1F58A && cp <= 0x1F58D) return 2;
    if (cp >= 0x1F590 && cp <= 0x1F590) return 2;
    if (cp >= 0x1F595 && cp <= 0x1F596) return 2;
    if (cp >= 0x1F5A4 && cp <= 0x1F5A4) return 2;
    if (cp >= 0x1F5A5 && cp <= 0x1F5A5) return 2;
    if (cp >= 0x1F5A8 && cp <= 0x1F5A8) return 2;
    if (cp >= 0x1F5B1 && cp <= 0x1F5B2) return 2;
    if (cp >= 0x1F5BC && cp <= 0x1F5BC) return 2;
    if (cp >= 0x1F5C2 && cp <= 0x1F5C4) return 2;
    if (cp >= 0x1F5D1 && cp <= 0x1F5D3) return 2;
    if (cp >= 0x1F5DC && cp <= 0x1F5DE) return 2;
    if (cp >= 0x1F5E1 && cp <= 0x1F5E1) return 2;
    if (cp >= 0x1F5E3 && cp <= 0x1F5E3) return 2;
    if (cp >= 0x1F5E8 && cp <= 0x1F5E8) return 2;
    if (cp >= 0x1F5EF && cp <= 0x1F5EF) return 2;
    if (cp >= 0x1F5F3 && cp <= 0x1F5F3) return 2;
    if (cp >= 0x1F5FA && cp <= 0x1F64F) return 2;
    if (cp >= 0x1F680 && cp <= 0x1F6C5) return 2;
    if (cp >= 0x1F6CB && cp <= 0x1F6D2) return 2;
    if (cp >= 0x1F6D5 && cp <= 0x1F6D5) return 2;
    if (cp >= 0x1F6E0 && cp <= 0x1F6E8) return 2;
    if (cp >= 0x1F6EB && cp <= 0x1F6EC) return 2;
    if (cp >= 0x1F6F0 && cp <= 0x1F6F8) return 2;
    if (cp >= 0x1F910 && cp <= 0x1F93E) return 2;
    if (cp >= 0x1F940 && cp <= 0x1F970) return 2;
    if (cp >= 0x1F973 && cp <= 0x1F976) return 2;
    if (cp >= 0x1F97A && cp <= 0x1F97A) return 2;
    if (cp >= 0x1F97C && cp <= 0x1F9A2) return 2;
    if (cp >= 0x1F9A5 && cp <= 0x1F9AA) return 2;
    if (cp >= 0x1F9AE && cp <= 0x1F9C0) return 2;
    if (cp >= 0x1F9C2 && cp <= 0x1F9C2) return 2;
    if (cp >= 0x1F9C5 && cp <= 0x1F9CA) return 2;
    if (cp >= 0x1F9CD && cp <= 0x1F9FF) return 2;
    if (cp >= 0x1FA00 && cp <= 0x1FA6F) return 2;
    if (cp >= 0x1FA70 && cp <= 0x1FA74) return 2;
    if (cp >= 0x1FA78 && cp <= 0x1FA7A) return 2;
    if (cp >= 0x1FA80 && cp <= 0x1FA86) return 2;
    if (cp >= 0x1FA90 && cp <= 0x1FAA8) return 2;
    if (cp >= 0x1FAB0 && cp <= 0x1FAB6) return 2;
    if (cp >= 0x1FAC0 && cp <= 0x1FAC2) return 2;
    if (cp >= 0x1FAD0 && cp <= 0x1FAD6) return 2;
    if (cp >= 0x20000 && cp <= 0x2FFFD) return 2;
    if (cp >= 0x30000 && cp <= 0x3FFFD) return 2;
    return 1;
}

static int char_width(const char *s, size_t bytes) {
    if (bytes == 1) return 1;
    unsigned int cp;
    int r = decode_utf8(s, bytes, &cp);
    if (r < 0) return 1;
    return east_asian_width(cp);
}

size_t visible_strlen(const char *s) {
    size_t len = 0;
    int state = 0;
    for (const char *p = s; *p; p++) {
        if (state == 0) {
            if (*p == '\x1b') {
                state = 1;
            } else if ((*p & 0xC0) != 0x80) {
                size_t bytes = 0;
                if ((*p & 0xE0) == 0xC0) bytes = 2;
                else if ((*p & 0xF0) == 0xE0) bytes = 3;
                else if ((*p & 0xF8) == 0xF0) bytes = 4;
                else bytes = 1;
                len += (size_t)char_width(p, bytes);
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
    if (visible_strlen(src) <= width) {
        size_t slen = strlen(src);
        if (slen + 1 < dst_size) {
            memcpy(dst, src, slen + 1);
        } else {
            memcpy(dst, src, dst_size - 1);
            dst[dst_size - 1] = '\0';
        }
        return strlen(dst);
    }

    size_t trunc_visible = visible_strlen(trunc_char);
    size_t target = 0;
    if (width > trunc_visible) target = width - trunc_visible;

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
                if ((c & 0xC0) != 0x80) {
                    size_t bytes = 1;
                    if ((c & 0xE0) == 0xC0) bytes = 2;
                    else if ((c & 0xF0) == 0xE0) bytes = 3;
                    else if ((c & 0xF8) == 0xF0) bytes = 4;
                    int cw = char_width(src + si, bytes);
                    if (visible + (size_t)cw > target) break;
                    if (di + bytes < dst_size) {
                        for (size_t b = 0; b < bytes && si + b < src_len; b++) {
                            dst[di++] = src[si + b];
                        }
                        si += bytes - 1;
                        visible += (size_t)cw;
                    } else {
                        dst[di] = '\0';
                        break;
                    }
                } else {
                    if (di + 1 < dst_size) dst[di++] = src[si];
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

    while (di > 0) {
        unsigned char last = (unsigned char)dst[di - 1];
        int is_term = (last >= 'A' && last <= 'Z') || (last >= 'a' && last <= 'z') || last == '\x07';
        if (!is_term) break;
        size_t j = di - 1;
        while (j > 0 && (unsigned char)dst[j - 1] != '\x1b') j--;
        if (j == 0 && (unsigned char)dst[0] != '\x1b') break;
        size_t seq_len = di - j;
        int valid = 0;
        if (seq_len >= 3 && (unsigned char)dst[j + 1] == '[') {
            int all_params = 1;
            for (size_t k = j + 2; k < di - 1; k++) {
                unsigned char pc = (unsigned char)dst[k];
                if (!((pc >= '0' && pc <= '9') || pc == ';')) {
                    all_params = 0;
                    break;
                }
            }
            if (all_params) valid = 1;
        } else if (seq_len >= 3 && (unsigned char)dst[j + 1] == ']' && last == '\x07') {
            valid = 1;
        }
        if (!valid) break;
        di = j;
    }

    size_t trunc_len = strlen(trunc_char);
    for (size_t i = 0; i < trunc_len && di + 1 < dst_size; i++) {
        dst[di++] = trunc_char[i];
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
        if ((c & 0xC0) != 0x80) {
            size_t bytes = 1;
            if ((c & 0xE0) == 0xC0) bytes = 2;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xF8) == 0xF0) bytes = 4;
            visible += (size_t)char_width(line + i, bytes);
        }
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
