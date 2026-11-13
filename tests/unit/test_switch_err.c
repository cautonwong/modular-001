#include "unity.h"

#include "domain/events/switch_err.h"

void setUp(void) {}
void tearDown(void) {}

static void test_switch_error_starts_after_debounce(void)
{
    nd_switch_err_state_t state = {0};
    uint8_t i;

    for (i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_switch_err_step(&state, 3, 1, 0));
    }
    TEST_ASSERT_EQUAL_INT(ND_EVENT_STARTED,
                          nd_switch_err_step(&state, 3, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(1, state.event.debounce.active);
    TEST_ASSERT_EQUAL_UINT32(1, state.event.accumulated_seconds);
}

static void test_switch_error_hold_accumulates_without_transition(void)
{
    nd_switch_err_state_t state = {0};

    state.event.debounce.active = 1;
    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                          nd_switch_err_step(&state, 3, 1, 1));
    TEST_ASSERT_EQUAL_UINT32(1, state.event.accumulated_seconds);
    TEST_ASSERT_EQUAL_UINT8(1, state.event.debounce.active);
}

static void test_switch_error_hold_ignores_inactive_state(void)
{
    nd_switch_err_state_t state = {0};

    TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                          nd_switch_err_step(&state, 3, 1, 1));
    TEST_ASSERT_EQUAL_UINT32(0, state.event.accumulated_seconds);
    TEST_ASSERT_EQUAL_UINT8(0, state.event.debounce.active);
}

static void test_switch_error_clears_while_held(void)
{
    nd_switch_err_state_t state = {0};
    uint8_t i;

    state.event.debounce.active = 1;
    state.event.debounce.duration = 3;
    for (i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT(ND_EVENT_NONE,
                              nd_switch_err_step(&state, 3, 0, 1));
    }
    TEST_ASSERT_EQUAL_INT(ND_EVENT_ENDED,
                          nd_switch_err_step(&state, 3, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, state.event.debounce.active);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_switch_error_starts_after_debounce);
    RUN_TEST(test_switch_error_hold_accumulates_without_transition);
    RUN_TEST(test_switch_error_hold_ignores_inactive_state);
    RUN_TEST(test_switch_error_clears_while_held);
    return UNITY_END();
}
