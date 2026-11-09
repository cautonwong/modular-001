#include "unity.h"

#include <string.h>

#include "domain/events/cover_open.h"

static nd_cover_open_state_t state;
static nd_cover_open_rule_t rule;

void setUp(void)
{
    memset(&state, 0, sizeof(state));
    rule.debounce_seconds = 2;
}

void tearDown(void) {}

static void test_event_starts_after_the_debounce(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_cover_open_step(&state, &rule, 1, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_cover_open_step(&state, &rule, 1, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_cover_open_step(&state, &rule, 1, 0));

    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_seconds);
    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_low_power_seconds);
}

static void test_low_power_uses_its_own_accumulator(void)
{
    for (int step = 0; step < 3; step++) {
        nd_cover_open_step(&state, &rule, 1, 1);
    }

    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_seconds);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_low_power_seconds);
}

static void test_closed_cover_never_starts(void)
{
    for (int step = 0; step < 5; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_cover_open_step(&state, &rule, 0, 0));
    }

    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_starts_after_the_debounce);
    RUN_TEST(test_low_power_uses_its_own_accumulator);
    RUN_TEST(test_closed_cover_never_starts);
    return UNITY_END();
}
