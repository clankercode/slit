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

    int vis = visible_strlen(dst);
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

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All render tests passed\n");
    return 0;
}
