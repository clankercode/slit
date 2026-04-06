#include "layout.h"
#include "render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct layout_chrome layout_get_chrome(enum layout_type type) {
    switch (type) {
        case LAYOUT_BOX:     return (struct layout_chrome){1, 1, 4};
        case LAYOUT_ROUNDED: return (struct layout_chrome){1, 1, 4};
        case LAYOUT_COMPACT: return (struct layout_chrome){1, 1, 0};
        case LAYOUT_MINIMAL: return (struct layout_chrome){0, 1, 0};
        case LAYOUT_NONE:    return (struct layout_chrome){0, 0, 0};
        case LAYOUT_QUOTE:   return (struct layout_chrome){0, 0, 2};
    }
    return (struct layout_chrome){0, 1, 0};
}

int layout_content_width(enum layout_type type, int term_width) {
    struct layout_chrome c = layout_get_chrome(type);
    int w = term_width - c.side_width;
    return w > 0 ? w : 1;
}

int layout_total_height(enum layout_type type, int content_lines) {
    struct layout_chrome c = layout_get_chrome(type);
    return content_lines + c.top_lines + c.bottom_lines;
}

int layout_data_lines(enum layout_type type, int content_lines) {
    if (type == LAYOUT_QUOTE) {
        int data = content_lines - 2;
        return data > 0 ? data : 0;
    }
    return content_lines;
}

static void pad_to(FILE *fp, const char *s, int width) {
    if (!s) s = "";
    size_t vis = visible_strlen(s);
    if ((int)vis > width) {
        char tmp[4096];
        trim_line(s, tmp, sizeof(tmp), (size_t)width, "");
        fputs(tmp, fp);
        return;
    }
    fputs(s, fp);
    if ((int)vis < width) {
        for (int i = (int)vis; i < width; i++) fputc(' ', fp);
    }
}

static void render_box(enum layout_type type, int width, const char *title,
                       const char **content_lines, int content_count,
                       const char *status) {
    const char *tl, *tr, *bl, *br;
    if (type == LAYOUT_ROUNDED) {
        tl = "\xe2\x95\xad"; tr = "\xe2\x95\xae";
        bl = "\xe2\x95\xb0"; br = "\xe2\x95\xaf";
    } else {
        tl = "\xe2\x94\x8c"; tr = "\xe2\x94\x90";
        bl = "\xe2\x94\x94"; br = "\xe2\x94\x98";
    }
    const char *h = "\xe2\x94\x80";
    const char *v = "\xe2\x94\x82";

    int content_width = width - 4;
    if (content_width < 1) content_width = 1;

    int title_vis = title ? (int)visible_strlen(title) : 0;
    int title_total = title_vis > 0 ? title_vis + 2 : 0;
    int inner = width - 2;
    int remaining = inner - title_total;
    if (remaining < 0) remaining = 0;

    fprintf(stderr, "%s", tl);
    if (title_total > 0) {
        fprintf(stderr, " %s ", title);
    }
    for (int i = 0; i < remaining; i++) fprintf(stderr, "%s", h);
    fprintf(stderr, "%s\n", tr);

    for (int i = 0; i < content_count; i++) {
        fprintf(stderr, "%s ", v);
        pad_to(stderr, content_lines[i], content_width);
        fprintf(stderr, " %s\n", v);
    }

    int status_vis = status ? (int)visible_strlen(status) : 0;
    int status_total = status_vis > 0 ? status_vis + 2 : 0;
    remaining = inner - status_total;
    if (remaining < 0) remaining = 0;

    fprintf(stderr, "%s", bl);
    if (status_total > 0) {
        fprintf(stderr, " %s ", status);
    }
    for (int i = 0; i < remaining; i++) fprintf(stderr, "%s", h);
    fprintf(stderr, "%s\n", br);
}

static void render_compact(int width, const char *title,
                           const char **content_lines, int content_count,
                           const char *status) {
    char *top_buf = malloc(width + 32);
    if (top_buf) {
        int title_vis = title ? (int)visible_strlen(title) : 0;
        int title_len = title ? (int)strlen(title) : 0;
        int pos = 0;
        memcpy(top_buf, "\x1b[7m\x1b[1m", pos = 7);
        if (title && title_len > 0) {
            memcpy(top_buf + pos, title, title_len);
            pos += title_len;
        }
        int pad = width - title_vis;
        for (int i = 0; i < pad && pos < width + 30; i++) top_buf[pos++] = ' ';
        memcpy(top_buf + pos, "\x1b[0m", 4);
        pos += 4;
        top_buf[pos] = '\0';
        fprintf(stderr, "%s\n", top_buf);
        free(top_buf);
    }

    for (int i = 0; i < content_count; i++) {
        fprintf(stderr, "%s\n", content_lines[i] ? content_lines[i] : "");
    }

    if (status) {
        fprintf(stderr, "%s\n", status);
    }
}

static void render_minimal(const char **content_lines, int content_count,
                           const char *status) {
    for (int i = 0; i < content_count; i++) {
        fprintf(stderr, "%s\n", content_lines[i] ? content_lines[i] : "");
    }
    if (status) {
        fprintf(stderr, "%s\n", status);
    }
}

static void render_none(const char **content_lines, int content_count) {
    for (int i = 0; i < content_count; i++) {
        fprintf(stderr, "%s\n", content_lines[i] ? content_lines[i] : "");
    }
}

static void render_quote(int width, const char *title,
                         const char **content_lines, int content_count,
                         const char *status, const char *quote_bg) {
    const char *bar = "\xe2\x96\x8c";
    int use_bg = 0;
    int bg_r = 0, bg_g = 0, bg_b = 0;

    if (quote_bg && strcmp(quote_bg, "off") != 0 && quote_bg[0] == '#') {
        use_bg = 1;
        if (strlen(quote_bg) >= 7) {
            unsigned int r, g, b;
            sscanf(quote_bg + 1, "%2x%2x%2x", &r, &g, &b);
            bg_r = r; bg_g = g; bg_b = b;
        }
    }

    if (title) {
        if (use_bg) fprintf(stderr, "\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);
        fprintf(stderr, "%s %s", bar, title);
        int vis = (int)visible_strlen(title) + 2;
        if (use_bg) {
            for (int i = vis; i < width; i++) fputc(' ', stderr);
            fprintf(stderr, "\x1b[0m");
        }
        fputc('\n', stderr);
    }

    for (int i = 0; i < content_count; i++) {
        if (use_bg) fprintf(stderr, "\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);
        fprintf(stderr, "%s %s", bar, content_lines[i] ? content_lines[i] : "");
        if (use_bg) {
            int vis = (int)visible_strlen(content_lines[i] ? content_lines[i] : "") + 2;
            for (int j = vis; j < width; j++) fputc(' ', stderr);
            fprintf(stderr, "\x1b[0m");
        }
        fputc('\n', stderr);
    }

    if (status) {
        if (use_bg) fprintf(stderr, "\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);
        fprintf(stderr, "%s %s", bar, status);
        if (use_bg) {
            int vis = (int)visible_strlen(status) + 2;
            for (int i = vis; i < width; i++) fputc(' ', stderr);
            fprintf(stderr, "\x1b[0m");
        }
        fputc('\n', stderr);
    }
}

void layout_render(enum layout_type type, int term_width, const char *title,
                   const char **content_lines, int content_count,
                   const char *status, const char *quote_bg) {
    (void)term_width;
    switch (type) {
        case LAYOUT_BOX:
        case LAYOUT_ROUNDED:
            render_box(type, term_width, title, content_lines, content_count, status);
            break;
        case LAYOUT_COMPACT:
            render_compact(term_width, title, content_lines, content_count, status);
            break;
        case LAYOUT_MINIMAL:
            render_minimal(content_lines, content_count, status);
            break;
        case LAYOUT_NONE:
            render_none(content_lines, content_count);
            break;
        case LAYOUT_QUOTE:
            render_quote(term_width, title, content_lines, content_count, status, quote_bg);
            break;
    }
}
