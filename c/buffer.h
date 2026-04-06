/*** 
 * buffer.h - Ring buffer for line entries
 * 
 * A fixed-capacity circular buffer that stores line_entry pointers.
 * When full, oldest entries are evicted to make room for new ones.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include "slit.h"

struct ring_buffer {
    struct line_entry **entries;
    size_t capacity;
    size_t head;
    size_t count;
    size_t total_lines;
    size_t total_bytes;
};

struct ring_buffer *buffer_create(size_t capacity);
void buffer_push(struct ring_buffer *b, struct line_entry *entry);
struct line_entry *buffer_get(struct ring_buffer *b, size_t index);
size_t buffer_count(struct ring_buffer *b);
size_t buffer_total_lines(struct ring_buffer *b);
size_t buffer_total_bytes(struct ring_buffer *b);
void buffer_free(struct ring_buffer *b);

#endif /* BUFFER_H */
