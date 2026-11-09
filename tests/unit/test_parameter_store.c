#include "unity.h"

#include <string.h>

#include "infra/memory/parameter_store.h"

static unsigned char data_a[8];
static unsigned char data_b[2];
static nd_param_record_t records[2];
static nd_param_table_t table;

void setUp(void)
{
    memset(data_a, 0, sizeof(data_a));
    memset(data_b, 0, sizeof(data_b));
    records[0] = (nd_param_record_t){100, data_a, sizeof(data_a)};
    records[1] = (nd_param_record_t){101, data_b, sizeof(data_b)};
    nd_param_table_init(&table, records, 2);
}

void tearDown(void) {}

static void test_write_then_read_roundtrip(void)
{
    const uint8_t pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t readback[4] = {0};

    TEST_ASSERT_EQUAL_INT(0, nd_param_write(nd_param_table_port(&table), 100, pattern, 4));
    TEST_ASSERT_EQUAL_INT(0, nd_param_read(nd_param_table_port(&table), 100, readback, 4));
    TEST_ASSERT_EQUAL_MEMORY(pattern, readback, 4);
}

static void test_unknown_id_fails(void)
{
    uint8_t buffer[1];

    TEST_ASSERT_EQUAL_INT(-1, nd_param_read(nd_param_table_port(&table), 999, buffer, 1));
}

static void test_overlong_access_fails(void)
{
    uint8_t buffer[3] = {0};

    TEST_ASSERT_EQUAL_INT(-1, nd_param_read(nd_param_table_port(&table), 101, buffer, 3));
    TEST_ASSERT_EQUAL_INT(-1, nd_param_write(nd_param_table_port(&table), 101, buffer, 3));
}

static void test_missing_port_fails(void)
{
    uint8_t buffer[1];

    TEST_ASSERT_EQUAL_INT(-1, nd_param_read(NULL, 100, buffer, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_param_write(NULL, 100, buffer, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_write_then_read_roundtrip);
    RUN_TEST(test_unknown_id_fails);
    RUN_TEST(test_overlong_access_fails);
    RUN_TEST(test_missing_port_fails);
    return UNITY_END();
}
