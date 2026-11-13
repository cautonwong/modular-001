#include "unity.h"

#include "application/adapters/dl698/dl698_time.h"

void setUp(void) {}

void tearDown(void) {}

static void date(uint8_t *out, uint16_t year, uint8_t month, uint8_t day,
                 uint8_t hour, uint8_t minute, uint8_t second)
{
    out[0] = (uint8_t)(year >> 8);
    out[1] = (uint8_t)year;
    out[2] = month;
    out[3] = day;
    out[4] = hour;
    out[5] = minute;
    out[6] = second;
}

static void test_valid_date_time_is_accepted(void)
{
    uint8_t when[7];

    date(when, 2024, 6, 6, 12, 34, 56);
    TEST_ASSERT_TRUE(dl_data_time_check(when, 0));
}

static void test_leap_years_follow_the_calendar(void)
{
    uint8_t when[7];

    date(when, 2024, 2, 29, 0, 0, 0);
    TEST_ASSERT_TRUE(dl_data_time_check(when, 0));
    date(when, 2023, 2, 29, 0, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2000, 2, 29, 0, 0, 0);
    TEST_ASSERT_TRUE(dl_data_time_check(when, 0));
}

static void test_out_of_range_fields_are_rejected(void)
{
    uint8_t when[7];

    date(when, 2024, 0, 1, 0, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 13, 1, 0, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 1, 0, 0, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 1, 32, 0, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 1, 1, 24, 0, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 1, 1, 0, 60, 0);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
    date(when, 2024, 1, 1, 0, 0, 60);
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
}

static void test_wildcards_are_only_accepted_when_requested(void)
{
    uint8_t when[7];

    date(when, 0xFFFF, 1, 1, 0, 0, 0);
    TEST_ASSERT_TRUE(dl_data_time_check(when, TIME_WILDCARD));
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));

    date(when, 2024, 0xFF, 31, 0, 0, 0);
    TEST_ASSERT_TRUE(dl_data_time_check(when, TIME_WILDCARD));

    date(when, 2024, 6, 0xFF, 0xFF, 0xFF, 0xFF);
    TEST_ASSERT_TRUE(dl_data_time_check(when, TIME_WILDCARD));
    TEST_ASSERT_FALSE(dl_data_time_check(when, 0));
}

static void test_time_tag_interval_uses_seconds(void)
{
    uint8_t tag[3];

    tag[0] = 0; tag[1] = 0; tag[2] = 5;
    TEST_ASSERT_EQUAL_UINT32(5, dl_timeTag_interval(tag, 0));
    tag[0] = 1; tag[1] = 0; tag[2] = 5;
    TEST_ASSERT_EQUAL_UINT32(300, dl_timeTag_interval(tag, 0));
    tag[0] = 2; tag[1] = 0; tag[2] = 5;
    TEST_ASSERT_EQUAL_UINT32(18000, dl_timeTag_interval(tag, 0));
    tag[0] = 3; tag[1] = 0; tag[2] = 5;
    TEST_ASSERT_EQUAL_UINT32(432000, dl_timeTag_interval(tag, 0));
    tag[0] = 6; tag[1] = 0; tag[2] = 5;
    TEST_ASSERT_EQUAL_UINT32(0, dl_timeTag_interval(tag, 0));
}

static void test_datetime_converts_to_seconds_since_2000(void)
{
    uint8_t when[7];

    date(when, 2000, 1, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(0, dl_datetime_to_second(when, 0));
    date(when, 2000, 1, 1, 0, 0, 1);
    TEST_ASSERT_EQUAL_UINT32(1, dl_datetime_to_second(when, 0));
    date(when, 2000, 1, 2, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(86400, dl_datetime_to_second(when, 0));
    date(when, 2024, 6, 6, 12, 34, 56);
    TEST_ASSERT_EQUAL_UINT32(8923u * 86400u + 12u * 3600u + 34u * 60u + 56u,
                             dl_datetime_to_second(when, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_date_time_is_accepted);
    RUN_TEST(test_leap_years_follow_the_calendar);
    RUN_TEST(test_out_of_range_fields_are_rejected);
    RUN_TEST(test_wildcards_are_only_accepted_when_requested);
    RUN_TEST(test_time_tag_interval_uses_seconds);
    RUN_TEST(test_datetime_converts_to_seconds_since_2000);
    return UNITY_END();
}
