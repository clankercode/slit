/*** 
 * test_buffer.c - Unit tests for ring buffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../buffer.h"
#include "../slit.h"

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

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAILED: Expected %zu, got %zu\n", (size_t)(b), (size_t)(a)); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("FAILED: Expected '%s', got '%s'\n", (b), (a)); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* Create a line entry with duplicated text */
static struct line_entry *make_entry(const char *text, size_t line_num) {
    struct line_entry *e = malloc(sizeof(struct line_entry));
    if (!e) return NULL;
    e->text = strdup(text);
    e->line_num = line_num;
    e->byte_len = strlen(text);
    e->arrival = 0;
    return e;
}

TEST(create_destroy) {
    struct ring_buffer *b = buffer_create(10);
    ASSERT(b != NULL);
    ASSERT_EQ(buffer_count(b), 0);
    ASSERT_EQ(buffer_total_lines(b), 0);
    ASSERT_EQ(buffer_total_bytes(b), 0);
    buffer_free(b);
}

TEST(single_push) {
    struct ring_buffer *b = buffer_create(10);
    struct line_entry *e = make_entry("hello", 1);
    buffer_push(b, e);
    ASSERT_EQ(buffer_count(b), 1);
    ASSERT_EQ(buffer_total_lines(b), 1);
    ASSERT_EQ(buffer_total_bytes(b), 5);
    buffer_free(b);
}

TEST(multiple_push) {
    struct ring_buffer *b = buffer_create(10);
    for (int i = 0; i < 5; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "line%d", i);
        buffer_push(b, make_entry(buf, i + 1));
    }
    ASSERT_EQ(buffer_count(b), 5);
    ASSERT_EQ(buffer_total_lines(b), 5);
    ASSERT_EQ(buffer_total_bytes(b), 25); // "line0" through "line4" = 5 * 5
    buffer_free(b);
}

TEST(buffer_get) {
    struct ring_buffer *b = buffer_create(10);
    buffer_push(b, make_entry("first", 1));
    buffer_push(b, make_entry("second", 2));
    buffer_push(b, make_entry("third", 3));
    
    struct line_entry *e0 = buffer_get(b, 0);
    struct line_entry *e1 = buffer_get(b, 1);
    struct line_entry *e2 = buffer_get(b, 2);
    
    ASSERT_STR_EQ(e0->text, "first");
    ASSERT_EQ(e0->line_num, 1);
    ASSERT_STR_EQ(e1->text, "second");
    ASSERT_EQ(e1->line_num, 2);
    ASSERT_STR_EQ(e2->text, "third");
    ASSERT_EQ(e2->line_num, 3);
    
    // Out of bounds should return NULL
    ASSERT(buffer_get(b, 3) == NULL);
    ASSERT(buffer_get(b, 100) == NULL);
    
    buffer_free(b);
}

TEST(overflow_eviction) {
    struct ring_buffer *b = buffer_create(3);
    buffer_push(b, make_entry("one", 1));
    buffer_push(b, make_entry("two", 2));
    buffer_push(b, make_entry("three", 3));
    buffer_push(b, make_entry("four", 4)); // Evicts "one"
    
    ASSERT_EQ(buffer_count(b), 3);
    ASSERT_EQ(buffer_total_lines(b), 4);
    
    // Oldest entry should now be "two"
    struct line_entry *e0 = buffer_get(b, 0);
    ASSERT_STR_EQ(e0->text, "two");
    ASSERT_EQ(e0->line_num, 2);
    
    // Newest should be "four"
    struct line_entry *e2 = buffer_get(b, 2);
    ASSERT_STR_EQ(e2->text, "four");
    ASSERT_EQ(e2->line_num, 4);
    
    buffer_free(b);
}

TEST(complete_wraparound) {
    struct ring_buffer *b = buffer_create(3);
    buffer_push(b, make_entry("a", 1));
    buffer_push(b, make_entry("b", 2));
    buffer_push(b, make_entry("c", 3));
    buffer_push(b, make_entry("d", 4)); // Evicts "a"
    buffer_push(b, make_entry("e", 5)); // Evicts "b"
    buffer_push(b, make_entry("f", 6)); // Evicts "c"
    
    ASSERT_EQ(buffer_count(b), 3);
    ASSERT_EQ(buffer_total_lines(b), 6);
    
    ASSERT_STR_EQ(buffer_get(b, 0)->text, "d");
    ASSERT_STR_EQ(buffer_get(b, 1)->text, "e");
    ASSERT_STR_EQ(buffer_get(b, 2)->text, "f");
    
    buffer_free(b);
}

TEST(empty_buffer_get) {
    struct ring_buffer *b = buffer_create(5);
    ASSERT(buffer_get(b, 0) == NULL);
    buffer_free(b);
}

TEST(total_bytes_tracking) {
    struct ring_buffer *b = buffer_create(3);
    buffer_push(b, make_entry("hello", 1));      // 5 bytes
    ASSERT_EQ(buffer_total_bytes(b), 5);
    
    buffer_push(b, make_entry("world", 2));      // +5 = 10 bytes
    ASSERT_EQ(buffer_total_bytes(b), 10);
    
    buffer_push(b, make_entry("!", 3));          // +1 = 11 bytes
    ASSERT_EQ(buffer_total_bytes(b), 11);
    
    buffer_push(b, make_entry("extra", 4));      // Evicts "hello" (5 bytes), adds "extra" (5 bytes)
    ASSERT_EQ(buffer_total_bytes(b), 16);        // Cumulative: 5+5+1+5 = 16
    
    buffer_free(b);
}

TEST(capacity_one) {
    struct ring_buffer *b = buffer_create(1);
    buffer_push(b, make_entry("first", 1));
    ASSERT_EQ(buffer_count(b), 1);
    ASSERT_STR_EQ(buffer_get(b, 0)->text, "first");
    
    buffer_push(b, make_entry("second", 2));
    ASSERT_EQ(buffer_count(b), 1);
    ASSERT_STR_EQ(buffer_get(b, 0)->text, "second");
    ASSERT_EQ(buffer_total_lines(b), 2);
    
    buffer_free(b);
}

TEST(null_safety) {
    // These should not crash
    buffer_push(NULL, make_entry("test", 1));
    buffer_push(NULL, NULL);
    ASSERT(buffer_count(NULL) == 0);
    ASSERT(buffer_total_lines(NULL) == 0);
    ASSERT(buffer_total_bytes(NULL) == 0);
    ASSERT(buffer_get(NULL, 0) == NULL);
    buffer_free(NULL);
}

TEST(total_bytes_cumulative_after_eviction) {
    struct ring_buffer *b = buffer_create(3);
    buffer_push(b, make_entry("short", 1));                    // 5, cumulative=5
    ASSERT_EQ(buffer_total_bytes(b), 5);
    buffer_push(b, make_entry("a very long line here", 2));    // 21, cumulative=26
    ASSERT_EQ(buffer_total_bytes(b), 26);
    buffer_push(b, make_entry("mid", 3));                      // 3, cumulative=29
    ASSERT_EQ(buffer_total_bytes(b), 29);

    buffer_push(b, make_entry("x", 4));                        // evicts "short", adds "x"(1), cumulative=30
    ASSERT_EQ(buffer_total_bytes(b), 30);

    buffer_push(b, make_entry("yy", 5));                       // evicts "a very long line here", adds "yy"(2), cumulative=32
    ASSERT_EQ(buffer_total_bytes(b), 32);

    buffer_free(b);
}

int main(void) {
    printf("Running ring buffer tests...\n\n");
    
    RUN_TEST(create_destroy);
    RUN_TEST(single_push);
    RUN_TEST(multiple_push);
    RUN_TEST(buffer_get);
    RUN_TEST(overflow_eviction);
    RUN_TEST(complete_wraparound);
    RUN_TEST(empty_buffer_get);
    RUN_TEST(total_bytes_tracking);
    RUN_TEST(capacity_one);
    RUN_TEST(null_safety);
    RUN_TEST(total_bytes_cumulative_after_eviction);
    
    printf("\n");
    printf("==============================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("==============================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
