#include "unity.h"

#include "components/services/channel/nd_channel.h"

#include <string.h>

typedef struct {
    uint8_t data[16];
    size_t read_pos;
    size_t write_pos;
    size_t write_limit;
    int flush_calls;
} fake_channel_t;

static int fake_read(void *ctx, uint8_t *buffer, size_t length)
{
    fake_channel_t *fake = ctx;
    size_t available = fake->write_pos - fake->read_pos;
    size_t count = length < available ? length : available;

    memcpy(buffer, fake->data + fake->read_pos, count);
    fake->read_pos += count;
    return (int)count;
}

static int fake_write(void *ctx, const uint8_t *buffer, size_t length)
{
    fake_channel_t *fake = ctx;
    size_t room = sizeof(fake->data) - fake->write_pos;
    size_t count = length < room ? length : room;

    if ((fake->write_limit != 0) && (count > fake->write_limit)) {
        count = fake->write_limit;
    }
    memcpy(fake->data + fake->write_pos, buffer, count);
    fake->write_pos += count;
    return (int)count;
}

static int fake_flush(void *ctx)
{
    fake_channel_t *fake = ctx;

    fake->flush_calls++;
    return 0;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_read_write_flush_dispatch(void)
{
    fake_channel_t fake = { 0 };
    nd_channel_t channel = { &fake, fake_read, fake_write, fake_flush };
    const uint8_t source[] = { 1, 2, 3 };
    uint8_t target[3] = { 0 };

    TEST_ASSERT_EQUAL_INT(3, nd_channel_write(&channel, source,
                                              sizeof(source)));
    TEST_ASSERT_EQUAL_INT(0, nd_channel_flush(&channel));
    TEST_ASSERT_EQUAL_INT(3, nd_channel_read(&channel, target, sizeof(target)));
    TEST_ASSERT_EQUAL_MEMORY(source, target, sizeof(source));
    TEST_ASSERT_EQUAL_INT(1, fake.flush_calls);
}

static void test_partial_write_is_reported(void)
{
    fake_channel_t fake = { 0 };
    nd_channel_t channel = { &fake, fake_read, fake_write, fake_flush };
    const uint8_t source[] = { 1, 2, 3, 4, 5 };

    fake.write_limit = 2;
    TEST_ASSERT_EQUAL_INT(2, nd_channel_write(&channel, source,
                                              sizeof(source)));
    TEST_ASSERT_EQUAL_INT(2, fake.write_pos);
}

static void test_null_channel_and_callbacks_are_rejected(void)
{
    fake_channel_t fake = { 0 };
    nd_channel_t empty = { &fake, NULL, NULL, NULL };
    uint8_t buffer[1] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, nd_channel_read(NULL, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_channel_write(NULL, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_channel_flush(NULL));

    TEST_ASSERT_EQUAL_INT(-1, nd_channel_read(&empty, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_channel_write(&empty, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_channel_flush(&empty));
}

static void test_zero_length_operations_do_not_touch_null_buffers(void)
{
    fake_channel_t fake = { 0 };
    nd_channel_t channel = { &fake, fake_read, fake_write, fake_flush };

    TEST_ASSERT_EQUAL_INT(0, nd_channel_read(&channel, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_channel_write(&channel, NULL, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_read_write_flush_dispatch);
    RUN_TEST(test_partial_write_is_reported);
    RUN_TEST(test_null_channel_and_callbacks_are_rejected);
    RUN_TEST(test_zero_length_operations_do_not_touch_null_buffers);
    return UNITY_END();
}
