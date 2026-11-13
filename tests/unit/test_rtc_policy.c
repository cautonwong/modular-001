#include "unity.h"

#include "domain/events/rtc.h"

void setUp(void) {}
void tearDown(void) {}

static void test_rtc_default_datetime_is_legacy_value(void)
{
    nd_datetime_t value;

    nd_rtc_default_datetime(&value);
    TEST_ASSERT_EQUAL_UINT16(2022, value.year);
    TEST_ASSERT_EQUAL_UINT8(9, value.month);
    TEST_ASSERT_EQUAL_UINT8(8, value.day);
    TEST_ASSERT_EQUAL_UINT8(8, value.hour);
    TEST_ASSERT_EQUAL_UINT8(8, value.minute);
    TEST_ASSERT_EQUAL_UINT8(8, value.second);
}

static void test_rtc_snapshot_due_at_fixed_time(void)
{
    nd_datetime_t now = {
        .second = 35,
        .minute = 1,
        .hour = 12,
        .day = 11,
        .month = 8,
        .year = 2026,
    };

    TEST_ASSERT_EQUAL_UINT8(1, nd_rtc_snapshot_due(&now));
    now.second = 34;
    TEST_ASSERT_EQUAL_UINT8(0, nd_rtc_snapshot_due(&now));
    now.second = 35;
    now.minute = 0;
    TEST_ASSERT_EQUAL_UINT8(0, nd_rtc_snapshot_due(&now));
}

static void test_rtc_poweron_gap_and_rollback(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_rtc_poweron_fault(0, 100, 50));
    TEST_ASSERT_EQUAL_UINT8(1, nd_rtc_poweron_fault(1, 49, 50));
    TEST_ASSERT_EQUAL_UINT8(0, nd_rtc_poweron_fault(
        1, 50 + ND_RTC_POWERON_MAX_GAP_SECONDS, 50));
    TEST_ASSERT_EQUAL_UINT8(1, nd_rtc_poweron_fault(
        1, 51 + ND_RTC_POWERON_MAX_GAP_SECONDS, 50));
    TEST_ASSERT_EQUAL_UINT8(0, nd_rtc_poweron_fault(1, 100, 50));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rtc_default_datetime_is_legacy_value);
    RUN_TEST(test_rtc_snapshot_due_at_fixed_time);
    RUN_TEST(test_rtc_poweron_gap_and_rollback);
    return UNITY_END();
}
