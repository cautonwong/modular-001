#include "unity.h"

#include "infra/channel/fifo_channel.h"
#include "infra/kernel/fifo.h"

#include <string.h>

static uint8_t buffer[8];
static struct fifo_t fifo;
static nd_fifo_channel_t storage;
static nd_channel_t channel;

void setUp(void)
{
    memset(buffer, 0, sizeof(buffer));
    fifoBuf_init(&fifo, buffer, sizeof(buffer));
    nd_fifo_channel_init(&channel, &storage, &fifo);
}

void tearDown(void)
{
}

static void test_write_then_read_round_trip(void)
{
    const uint8_t source[] = { 1, 2, 3, 4 };
    uint8_t target[4] = { 0 };

    TEST_ASSERT_EQUAL_INT(4, nd_channel_write(&channel, source,
                                              sizeof(source)));
    TEST_ASSERT_EQUAL_INT(4, nd_channel_read(&channel, target,
                                             sizeof(target)));
    TEST_ASSERT_EQUAL_MEMORY(source, target, sizeof(source));
}

static void test_read_empty_returns_zero(void)
{
    uint8_t target[2] = { 0 };

    TEST_ASSERT_EQUAL_INT(0, nd_channel_read(&channel, target,
                                             sizeof(target)));
}

static void test_write_is_bounded_by_fifo_capacity(void)
{
    const uint8_t source[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    TEST_ASSERT_EQUAL_INT(sizeof(buffer) - 1, nd_channel_write(&channel, source,
                                                               sizeof(source)));
    TEST_ASSERT_EQUAL_INT(sizeof(buffer) - 1, fifoBuf_getUsed(&fifo));
}

static void test_flush_is_a_no_op(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_channel_flush(&channel));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_write_then_read_round_trip);
    RUN_TEST(test_read_empty_returns_zero);
    RUN_TEST(test_write_is_bounded_by_fifo_capacity);
    RUN_TEST(test_flush_is_a_no_op);
    return UNITY_END();
}
