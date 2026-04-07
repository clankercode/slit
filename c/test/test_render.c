#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../render.h"

static int failures = 0;

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while(0)

#define ASSERT_STREQ(a, b) ASSERT(strcmp((a), (b)) == 0)

static void test_visible_strlen_plain(void) {
    ASSERT(visible_strlen("hello") == 5);
    ASSERT(visible_strlen("") == 0);
    ASSERT(visible_strlen("abc def") == 7);
}

static void test_visible_strlen_ansi(void) {
    ASSERT(visible_strlen("\x1b[31mred\x1b[0m") == 3);
    ASSERT(visible_strlen("\x1b[1;32;40mbold\x1b[0m") == 4);
    ASSERT(visible_strlen("\x1b[0m") == 0);
}

static void test_strip_ansi(void) {
    char dst[256];
    strip_ansi("\x1b[31mred\x1b[0m normal", dst, sizeof(dst));
    ASSERT_STREQ(dst, "red normal");

    strip_ansi("no ansi here", dst, sizeof(dst));
    ASSERT_STREQ(dst, "no ansi here");

    strip_ansi("", dst, sizeof(dst));
    ASSERT_STREQ(dst, "");
}

static void test_trim_line_plain(void) {
    char dst[256];
    trim_line("hello world", dst, sizeof(dst), 5, "...");
    ASSERT_STREQ(dst, "he...");

    trim_line("hi", dst, sizeof(dst), 10, "...");
    ASSERT_STREQ(dst, "hi");

    trim_line("hello", dst, sizeof(dst), 5, "...");
    ASSERT_STREQ(dst, "hello");
}

static void test_trim_line_ansi(void) {
    char dst[256];
    trim_line("\x1b[31mhello world\x1b[0m", dst, sizeof(dst), 5, "...");
    ASSERT(strlen(dst) > 0);

    int vis = (int)visible_strlen(dst);
    ASSERT(vis == 5);
}

static void test_trim_line_truncation_char(void) {
    char dst[256];
    trim_line("hello world", dst, sizeof(dst), 8, "...");
    ASSERT_STREQ(dst, "hello...");

    trim_line("abcdefghij", dst, sizeof(dst), 7, "~");
    ASSERT(visible_strlen(dst) == 7);
}

static void test_wrap_line_basic(void) {
    char **lines = NULL;
    int count = 0;

    wrap_line("hello world", 5, &lines, &count);
    ASSERT(count == 3);
    ASSERT_STREQ(lines[0], "hello");
    ASSERT_STREQ(lines[1], " worl");
    ASSERT_STREQ(lines[2], "d");
    wrap_lines_free(lines, count);
}

static void test_wrap_line_short(void) {
    char **lines = NULL;
    int count = 0;

    wrap_line("hi", 10, &lines, &count);
    ASSERT(count == 1);
    ASSERT_STREQ(lines[0], "hi");
    wrap_lines_free(lines, count);
}

static void test_wrap_line_empty(void) {
    char **lines = NULL;
    int count = 0;

    wrap_line("", 10, &lines, &count);
    ASSERT(count == 1);
    ASSERT_STREQ(lines[0], "");
    wrap_lines_free(lines, count);
}

static void test_wrap_line_exact(void) {
    char **lines = NULL;
    int count = 0;

    wrap_line("hello", 5, &lines, &count);
    ASSERT(count == 1);
    ASSERT_STREQ(lines[0], "hello");
    wrap_lines_free(lines, count);
}

static void make_maze_line(char *buf, size_t buf_size, int width) {
    size_t pos = 0;
    for (int x = 0; x < width && pos + 32 < buf_size; x++) {
        int r = (x * 37 + 100) % 256;
        int g = (x * 73 + 50) % 256;
        int b = (x * 113 + 150) % 256;
        const char *ch = (x % 2 == 0) ? "\xe2\x95\xb1" : "\xe2\x95\xb2";
        pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                "\x1b[38;2;%d;%d;%dm%s\x1b[0m", r, g, b, ch);
    }
}

static void test_bug2_trim_ansi_content(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 76);
    char dst[8192];
    trim_line(maze, dst, sizeof(dst), 76, "");
    ASSERT(visible_strlen(dst) == 76);
}

static void test_bug2_trim_ansi_truncation(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 80);
    char dst[8192];
    trim_line(maze, dst, sizeof(dst), 40, "");
    ASSERT(visible_strlen(dst) <= 40);
}

static void test_bug3_wrap_reset_detection(void) {
    char **lines = NULL;
    int count = 0;
    wrap_line("\x1b[31mhello\x1b[0m", 5, &lines, &count);
    ASSERT(count == 1);
    ASSERT(visible_strlen(lines[0]) == 5);
    wrap_lines_free(lines, count);
}

static void test_bug3_wrap_no_phantom_line(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 80);
    char **lines = NULL;
    int count = 0;
    wrap_line(maze, 40, &lines, &count);
    ASSERT(count == 2);
    ASSERT(visible_strlen(lines[0]) == 40);
    ASSERT(visible_strlen(lines[1]) == 40);
    wrap_lines_free(lines, count);
}

static void test_bug3_wrap_no_phantom_multiple_widths(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 80);
    int widths[] = {10, 20, 40, 80};
    for (int w = 0; w < 4; w++) {
        char **lines = NULL;
        int count = 0;
        wrap_line(maze, (size_t)widths[w], &lines, &count);
        int expected = 80 / widths[w];
        ASSERT(count == expected);
        wrap_lines_free(lines, count);
    }
}

static void test_bug3_wrap_width79(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 80);
    char **lines = NULL;
    int count = 0;
    wrap_line(maze, 79, &lines, &count);
    ASSERT(count == 2);
    ASSERT(visible_strlen(lines[0]) == 79);
    ASSERT(visible_strlen(lines[1]) == 1);
    wrap_lines_free(lines, count);
}

static void test_bug4_trim_no_dangling_ansi(void) {
    char maze[8192];
    make_maze_line(maze, sizeof(maze), 80);
    char dst[8192];
    trim_line(maze, dst, sizeof(dst), 5, "\xe2\x80\xa6");
    ASSERT(visible_strlen(dst) == 5);
    char stripped[8192];
    strip_ansi(dst, stripped, sizeof(stripped));
    size_t slen = strlen(stripped);
    ASSERT(slen > 0);
    unsigned char last[4] = {0};
    size_t li = slen;
    if (li >= 3 && (unsigned char)stripped[li-3] == 0xe2) {
        memcpy(last, stripped + li - 3, 3);
    } else if (li >= 1) {
        last[0] = (unsigned char)stripped[li-1];
    }
    int is_trunc = (last[0] == 0xe2 && last[1] == 0x80 && last[2] == 0xa6);
    ASSERT(is_trunc);
}

static void test_bug5_visible_width_wide_char(void) {
    ASSERT(visible_strlen("\xe4\xbd\xa0\xe5\xa5\xbd") == 4);
}

static void test_bug5_visible_width_mixed(void) {
    ASSERT(visible_strlen("a\xe4\xbd\xa0" "b\xe5\xa5\xbd" "c") == 7);
}

static void test_bug5_visible_width_maze_char(void) {
    ASSERT(visible_strlen("\xe2\x95\xb1\xe2\x95\xb2") == 2);
}

static void test_bug5_trim_wide_char(void) {
    char dst[256];
    trim_line("a\xe4\xbd\xa0" "b\xe5\xa5\xbd" "cdef", dst, sizeof(dst), 5, "\xe2\x80\xa6");
    ASSERT(visible_strlen(dst) == 5);
}

static void test_bug5_wrap_wide_char(void) {
    const char *input = "a\xe4\xbd\xa0" "b\xe5\xa5\xbd" "cdef";
    size_t total = visible_strlen(input);
    char **lines = NULL;
    int count = 0;
    wrap_line(input, 5, &lines, &count);
    ASSERT(count >= 2);
    size_t total_vis = 0;
    for (int i = 0; i < count; i++) {
        total_vis += visible_strlen(lines[i]);
    }
    ASSERT(total_vis == total);
    wrap_lines_free(lines, count);
}

static void test_bug6_osc_st_terminator_visible_strlen(void) {
    ASSERT(visible_strlen("\x1b]0;title\x1b\\text") == 4);
    ASSERT(visible_strlen("\x1b]0;title\x07text") == 4);
}

static void test_bug6_osc_st_terminator_strip_ansi(void) {
    char dst[256];
    strip_ansi("\x1b]0;title\x1b\\hello", dst, sizeof(dst));
    ASSERT_STREQ(dst, "hello");
}

static void test_bug6_osc_st_terminator_trim_line(void) {
    char dst[256];
    trim_line("\x1b]0;title\x1b\\hello world", dst, sizeof(dst), 5, "...");
    ASSERT(visible_strlen(dst) == 5);
}

static void test_bug7_wrap_sgr_reset_leading_zeros(void) {
    char **lines = NULL;
    int count = 0;
    wrap_line("\x1b[31mhello\x1b[00m world", 5, &lines, &count);
    ASSERT(count == 3);
    ASSERT(visible_strlen(lines[0]) == 5);
    ASSERT(visible_strlen(lines[1]) == 5);
    ASSERT(visible_strlen(lines[2]) == 1);
    char stripped[256];
    strip_ansi(lines[1], stripped, sizeof(stripped));
    ASSERT_STREQ(stripped, " worl");
    wrap_lines_free(lines, count);
}

static void test_bug7_wrap_sgr_reset_empty_params(void) {
    char **lines = NULL;
    int count = 0;
    wrap_line("\x1b[31mhello\x1b[m world", 5, &lines, &count);
    ASSERT(count == 3);
    ASSERT(visible_strlen(lines[0]) == 5);
    ASSERT(visible_strlen(lines[1]) == 5);
    ASSERT(visible_strlen(lines[2]) == 1);
    char stripped[256];
    strip_ansi(lines[1], stripped, sizeof(stripped));
    ASSERT_STREQ(stripped, " worl");
    wrap_lines_free(lines, count);
}

static void test_bug7_wrap_sgr_reset_multiple_zeros(void) {
    char **lines = NULL;
    int count = 0;
    wrap_line("\x1b[31mhello\x1b[000m world", 5, &lines, &count);
    ASSERT(count == 3);
    ASSERT(visible_strlen(lines[0]) == 5);
    ASSERT(visible_strlen(lines[1]) == 5);
    ASSERT(visible_strlen(lines[2]) == 1);
    char stripped[256];
    strip_ansi(lines[1], stripped, sizeof(stripped));
    ASSERT_STREQ(stripped, " worl");
    wrap_lines_free(lines, count);
}

int main(void) {
    test_visible_strlen_plain();
    test_visible_strlen_ansi();
    test_strip_ansi();
    test_trim_line_plain();
    test_trim_line_ansi();
    test_trim_line_truncation_char();
    test_wrap_line_basic();
    test_wrap_line_short();
    test_wrap_line_empty();
    test_wrap_line_exact();

    test_bug2_trim_ansi_content();
    test_bug2_trim_ansi_truncation();
    test_bug3_wrap_reset_detection();
    test_bug3_wrap_no_phantom_line();
    test_bug3_wrap_no_phantom_multiple_widths();
    test_bug3_wrap_width79();
    test_bug4_trim_no_dangling_ansi();
    test_bug5_visible_width_wide_char();
    test_bug5_visible_width_mixed();
    test_bug5_visible_width_maze_char();
    test_bug5_trim_wide_char();
    test_bug5_wrap_wide_char();

    test_bug6_osc_st_terminator_visible_strlen();
    test_bug6_osc_st_terminator_strip_ansi();
    test_bug6_osc_st_terminator_trim_line();
    test_bug7_wrap_sgr_reset_leading_zeros();
    test_bug7_wrap_sgr_reset_empty_params();
    test_bug7_wrap_sgr_reset_multiple_zeros();

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All render tests passed\n");
    return 0;
}
