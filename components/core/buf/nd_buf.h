#ifndef ND_CORE_BUF_H
#define ND_CORE_BUF_H

#include <stddef.h>
#include <stdint.h>

/*
 * A chain node for link-layer frames and application blocks. The block does
 * not allocate; the producer owns the storage and supplies the release hook.
 */
typedef struct nd_frame {
    uint8_t *data;
    size_t len;
    struct nd_frame *next;
    void *owner;
    void (*release)(struct nd_frame *frame);
} nd_frame_t;

typedef struct nd_block {
    nd_frame_t *first;
    nd_frame_t *last;
    size_t total_len;
} nd_block_t;

typedef struct nd_cursor {
    const nd_frame_t *frame;
    size_t offset;
    size_t remaining;
} nd_cursor_t;

void nd_block_init(nd_block_t *block);
int nd_block_append(nd_block_t *block, nd_frame_t *frame);
int nd_block_truncate(nd_block_t *block, size_t length);
size_t nd_block_copy(const nd_block_t *block, uint8_t *dst, size_t capacity);
void nd_block_release(nd_block_t *block);

void nd_block_cursor(const nd_block_t *block, nd_cursor_t *cursor);
size_t nd_cursor_read(nd_cursor_t *cursor, uint8_t *dst, size_t len);
size_t nd_cursor_skip(nd_cursor_t *cursor, size_t len);
size_t nd_cursor_remaining(const nd_cursor_t *cursor);

#endif
