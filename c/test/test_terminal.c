#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../terminal.h"

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_skipped = 0;
static int test_ok;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    test_ok = 1; \
    printf("  Running test_" #name "... "); \
    test_##name(); \
    if (test_ok == 1) { printf("PASSED\n"); tests_passed++; } \
    else if (test_ok == -1) { tests_skipped++; } \
    else { tests_failed++; } \
} while(0)

#define SKIP(msg) do { \
    printf("SKIPPED: %s\n", msg); \
    test_ok = -1; \
    return; \
} while(0)

#define FAIL(msg) do { \
    printf("FAILED: %s\n", msg); \
    test_ok = 0; \
    return; \
} while(0)

#define ASSERT_MSG(cond, msg) do { \
    if (!(cond)) { FAIL(msg); } \
} while(0)

TEST(init_opens_tty) {
    int fd = term_init();
    if (fd < 0) { SKIP("no /dev/tty available"); }
    term_cleanup();
}

TEST(init_returns_same_fd) {
    int fd1 = term_init();
    if (fd1 < 0) { SKIP("no /dev/tty"); }
    int fd2 = term_init();
    ASSERT_MSG(fd1 == fd2, "expected same fd on re-init");
    term_cleanup();
}

TEST(cleanup_closes_fd) {
    int fd = term_init();
    if (fd < 0) { SKIP("no /dev/tty"); }
    term_cleanup();
    term_cleanup();
}

TEST(stdin_tty_check) {
    is_stdin_tty();
}

TEST(stderr_tty_check) {
    is_stderr_tty();
}

TEST(get_size_returns_valid) {
    int fd = term_init();
    if (fd < 0) { SKIP("no /dev/tty"); }
    int rows, cols;
    term_get_size(fd, &rows, &cols);
    ASSERT_MSG(rows > 0, "expected rows > 0");
    ASSERT_MSG(cols > 0, "expected cols > 0");
    term_cleanup();
}

TEST(get_size_defaults_on_bad_fd) {
    int rows = 0, cols = 0;
    term_get_size(-1, &rows, &cols);
    ASSERT_MSG(rows == 24, "expected default rows=24");
    ASSERT_MSG(cols == 80, "expected default cols=80");
}

TEST(raw_mode_idempotent) {
    int fd = term_init();
    if (fd < 0) { SKIP("no /dev/tty"); }
    term_raw_enter(fd);
    term_raw_enter(fd);
    term_raw_restore();
    term_raw_restore();
    term_cleanup();
}

TEST(multiple_cleanup_safe) {
    int fd = term_init();
    if (fd < 0) { SKIP("no /dev/tty"); }
    term_cleanup();
    term_cleanup();
}

int main(void) {
    printf("Running terminal tests...\n\n");

    RUN_TEST(init_opens_tty);
    RUN_TEST(init_returns_same_fd);
    RUN_TEST(cleanup_closes_fd);
    RUN_TEST(stdin_tty_check);
    RUN_TEST(stderr_tty_check);
    RUN_TEST(get_size_returns_valid);
    RUN_TEST(get_size_defaults_on_bad_fd);
    RUN_TEST(raw_mode_idempotent);
    RUN_TEST(multiple_cleanup_safe);

    printf("\n");
    printf("==============================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Tests skipped: %d\n", tests_skipped);
    printf("==============================\n");

    return tests_failed > 0 ? 1 : 0;
}
