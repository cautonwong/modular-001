#include "unity.h"

#include "domain/metering/demand_history.h"

static nd_demand_history_t history;

void setUp(void)
{
    nd_demand_history_init(&history);
}

void tearDown(void) {}

static void test_capture_copies_last_period(void)
{
    nd_demand_snapshot_t snapshot = {
        .current_value = 100,
        .current_fee_value = 200,
        .period_seconds = 900,
        .periods = 15,
    };
    nd_datetime_t capture = {0, 15, 10, 11, 11, 2026};
    nd_datetime_t start = {0, 0, 10, 11, 11, 2026};
    TEST_ASSERT_EQUAL_INT(0, nd_demand_history_capture(
                                 &history, &snapshot, &capture, &start, 0x04));
    TEST_ASSERT_EQUAL_UINT32(100, history.last_value);
    TEST_ASSERT_EQUAL_UINT32(200, history.last_fee_value);
    TEST_ASSERT_EQUAL_UINT8(11, history.last_capture_time.day);
    TEST_ASSERT_EQUAL_UINT8(15, history.last_capture_time.minute);
    TEST_ASSERT_EQUAL_UINT8(0, history.period_start.minute);
    TEST_ASSERT_EQUAL_UINT8(0x04, history.status);
}

static void test_reset_clears_history(void)
{
    nd_demand_snapshot_t snapshot = {.current_value = 1};
    nd_datetime_t time = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_demand_history_capture(
                                 &history, &snapshot, &time, &time, 0xFF));
    TEST_ASSERT_EQUAL_INT(0, nd_demand_history_reset(&history));
    TEST_ASSERT_EQUAL_UINT32(0, history.last_value);
    TEST_ASSERT_EQUAL_UINT8(0, history.status);
}

static void test_rejects_null_inputs(void)
{
    nd_demand_snapshot_t snapshot = {.period_seconds = 900};
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(-1, nd_demand_history_capture(
                                  NULL, &snapshot, &time, &time, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_demand_history_capture(
                                  &history, NULL, &time, &time, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_demand_history_capture(
                                  &history, &snapshot, NULL, &time, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_demand_history_capture(
                                  &history, &snapshot, &time, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_demand_history_reset(NULL));
}


static void test_capture_computes_period_start(void)
{
    nd_demand_snapshot_t snapshot = {
        .current_value = 1,
        .period_seconds = 900,
    };
    nd_datetime_t capture = {0, 15, 10, 11, 11, 2026};
    TEST_ASSERT_EQUAL_INT(0, nd_demand_history_capture(
                                 &history, &snapshot, &capture, NULL, 0));
    TEST_ASSERT_EQUAL_UINT8(10, history.period_start.hour);
    TEST_ASSERT_EQUAL_UINT8(0, history.period_start.minute);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_capture_copies_last_period);
    RUN_TEST(test_reset_clears_history);
    RUN_TEST(test_rejects_null_inputs);
    RUN_TEST(test_capture_computes_period_start);
    return UNITY_END();
}
