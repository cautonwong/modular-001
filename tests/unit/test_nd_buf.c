#include "unity.h"

#include "components/core/buf/nd_buf.h"

#include <string.h>

static int released;
static uint8_t data_a[] = { 1, 2, 3 };
static uint8_t data_b[] = { 4, 5 };
static uint8_t data_c[] = { 6, 7, 8, 9 };

static void release_frame(nd_frame_t *frame)
{
    (void)frame;
    released++;
}

void setUp(void)
{
    released = 0;
}

void tearDown(void)
{
}

static void make_chain(nd_frame_t *frames, nd_block_t *block)
{
    frames[0].data = data_a;
    frames[0].len = sizeof(data_a);
    frames[0].next = NULL;
    frames[0].owner = NULL;
    frames[0].release = NULL;
    frames[1].data = data_b;
    frames[1].len = sizeof(data_b);
    frames[1].next = NULL;
    frames[1].owner = NULL;
    frames[1].release = NULL;
    frames[2].data = data_c;
    frames[2].len = sizeof(data_c);
    frames[2].next = NULL;
    frames[2].owner = NULL;
    frames[2].release = NULL;

    nd_block_init(block);
    nd_block_append(block, &frames[0]);
    nd_block_append(block, &frames[1]);
    nd_block_append(block, &frames[2]);
}

static void test_block_tracks_total_length(void)
{
    nd_frame_t frames[3];
    nd_block_t block;

    make_chain(frames, &block);
    TEST_ASSERT_EQUAL_INT(9, block.total_len);
    TEST_ASSERT_EQUAL_PTR(&frames[0], block.first);
    TEST_ASSERT_EQUAL_PTR(&frames[2], block.last);
}

static void test_cursor_reads_across_frames(void)
{
    nd_frame_t frames[3];
    nd_block_t block;
    nd_cursor_t cursor;
    uint8_t out[9] = { 0 };

    make_chain(frames, &block);
    nd_block_cursor(&block, &cursor);
    TEST_ASSERT_EQUAL_INT(9, nd_cursor_remaining(&cursor));
    TEST_ASSERT_EQUAL_INT(9, nd_cursor_read(&cursor, out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY(data_a, out, 3);
    TEST_ASSERT_EQUAL_MEMORY(data_b, out + 3, 2);
    TEST_ASSERT_EQUAL_MEMORY(data_c, out + 5, 4);
    TEST_ASSERT_EQUAL_INT(0, nd_cursor_remaining(&cursor));
}

static void test_cursor_read_is_bounded_by_remaining(void)
{
    nd_frame_t frames[3];
    nd_block_t block;
    nd_cursor_t cursor;
    uint8_t out[4] = { 0 };

    make_chain(frames, &block);
    nd_block_cursor(&block, &cursor);
    TEST_ASSERT_EQUAL_INT(4, nd_cursor_read(&cursor, out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(5, nd_cursor_remaining(&cursor));
    TEST_ASSERT_EQUAL_INT(0, nd_cursor_read(&cursor, out, 0));
}

static void test_cursor_skip_moves_across_frames(void)
{
    nd_frame_t frames[3];
    nd_block_t block;
    nd_cursor_t cursor;
    uint8_t out[4] = { 0 };

    make_chain(frames, &block);
    nd_block_cursor(&block, &cursor);
    TEST_ASSERT_EQUAL_INT(4, nd_cursor_skip(&cursor, 4));
    TEST_ASSERT_EQUAL_INT(5, nd_cursor_remaining(&cursor));
    TEST_ASSERT_EQUAL_INT(4, nd_cursor_read(&cursor, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8(5, out[0]);
    TEST_ASSERT_EQUAL_UINT8(6, out[1]);
    TEST_ASSERT_EQUAL_UINT8(8, out[3]);
}

static void test_block_truncate_keeps_prefix(void)
{
    nd_frame_t frames[3];
    nd_block_t block;
    nd_cursor_t cursor;
    uint8_t out[5] = { 0 };

    make_chain(frames, &block);
    TEST_ASSERT_EQUAL_INT(0, nd_block_truncate(&block, 4));
    TEST_ASSERT_EQUAL_INT(4, block.total_len);
    TEST_ASSERT_EQUAL_INT(4, nd_block_copy(&block, out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY(data_a, out, 3);
    TEST_ASSERT_EQUAL_UINT8(4, out[3]);

    nd_block_cursor(&block, &cursor);
    TEST_ASSERT_EQUAL_INT(4, nd_cursor_remaining(&cursor));

    TEST_ASSERT_EQUAL_INT(0, nd_block_truncate(&block, 0));
    TEST_ASSERT_EQUAL_INT(0, block.total_len);
    TEST_ASSERT_NULL(block.first);
}

static void test_block_copy_honors_capacity(void)
{
    nd_frame_t frames[3];
    nd_block_t block;
    uint8_t out[9] = { 0 };

    make_chain(frames, &block);
    TEST_ASSERT_EQUAL_INT(9, nd_block_copy(&block, out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY(data_a, out, 3);
    TEST_ASSERT_EQUAL_INT(5, nd_block_copy(&block, out, 5));
}

static void test_release_walks_the_chain(void)
{
    nd_frame_t frames[3];
    nd_block_t block;

    make_chain(frames, &block);
    frames[0].release = release_frame;
    frames[1].release = release_frame;
    frames[2].release = release_frame;
    nd_block_release(&block);
    TEST_ASSERT_EQUAL_INT(3, released);
    TEST_ASSERT_NULL(block.first);
    TEST_ASSERT_EQUAL_INT(0, block.total_len);
}

static void test_append_rejects_linked_frame(void)
{
    nd_frame_t frames[3];
    nd_block_t block;

    make_chain(frames, &block);
    memset(&frames[0], 0, sizeof(frames[0]));
    frames[0].next = &frames[1];
    TEST_ASSERT_EQUAL_INT(-1, nd_block_append(&block, &frames[0]));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_block_tracks_total_length);
    RUN_TEST(test_cursor_reads_across_frames);
    RUN_TEST(test_cursor_read_is_bounded_by_remaining);
    RUN_TEST(test_cursor_skip_moves_across_frames);
    RUN_TEST(test_block_truncate_keeps_prefix);
    RUN_TEST(test_block_copy_honors_capacity);
    RUN_TEST(test_release_walks_the_chain);
    RUN_TEST(test_append_rejects_linked_frame);
    return UNITY_END();
}
