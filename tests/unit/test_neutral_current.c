#include "unity.h"

#include <string.h>

#include "domain/events/neutral_current.h"

static nd_neutral_current_state_t state;
static nd_neutral_current_rule_t rule;

void setUp(void)
{
    memset(&state, 0, sizeof(state));
    rule.minimum_current = 100;
    rule.rate_threshold = 1000;
    rule.debounce_seconds = 2;
}

void tearDown(void) {}

static void test_imbalance_starts_after_the_debounce(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                          nd_neutral_current_step(&state, &rule, 1000, 500));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                          nd_neutral_current_step(&state, &rule, 1000, 500));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED,
                          nd_neutral_current_step(&state, &rule, 1000, 500));
    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_seconds);
}

static void test_small_imbalance_stays_quiet(void)
{
    for (int step = 0; step < 6; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_neutral_current_step(&state, &rule, 1000, 990));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_currents_below_the_minimum_are_ignored(void)
{
    for (int step = 0; step < 6; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_neutral_current_step(&state, &rule, 50, 40));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_zero_rate_threshold_disables_the_event(void)
{
    rule.rate_threshold = 0;

    for (int step = 0; step < 6; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_neutral_current_step(&state, &rule, 1000, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_both_currents_zero_do_not_divide_by_zero(void)
{
    for (int step = 0; step < 6; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_neutral_current_step(&state, &rule, 0, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_negative_currents_use_magnitude(void)
{
    for (int step = 0; step < 3; step++) {
        nd_neutral_current_step(&state, &rule, -1000, -500);
    }

    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_imbalance_starts_after_the_debounce);
    RUN_TEST(test_small_imbalance_stays_quiet);
    RUN_TEST(test_currents_below_the_minimum_are_ignored);
    RUN_TEST(test_zero_rate_threshold_disables_the_event);
    RUN_TEST(test_both_currents_zero_do_not_divide_by_zero);
    RUN_TEST(test_negative_currents_use_magnitude);
    return UNITY_END();
}
