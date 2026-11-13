#include "unity.h"

#include "application/adapters/dl698/report_list.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void make_list(uint8_t *buffer, const uint32_t *oad, uint8_t count)
{
    uint8_t i;

    buffer[0] = count;
    for (i = 0; i < count; i++) {
        buffer[1 + i * 4] = (uint8_t)oad[i];
        buffer[2 + i * 4] = (uint8_t)(oad[i] >> 8);
        buffer[3 + i * 4] = (uint8_t)(oad[i] >> 16);
        buffer[4 + i * 4] = (uint8_t)(oad[i] >> 24);
    }
}

static void test_parse_reads_little_endian_oads(void)
{
    const uint32_t oad[] = { 0x20150200, 0x30110200 };
    uint8_t buffer[16] = { 0 };
    nd_report_list_t list;

    make_list(buffer, oad, 2);
    TEST_ASSERT_EQUAL_INT(0, nd_report_list_parse(&list, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(2, list.count);
    TEST_ASSERT_EQUAL_HEX32(0x20150200, list.oad[0]);
    TEST_ASSERT_EQUAL_HEX32(0x30110200, list.oad[1]);
}

static void test_parse_rejects_bad_count_and_short_buffer(void)
{
    uint8_t buffer[4] = { ND_REPORT_LIST_MAX + 1, 0, 0, 0 };
    nd_report_list_t list;

    TEST_ASSERT_EQUAL_INT(-1, nd_report_list_parse(&list, buffer, sizeof(buffer)));

    buffer[0] = 2;
    TEST_ASSERT_EQUAL_INT(-1, nd_report_list_parse(&list, buffer, 5));
}

static void test_encode_round_trips(void)
{
    const uint32_t oad[] = { 0x20150200, 0x50040200, 0x30110200 };
    uint8_t source[16] = { 0 };
    uint8_t target[16] = { 0 };
    nd_report_list_t list;

    make_list(source, oad, 3);
    TEST_ASSERT_EQUAL_INT(0, nd_report_list_parse(&list, source, sizeof(source)));
    TEST_ASSERT_EQUAL_INT(13, nd_report_list_encode(&list, target, sizeof(target)));
    TEST_ASSERT_EQUAL_MEMORY(source, target, 13);
}

static void test_encode_rejects_short_buffer(void)
{
    nd_report_list_t list = { 2, { 0x20150200, 0x30110200 } };
    uint8_t buffer[8] = { 0 };

    TEST_ASSERT_EQUAL_INT(0, nd_report_list_encode(&list, buffer, sizeof(buffer)));
}

static void test_find_and_remove(void)
{
    nd_report_list_t list = {
        3, { 0x20150200, 0x30110200, 0x50040200 }
    };

    TEST_ASSERT_EQUAL_INT(1, nd_report_list_find(&list, 0x30110200));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_list_find(&list, 0x11111111));

    TEST_ASSERT_EQUAL_INT(1, nd_report_list_remove(&list, 0x30110200));
    TEST_ASSERT_EQUAL_INT(2, list.count);
    TEST_ASSERT_EQUAL_HEX32(0x20150200, list.oad[0]);
    TEST_ASSERT_EQUAL_HEX32(0x50040200, list.oad[1]);

    TEST_ASSERT_EQUAL_INT(0, nd_report_list_remove(&list, 0x30110200));
}

static void test_remove_at_handles_bounds(void)
{
    nd_report_list_t list = {
        2, { 0x20150200, 0x30110200 }
    };

    TEST_ASSERT_EQUAL_INT(1, nd_report_list_remove_at(&list, 1));
    TEST_ASSERT_EQUAL_INT(1, list.count);
    TEST_ASSERT_EQUAL_INT(0, nd_report_list_remove_at(&list, 1));
    TEST_ASSERT_EQUAL_INT(1, list.count);
}

static void test_insert_front_keeps_count_and_drops_last(void)
{
    nd_report_list_t list = { 0 };
    uint8_t i;

    nd_report_list_insert_front(&list, 0x20150200);
    TEST_ASSERT_EQUAL_INT(1, list.count);
    TEST_ASSERT_EQUAL_HEX32(0x20150200, list.oad[0]);

    nd_report_list_insert_front(&list, 0x30110200);
    TEST_ASSERT_EQUAL_INT(2, list.count);
    TEST_ASSERT_EQUAL_HEX32(0x30110200, list.oad[0]);
    TEST_ASSERT_EQUAL_HEX32(0x20150200, list.oad[1]);

    list.count = ND_REPORT_LIST_MAX;
    for (i = 0; i < ND_REPORT_LIST_MAX; i++) {
        list.oad[i] = 0x10000000 + i;
    }
    nd_report_list_insert_front(&list, 0x50040200);
    TEST_ASSERT_EQUAL_INT(ND_REPORT_LIST_MAX, list.count);
    TEST_ASSERT_EQUAL_HEX32(0x50040200, list.oad[0]);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, list.oad[1]);
    TEST_ASSERT_EQUAL_HEX32(0x10000000 + ND_REPORT_LIST_MAX - 2,
                            list.oad[ND_REPORT_LIST_MAX - 1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_reads_little_endian_oads);
    RUN_TEST(test_parse_rejects_bad_count_and_short_buffer);
    RUN_TEST(test_encode_round_trips);
    RUN_TEST(test_encode_rejects_short_buffer);
    RUN_TEST(test_find_and_remove);
    RUN_TEST(test_remove_at_handles_bounds);
    RUN_TEST(test_insert_front_keeps_count_and_drops_last);
    return UNITY_END();
}
