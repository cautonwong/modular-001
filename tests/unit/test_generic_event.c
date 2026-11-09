#include "unity.h"

#include <string.h>

#include "domain/events/generic.h"

static nd_generic_event_state_t state;

void setUp(void)
{
    memset(&state, 0, sizeof(state));
}

void tearDown(void) {}

static void test_generic_event_starts_and_ends(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_generic_event_step(&state, 1, 1));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_generic_event_step(&state, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(1, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(1, state.accumulated_seconds);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_generic_event_step(&state, 1, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_ENDED, nd_generic_event_step(&state, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
}

static void test_inactive_condition_never_starts(void)
{
    for (int step = 0; step < 5; step++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_generic_event_step(&state, 2, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(0, state.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(0, state.accumulated_seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_generic_event_starts_and_ends);
    RUN_TEST(test_inactive_condition_never_starts);
    return UNITY_END();
}
