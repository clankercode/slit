/*** 
 * test_terminal.c - Unit tests for terminal control
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../terminal.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("  Running test_" #name "... "); \
    test_##name(); \
    printf("PASSED\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAILED: %s\n", msg); \
    tests_failed++; \
    return; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) FAIL(#cond); \
} while(0)

TEST(init_opens_tty) {
    int fd = term_init();
    ASSERT(fd >= 0);
    term_cleanup();
}

TEST(init_returns_same_fd) {
    int fd1 = term_init();
    int fd2 = term_init();
    ASSERT(fd1 == fd2);
    term_cleanup();
}

TEST(cleanup_closes_fd) {
    int fd = term_init();
    ASSERT(fd >= 0);
    term_cleanup();
    /* Cleanup should close the fd, next init should get new fd */
    int fd2 = term_init();
    ASSERT(fd2 >= 0);
    term_cleanup();
}

TEST(stdin_tty_check) {
    /* Just check it doesn't crash and returns 0 or 1 */
    int result = is_stdin_tty();
    ASSERT(result == 0 || result == 1);
}

TEST(stderr_tty_check) {
    /* Just check it doesn't crash and returns 0 or 1 */
    int result = is_stderr_tty();
    ASSERT(result == 0 || result == 1);
}

TEST(get_size_returns_valid) {
    int fd = term_init();
    ASSERT(fd >= 0);
    
    int rows, cols;
    term_get_size(fd, &rows, &cols);
    
    /* Should return reasonable values (at least 1x1) */
    ASSERT(rows > 0);
    ASSERT(cols > 0);
    
    term_cleanup();
}

TEST(get_size_defaults_on_bad_fd) {
    int rows = 0, cols = 0;
    term_get_size(-1, &rows, &cols);
    
    /* Should return defaults on bad fd */
    ASSERT(rows == 24);
    ASSERT(cols == 80);
}

TEST(raw_mode_idempotent) {
    int fd = term_init();
    ASSERT(fd >= 0);
    
    /* Enter raw mode */
    term_raw_enter(fd);
    
    /* Should be idempotent - calling again shouldn't crash */
    term_raw_enter(fd);
    
    /* Restore */
    term_raw_restore();
    
    /* Should be idempotent - restoring when not raw shouldn't crash */
    term_raw_restore();
    
    term_cleanup();
}

TEST(multiple_cleanup_safe) {
    int fd = term_init();
    ASSERT(fd >= 0);
    
    term_cleanup();
    term_cleanup();  /* Should not crash */
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
    printf("==============================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
