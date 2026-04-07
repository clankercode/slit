/*** 
 * buffer.c - Ring buffer implementation
 * 
 * Fixed-capacity circular buffer that evicts oldest entries when full.
 * Stores line_entry pointers and manages memory for text via strdup.
 */

#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "slit.h"

struct ring_buffer *buffer_create(size_t capacity) {
    struct ring_buffer *b = malloc(sizeof(struct ring_buffer));
    if (!b) return NULL;
    
    b->entries = malloc(capacity * sizeof(struct line_entry *));
    if (!b->entries) {
        free(b);
        return NULL;
    }
    
    b->capacity = capacity;
    b->head = 0;
    b->count = 0;
    b->total_lines = 0;
    b->total_bytes = 0;
    
    return b;
}

void buffer_push(struct ring_buffer *b, struct line_entry *entry) {
    if (!b || !entry) return;
    
    // If buffer is full, free the oldest entry (at head)
    if (b->count == b->capacity) {
        struct line_entry *old = b->entries[b->head];
        if (old) {
            if (old->text) {
                free(old->text);
            }
            free(old);
        }
        b->head = (b->head + 1) % b->capacity;
        b->count--;
    }
    
    // Calculate where to store new entry
    size_t idx = (b->head + b->count) % b->capacity;
    b->entries[idx] = entry;
    b->count++;
    b->total_lines++;
    if (entry->text) {
        b->total_bytes += strlen(entry->text);
    }
}

struct line_entry *buffer_get(struct ring_buffer *b, size_t index) {
    if (!b || index >= b->count) return NULL;
    
    // index 0 is the oldest entry (at head)
    size_t idx = (b->head + index) % b->capacity;
    return b->entries[idx];
}

size_t buffer_count(struct ring_buffer *b) {
    if (!b) return 0;
    return b->count;
}

size_t buffer_total_lines(struct ring_buffer *b) {
    if (!b) return 0;
    return b->total_lines;
}

size_t buffer_total_bytes(struct ring_buffer *b) {
    if (!b) return 0;
    return b->total_bytes;
}

void buffer_free(struct ring_buffer *b) {
    if (!b) return;
    
    if (b->entries) {
        // Free all stored entries and their text
        for (size_t i = 0; i < b->count; i++) {
            size_t idx = (b->head + i) % b->capacity;
            struct line_entry *entry = b->entries[idx];
            if (entry) {
                if (entry->text) {
                    free(entry->text);
                }
                free(entry);
            }
        }
        free(b->entries);
    }
    
    free(b);
}
