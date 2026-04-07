#include "status.h"
#include "spinner.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void format_bytes_human(size_t bytes, char *buf, size_t buf_size) {
    if (bytes < 1024) {
        snprintf(buf, buf_size, "%zuB", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, buf_size, "%.1fKB", (double)bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024 * 1024) {
        snprintf(buf, buf_size, "%.1fMB", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buf, buf_size, "%.1fGB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

char *format_status_line(enum spinner_type spinner, int frame, int eof,
                         size_t line_count, size_t total_bytes,
                         long file_size, int width) {
    const char *spinner_str = spinner_frame(spinner, frame);
    char byte_str[32] = "";
    format_bytes_human(total_bytes, byte_str, sizeof(byte_str));
    char progress_str[64] = "";

    if (file_size > 0) {
        int bar_width = 10;
        int filled = (int)((double)bar_width * (double)total_bytes / (double)file_size);
        if (filled > bar_width) filled = bar_width;
        if (filled < 0) filled = 0;
        char bar[32];
        int pos = 0;
        for (int i = 0; i < filled && pos < 20; i++) bar[pos++] = '=';
        for (int i = filled; i < bar_width && pos < 20; i++) bar[pos++] = ' ';
        bar[pos] = '\0';
        snprintf(progress_str, sizeof(progress_str), " [%s]", bar);
    }

    const char *keys = (spinner != SPINNER_OFF) ? "q:quit" : "";
    int keys_vis = (int)visible_strlen(keys);

    char lhs[512];
    if (eof) {
        if (byte_str[0]) {
            snprintf(lhs, sizeof(lhs), "Done. (%zu lines, %s)%s",
                     line_count, byte_str, progress_str);
        } else {
            snprintf(lhs, sizeof(lhs), "Done. (%zu lines)%s",
                     line_count, progress_str);
        }
    } else {
        if (byte_str[0]) {
            snprintf(lhs, sizeof(lhs), "%s Streaming... (%zu lines, %s)%s",
                     spinner_str, line_count, byte_str, progress_str);
        } else {
            snprintf(lhs, sizeof(lhs), "%s Streaming... (%zu lines)%s",
                     spinner_str, line_count, progress_str);
        }
    }

    if (width <= 0) {
        size_t total = strlen(lhs) + (keys_vis > 0 ? 2 + strlen(keys) : 0);
        char *r = malloc(total + 1);
        if (!r) return strdup(lhs);
        if (keys_vis > 0) snprintf(r, total + 1, "%s  %s", lhs, keys);
        else { memcpy(r, lhs, strlen(lhs) + 1); }
        return r;
    }

    int lhs_vis = (int)visible_strlen(lhs);
    size_t lhs_byte_len = strlen(lhs);
    size_t keys_byte_len = keys_vis > 0 ? 2 + strlen(keys) : 0;
    int gap = lhs_vis + (keys_vis > 0 ? 2 + keys_vis : 0) >= width ? 0 : width - lhs_vis - (keys_vis > 0 ? 2 + keys_vis : 0);
    size_t buf_size = lhs_byte_len + gap + keys_byte_len + 1;
    char *result = malloc(buf_size);
    if (!result) return strdup(lhs);

    if (lhs_vis + (keys_vis > 0 ? 2 + keys_vis : 0) >= width) {
        trim_line(lhs, result, buf_size, width, "");
    } else {
        memcpy(result, lhs, lhs_byte_len + 1);
        size_t slen = lhs_byte_len;
        for (int i = 0; i < gap; i++) result[slen++] = ' ';
        if (keys_vis > 0) {
            result[slen++] = ' ';
            result[slen++] = ' ';
            memcpy(result + slen, keys, strlen(keys));
            slen += strlen(keys);
        }
        result[slen] = '\0';
    }

    return result;
}
