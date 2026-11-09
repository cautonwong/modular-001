#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"

static unsigned char data[64];
static nd_byte_store_t store;

void setUp(void)
{
    memset(data, 0, sizeof(data));
    nd_byte_store_init(&store, data, sizeof(data));
}

void tearDown(void) {}

static void test_write_then_read_roundtrip(void)
{
    const uint8_t pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t readback[4] = {0};

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_byte_store_port(&store), 8, pattern, 4));
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(nd_byte_store_port(&store), 8, readback, 4));
    TEST_ASSERT_EQUAL_MEMORY(pattern, readback, 4);
}

static void test_read_past_end_fails(void)
{
    uint8_t buffer[8];

    TEST_ASSERT_EQUAL_INT(-1, nd_store_read(nd_byte_store_port(&store), 60, buffer, 8));
}

static void test_write_past_end_fails(void)
{
    uint8_t buffer[8] = {0};

    TEST_ASSERT_EQUAL_INT(-1, nd_store_write(nd_byte_store_port(&store), 60, buffer, 8));
    TEST_ASSERT_EQUAL_UINT8(0, data[60]);
}

static void test_missing_port_fails(void)
{
    uint8_t buffer[1];

    TEST_ASSERT_EQUAL_INT(-1, nd_store_read(NULL, 0, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_store_write(NULL, 0, buffer, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_write_then_read_roundtrip);
    RUN_TEST(test_read_past_end_fails);
    RUN_TEST(test_write_past_end_fails);
    RUN_TEST(test_missing_port_fails);
    return UNITY_END();
}
