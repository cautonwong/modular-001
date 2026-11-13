#include "unity.h"

#include "domain/report/report_retry.h"

void setUp(void) {}
void tearDown(void) {}

static nd_report_retry_input_t input(uint8_t power_on)
{
    nd_report_retry_input_t value = {0};
    value.power_on = power_on;
    value.power_on_report_delay = 5;
    return value;
}

static void test_power_off_start_requires_the_window_and_limit(void)
{
    nd_report_retry_t state = {0};
    nd_report_retry_input_t in = input(0);

    in.power_on_sec_limit = 100;
    in.power_on_sec = 100;
    in.ac_power_off_sec = 1;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_3011_START,
                          nd_report_retry_next(&state, &in));

    in.ac_power_off_sec = 4;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NONE,
                          nd_report_retry_next(&state, &in));

    in.ac_power_off_sec = 2;
    in.power_on_sec = 99;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NONE,
                          nd_report_retry_next(&state, &in));

    in.power_on_sec = 100;
    state.retry_cnt[ND_REPORT_RETRY_3011_START] = 3;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NONE,
                          nd_report_retry_next(&state, &in));
}

static void test_reset_flag_blocks_power_off_report(void)
{
    nd_report_retry_t state = {0};
    nd_report_retry_input_t in = input(0);

    in.ac_power_off_sec = 1;
    in.power_on_sec = 100;
    in.power_on_sec_limit = 100;
    in.reset_flag = ND_REPORT_RESET_FLAG;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NONE,
                          nd_report_retry_next(&state, &in));
}

static void test_power_on_waits_then_prefers_3011(void)
{
    nd_report_retry_t state = {0};
    nd_report_retry_input_t in = input(1);

    in.power_on_report_delay = 4;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_WAIT,
                          nd_report_retry_next(&state, &in));

    in.power_on_report_delay = 5;
    state.cmdtype = ND_REPORT_CMD_3011_END | ND_REPORT_CMD_3320;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_3011_END,
                          nd_report_retry_next(&state, &in));

    state.cmdtype = ND_REPORT_CMD_3011_START;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_3011_START,
                          nd_report_retry_next(&state, &in));

    state.cmdtype = ND_REPORT_CMD_3320 | ND_REPORT_CMD_2015;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NORMAL,
                          nd_report_retry_next(&state, &in));
}

static void test_end_retry_limit_falls_through_to_normal(void)
{
    nd_report_retry_t state = {0};
    nd_report_retry_input_t in = input(1);

    state.cmdtype = ND_REPORT_CMD_3011_END | ND_REPORT_CMD_2015;
    state.retry_cnt[ND_REPORT_RETRY_3011_END] = 3;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NORMAL,
                          nd_report_retry_next(&state, &in));

    state.retry_cnt[ND_REPORT_RETRY_3011_END] = 0;
    state.retry_dly[ND_REPORT_RETRY_3011_END] = 1;
    TEST_ASSERT_EQUAL_INT(ND_REPORT_ACTION_NONE,
                          nd_report_retry_next(&state, &in));
}

static void test_3011_sent_updates_delay_count_and_piid(void)
{
    nd_report_retry_t state = {0};

    state.cmdtype = ND_REPORT_CMD_3011_END;
    nd_report_retry_3011_sent(&state, 1, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(10, state.retry_dly[ND_REPORT_RETRY_3011_END]);
    TEST_ASSERT_EQUAL_UINT8(1, state.retry_cnt[ND_REPORT_RETRY_3011_END]);
    TEST_ASSERT_EQUAL_UINT8(0, state.piid);

    state.retry_cnt[ND_REPORT_RETRY_3011_END] = 2;
    nd_report_retry_3011_sent(&state, 1, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(3, state.retry_cnt[ND_REPORT_RETRY_3011_END]);
    TEST_ASSERT_EQUAL_UINT8(1, state.piid);

    state.cmdtype = ND_REPORT_CMD_3011_END;
    nd_report_retry_3011_sent(&state, 1, 1, 0);
    TEST_ASSERT_EQUAL_UINT16(0, state.cmdtype);
}

static void test_3011_start_clears_after_three_and_off_start_only_counts(void)
{
    nd_report_retry_t state = {0};

    state.cmdtype = ND_REPORT_CMD_3011_START;
    state.retry_cnt[ND_REPORT_RETRY_3011_START] = 2;
    nd_report_retry_3011_sent(&state, 0, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(3, state.retry_cnt[ND_REPORT_RETRY_3011_START]);
    TEST_ASSERT_EQUAL_UINT8(1, state.piid);
    TEST_ASSERT_EQUAL_UINT16(0, state.cmdtype);

    state.cmdtype = ND_REPORT_CMD_3011_START;
    state.retry_cnt[ND_REPORT_RETRY_3011_START] = 2;
    state.piid = 0;
    nd_report_retry_3011_start_off_sent(&state, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(3, state.retry_cnt[ND_REPORT_RETRY_3011_START]);
    TEST_ASSERT_EQUAL_UINT8(0, state.piid);
    TEST_ASSERT_EQUAL_UINT16(ND_REPORT_CMD_3011_START, state.cmdtype);
}

static void test_normal_prepare_tracks_each_item(void)
{
    nd_report_retry_t state = {0};
    int include_3320;
    int include_2015;

    state.cmdtype = ND_REPORT_CMD_3320 | ND_REPORT_CMD_2015;
    state.retry_cnt[ND_REPORT_RETRY_3320] = 2;
    TEST_ASSERT_TRUE(nd_report_retry_normal_prepare(
        &state, 1, 1, 1, &include_3320, &include_2015));
    TEST_ASSERT_TRUE(include_3320);
    TEST_ASSERT_TRUE(include_2015);
    TEST_ASSERT_EQUAL_UINT8(3, state.retry_cnt[ND_REPORT_RETRY_3320]);
    TEST_ASSERT_EQUAL_UINT8(1, state.retry_cnt[ND_REPORT_RETRY_2015]);
    TEST_ASSERT_EQUAL_UINT8(1, state.piid);

    state.cmdtype = ND_REPORT_CMD_3320 | ND_REPORT_CMD_2015;
    state.retry_cnt[ND_REPORT_RETRY_3320] = 0;
    state.retry_cnt[ND_REPORT_RETRY_2015] = 0;
    TEST_ASSERT_FALSE(nd_report_retry_normal_prepare(
        &state, 1, 0, 0, &include_3320, &include_2015));
    TEST_ASSERT_FALSE(include_3320);
    TEST_ASSERT_FALSE(include_2015);
    TEST_ASSERT_EQUAL_UINT16(0, state.cmdtype);
}

static void test_normal_sent_sets_both_delays(void)
{
    nd_report_retry_t state = {0};

    nd_report_retry_normal_sent(&state, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(10, state.retry_dly[ND_REPORT_RETRY_3320]);
    TEST_ASSERT_EQUAL_UINT8(10, state.retry_dly[ND_REPORT_RETRY_2015]);

    state.retry_dly[ND_REPORT_RETRY_3320] = 0;
    state.retry_dly[ND_REPORT_RETRY_2015] = 0;
    nd_report_retry_normal_sent(&state, 1, 0);
    TEST_ASSERT_EQUAL_UINT8(0, state.retry_dly[ND_REPORT_RETRY_3320]);
    TEST_ASSERT_EQUAL_UINT8(0, state.retry_dly[ND_REPORT_RETRY_2015]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_off_start_requires_the_window_and_limit);
    RUN_TEST(test_reset_flag_blocks_power_off_report);
    RUN_TEST(test_power_on_waits_then_prefers_3011);
    RUN_TEST(test_end_retry_limit_falls_through_to_normal);
    RUN_TEST(test_3011_sent_updates_delay_count_and_piid);
    RUN_TEST(test_3011_start_clears_after_three_and_off_start_only_counts);
    RUN_TEST(test_normal_prepare_tracks_each_item);
    RUN_TEST(test_normal_sent_sets_both_delays);
    return UNITY_END();
}
