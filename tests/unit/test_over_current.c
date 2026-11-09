#include "unity.h"

#include <string.h>

#include "domain/events/over_current.h"

static nd_over_current_state_t state;
static nd_over_current_rule_t rule;

void setUp(void)
{
    memset(&state, 0, sizeof(state));
    rule.threshold = 100;
    rule.debounce_seconds = 2;
}

void tearDown(void) {}

static void test_event_starts_after_the_debounce(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 150));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 150));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_over_current_step(&state, &rule, 150));
    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_seconds);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 150));
    TEST_ASSERT_EQUAL_UINT32(2, state.accumulated_seconds);
}

static void test_current_at_the_threshold_does_not_start(void)
{
    for (int step = 0; step < 5; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 100));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_seconds);
}

static void test_zero_threshold_is_ignored(void)
{
    rule.threshold = 0;

    for (int step = 0; step < 5; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 1000));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_event_ends_after_the_clear_debounce(void)
{
    for (int step = 0; step < 3; step++) {
        nd_over_current_step(&state, &rule, 150);
    }
    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_over_current_step(&state, &rule, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_ENDED, nd_over_current_step(&state, &rule, 0));
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(3, state.accumulated_seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_starts_after_the_debounce);
    RUN_TEST(test_current_at_the_threshold_does_not_start);
    RUN_TEST(test_zero_threshold_is_ignored);
    RUN_TEST(test_event_ends_after_the_clear_debounce);
    return UNITY_END();
}
