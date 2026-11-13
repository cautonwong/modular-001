#include "unity.h"

#include "domain/events/duration.h"

static nd_event_duration_t state;

void setUp(void)
{
    state.duration = 0;
    state.active = 0;
}

void tearDown(void) {}

static void test_event_starts_after_the_debounce(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 3, 1));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 3, 1));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 3, 1));
    TEST_ASSERT_EQUAL_UINT8(3, state.duration);
    TEST_ASSERT_EQUAL_UINT8(0, state.active);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_event_duration_step(&state, 3, 1));
    TEST_ASSERT_EQUAL_UINT8(1, state.active);
}

static void test_event_reports_once_while_active(void)
{
    for (int step = 0; step < 4; step++) {
        nd_event_duration_step(&state, 1, 1);
    }
    TEST_ASSERT_EQUAL_UINT8(1, state.active);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 1, 1));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 1, 1));
}

static void test_event_ends_after_the_clear_debounce(void)
{
    for (int step = 0; step < 2; step++) {
        nd_event_duration_step(&state, 1, 1);
    }
    TEST_ASSERT_EQUAL_UINT8(1, state.active);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 1, 0));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_ENDED, nd_event_duration_step(&state, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(0, state.active);
    TEST_ASSERT_EQUAL_UINT8(0, state.duration);
}

static void test_zero_debounce_starts_immediately(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_event_duration_step(&state, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(1, state.active);
}

static void test_inactive_event_stays_at_zero(void)
{
    for (int step = 0; step < 5; step++) {
        nd_event_duration_step(&state, 3, 0);
    }

    TEST_ASSERT_EQUAL_UINT8(0, state.duration);
    TEST_ASSERT_EQUAL_UINT8(0, state.active);
}

static void test_event_can_start_again_after_it_ends(void)
{
    for (int step = 0; step < 2; step++) {
        nd_event_duration_step(&state, 1, 1);
    }
    nd_event_duration_step(&state, 1, 0);
    nd_event_duration_step(&state, 1, 0);

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 1, 1));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_event_duration_step(&state, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(1, state.active);
}

static void test_forced_start_activates_once(void)
{
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED, nd_event_duration_start(&state));
    TEST_ASSERT_EQUAL_UINT8(1, state.active);
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_start(&state));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE, nd_event_duration_step(&state, 3, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_starts_after_the_debounce);
    RUN_TEST(test_event_reports_once_while_active);
    RUN_TEST(test_event_ends_after_the_clear_debounce);
    RUN_TEST(test_zero_debounce_starts_immediately);
    RUN_TEST(test_inactive_event_stays_at_zero);
    RUN_TEST(test_event_can_start_again_after_it_ends);
    RUN_TEST(test_forced_start_activates_once);
    return UNITY_END();
}
