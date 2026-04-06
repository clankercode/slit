#ifndef LAYOUT_H
#define LAYOUT_H

#include "slit.h"
#include <stddef.h>

struct layout_chrome {
    int top_lines;
    int bottom_lines;
    int side_width;
};

struct layout_chrome layout_get_chrome(enum layout_type type);
int layout_content_width(enum layout_type type, int term_width);
int layout_total_height(enum layout_type type, int content_lines);
void layout_render(enum layout_type type, int term_width, const char *title,
                   const char **content_lines, int content_count,
                   const char *status, const char *quote_bg);
int layout_data_lines(enum layout_type type, int content_lines);

#endif
