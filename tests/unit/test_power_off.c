#include "unity.h"

#include <string.h>

#include "domain/events/power_off.h"

static nd_power_off_state_t state;
static nd_power_off_rule_t rule;

void setUp(void)
{
    memset(&state, 0, sizeof(state));
    rule.debounce_seconds = 4;
}

void tearDown(void) {}

static void test_single_phase_below_threshold_starts(void)
{
    uint32_t voltages[1] = {50};

    for (int step = 0; step < 4; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_power_off_step(&state, &rule, voltages, 1, 60, 0));
    }
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED,
                          nd_power_off_step(&state, &rule, voltages, 1, 60, 0));
    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
}

static void test_one_phase_above_threshold_blocks_the_event(void)
{
    uint32_t voltages[2] = {50, 70};

    for (int step = 0; step < 8; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_power_off_step(&state, &rule, voltages, 2, 60, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_all_phases_below_threshold_start(void)
{
    uint32_t voltages[2] = {50, 59};

    for (int step = 0; step < 4; step++) {
        nd_power_off_step(&state, &rule, voltages, 2, 60, 0);
    }

    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED,
                          nd_power_off_step(&state, &rule, voltages, 2, 60, 0));
}

static void test_empty_phase_list_never_starts(void)
{
    for (int step = 0; step < 8; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_power_off_step(&state, &rule, NULL, 0, 60, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_low_power_uses_its_own_accumulator(void)
{
    uint32_t voltages[1] = {50};

    for (int step = 0; step < 5; step++) {
        nd_power_off_step(&state, &rule, voltages, 1, 60, 1);
    }

    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_seconds);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_low_power_seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_phase_below_threshold_starts);
    RUN_TEST(test_one_phase_above_threshold_blocks_the_event);
    RUN_TEST(test_all_phases_below_threshold_start);
    RUN_TEST(test_empty_phase_list_never_starts);
    RUN_TEST(test_low_power_uses_its_own_accumulator);
    return UNITY_END();
}
