#include "components/core/buf/nd_buf.h"

#include <string.h>

void nd_block_init(nd_block_t *block)
{
    if (block == NULL) {
        return;
    }
    block->first = NULL;
    block->last = NULL;
    block->total_len = 0;
}

int nd_block_append(nd_block_t *block, nd_frame_t *frame)
{
    if ((block == NULL) || (frame == NULL) || (frame->next != NULL)) {
        return -1;
    }

    if (block->last == NULL) {
        block->first = frame;
    } else {
        block->last->next = frame;
    }
    block->last = frame;
    block->total_len += frame->len;
    return 0;
}

int nd_block_truncate(nd_block_t *block, size_t length)
{
    nd_frame_t *frame;
    size_t remaining;

    if (block == NULL) {
        return -1;
    }
    if (length >= block->total_len) {
        return 0;
    }

    if (length == 0) {
        block->first = NULL;
        block->last = NULL;
        block->total_len = 0;
        return 0;
    }

    frame = block->first;
    remaining = length;
    while ((frame != NULL) && (remaining > frame->len)) {
        remaining -= frame->len;
        frame = frame->next;
    }
    if (frame == NULL) {
        return -1;
    }

    frame->len = remaining;
    frame->next = NULL;
    block->last = frame;
    block->total_len = length;
    return 0;
}

size_t nd_block_copy(const nd_block_t *block, uint8_t *dst, size_t capacity)
{
    size_t copied = 0;
    const nd_frame_t *frame;

    if ((block == NULL) || (dst == NULL)) {
        return 0;
    }

    for (frame = block->first; (frame != NULL) && (copied < capacity);
         frame = frame->next) {
        size_t count = frame->len;

        if (count > capacity - copied) {
            count = capacity - copied;
        }
        memcpy(dst + copied, frame->data, count);
        copied += count;
    }
    return copied;
}

void nd_block_release(nd_block_t *block)
{
    nd_frame_t *frame;

    if (block == NULL) {
        return;
    }

    frame = block->first;
    while (frame != NULL) {
        nd_frame_t *next = frame->next;

        if (frame->release != NULL) {
            frame->release(frame);
        }
        frame = next;
    }
    nd_block_init(block);
}

void nd_block_cursor(const nd_block_t *block, nd_cursor_t *cursor)
{
    if ((block == NULL) || (cursor == NULL)) {
        return;
    }

    cursor->frame = block->first;
    cursor->offset = 0;
    cursor->remaining = block->total_len;
}

size_t nd_cursor_read(nd_cursor_t *cursor, uint8_t *dst, size_t len)
{
    size_t copied = 0;

    if ((cursor == NULL) || (dst == NULL)) {
        return 0;
    }

    while ((copied < len) && (cursor->frame != NULL)
           && (cursor->remaining > 0)) {
        size_t available = cursor->frame->len - cursor->offset;
        size_t count = len - copied;

        if (count > available) {
            count = available;
        }
        if (count > cursor->remaining) {
            count = cursor->remaining;
        }
        memcpy(dst + copied, cursor->frame->data + cursor->offset, count);
        copied += count;
        cursor->offset += count;
        cursor->remaining -= count;

        if (cursor->offset == cursor->frame->len) {
            cursor->frame = cursor->frame->next;
            cursor->offset = 0;
        }
    }
    return copied;
}

size_t nd_cursor_skip(nd_cursor_t *cursor, size_t len)
{
    size_t skipped = 0;

    if (cursor == NULL) {
        return 0;
    }

    while ((skipped < len) && (cursor->frame != NULL)
           && (cursor->remaining > 0)) {
        size_t available = cursor->frame->len - cursor->offset;
        size_t count = len - skipped;

        if (count > available) {
            count = available;
        }
        if (count > cursor->remaining) {
            count = cursor->remaining;
        }
        skipped += count;
        cursor->offset += count;
        cursor->remaining -= count;

        if (cursor->offset == cursor->frame->len) {
            cursor->frame = cursor->frame->next;
            cursor->offset = 0;
        }
    }
    return skipped;
}

size_t nd_cursor_remaining(const nd_cursor_t *cursor)
{
    if (cursor == NULL) {
        return 0;
    }
    return cursor->remaining;
}
