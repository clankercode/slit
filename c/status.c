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
    char progress_str[64] = "";

    if (file_size > 0) {
        int bar_width = 10;
        int filled = (int)((double)bar_width * (double)total_bytes / (double)file_size);
        if (filled > bar_width) filled = bar_width;
        if (filled < 0) filled = 0;
        char bar[12];
        int pos = 0;
        for (int i = 0; i < filled; i++) bar[pos++] = '=';
        for (int i = filled; i < bar_width; i++) bar[pos++] = ' ';
        bar[pos] = '\0';
        snprintf(progress_str, sizeof(progress_str), " [%s]", bar);
    }

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

    const char *keys = (spinner != SPINNER_OFF) ? "  q:quit" : "";

    char line[1024];
    if (eof) {
        if (byte_str[0]) {
            snprintf(line, sizeof(line), "Done. (%zu lines, %s)%s%s",
                     line_count, byte_str, progress_str, keys);
        } else {
            snprintf(line, sizeof(line), "Done. (%zu lines)%s%s",
                     line_count, progress_str, keys);
        }
    } else {
        if (byte_str[0]) {
            snprintf(line, sizeof(line), "%s Streaming... (%zu lines, %s)%s%s",
                     spinner_str, line_count, byte_str, progress_str, keys);
        } else {
            snprintf(line, sizeof(line), "%s Streaming... (%zu lines)%s%s",
                     spinner_str, line_count, progress_str, keys);
        }
    }

    if (width <= 0) {
        return strdup(line);
    }

    int vis = (int)visible_strlen(line);
    char *result = malloc(width + 1);
    if (!result) return strdup(line);

    if (vis >= width) {
        trim_line(line, result, width + 1, width, "");
    } else {
        strcpy(result, line);
        int pad = width - vis;
        size_t slen = strlen(result);
        for (int i = 0; i < pad; i++) result[slen + i] = ' ';
        result[slen + pad] = '\0';
    }

    return result;
}
