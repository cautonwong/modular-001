#include "unity.h"

#include <string.h>

#include "infra/kernel/fifo.h"

static unsigned char storage[8];
static struct fifo_t fifo;

void setUp(void)
{
    fifoBuf_init(&fifo, storage, sizeof(storage));
}

void tearDown(void) {}

static void test_byte_put_peek_and_get(void)
{
    TEST_ASSERT_EQUAL_UINT16(7, fifoBuf_getSize(&fifo));

    fifoBuf_putByte(&fifo, 'a');
    TEST_ASSERT_EQUAL_UINT16(1, fifoBuf_getUsed(&fifo));
    TEST_ASSERT_EQUAL_UINT16(6, fifoBuf_getFree(&fifo));
    TEST_ASSERT_EQUAL_INT('a', fifoBuf_getBytePeek(&fifo));
    TEST_ASSERT_EQUAL_UINT16(1, fifoBuf_getUsed(&fifo));

    TEST_ASSERT_EQUAL_UINT8('a', fifoBuf_getByte(&fifo));
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&fifo));
    TEST_ASSERT_EQUAL_UINT8(0, fifoBuf_getByte(&fifo));
    TEST_ASSERT_EQUAL_INT(-1, fifoBuf_getBytePeek(&fifo));
}

static void test_block_put_and_get(void)
{
    TEST_ASSERT_EQUAL_UINT16(5, fifoBuf_putData(&fifo, "abcde", 5));

    char first[3] = {0};
    TEST_ASSERT_EQUAL_UINT16(3, fifoBuf_getData(&fifo, first, 3));
    TEST_ASSERT_EQUAL_MEMORY("abc", first, 3);

    char rest[8] = {0};
    TEST_ASSERT_EQUAL_UINT16(2, fifoBuf_getData(&fifo, rest, sizeof(rest)));
    TEST_ASSERT_EQUAL_MEMORY("de", rest, 2);
}

static void test_wraparound_preserves_order(void)
{
    fifoBuf_putData(&fifo, "abcdefg", 7);
    char first[5] = {0};
    fifoBuf_getData(&fifo, first, 5);

    fifoBuf_putData(&fifo, "hij", 3);

    char second[5] = {0};
    TEST_ASSERT_EQUAL_UINT16(5, fifoBuf_getData(&fifo, second, 5));
    TEST_ASSERT_EQUAL_MEMORY("fghij", second, 5);
}

static void test_u16_and_u32_roundtrip(void)
{
    TEST_ASSERT_EQUAL_INT(4, ringbuffer_putU16(&fifo, 0x1234));
    uint16_t word = 0;
    TEST_ASSERT_EQUAL_INT(4, ringbuffer_getU16(&fifo, &word));
    TEST_ASSERT_EQUAL_UINT16(0x1234, word);

    TEST_ASSERT_EQUAL_INT(4, ringbuffer_putU32(&fifo, 0x11223344));
    uint32_t dword = 0;
    TEST_ASSERT_EQUAL_INT(4, ringbuffer_getU32(&fifo, &dword));
    TEST_ASSERT_EQUAL_UINT32(0x11223344, dword);
}

static void test_clear_resets_the_ring(void)
{
    fifoBuf_putData(&fifo, "abcd", 4);

    ringbuffer_clear(&fifo);

    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&fifo));
    TEST_ASSERT_EQUAL_UINT16(7, fifoBuf_getFree(&fifo));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_byte_put_peek_and_get);
    RUN_TEST(test_block_put_and_get);
    RUN_TEST(test_wraparound_preserves_order);
    RUN_TEST(test_u16_and_u32_roundtrip);
    RUN_TEST(test_clear_resets_the_ring);
    return UNITY_END();
}
