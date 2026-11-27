#include "unity.h"

#include "domain/load_control/load_control.h"
#include "support/fake_allocator.h"

static unsigned char arena[256];
static fake_allocator_t fake;
static load_control_state_t *state;
static nd_load_inputs_t inputs;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
    state = load_control_create(&fake.port);
    inputs.current = 0;
    inputs.voltage = 220;
    inputs.volt_low = 100;
    inputs.volt_high = 300;
    inputs.protection_current = 600;
    inputs.protection_minutes = 5;
    inputs.factory_mode = 1;
}

void tearDown(void)
{
    load_control_destroy(state, &fake.port);
}

static nd_load_actions_t tick(void)
{
    nd_load_actions_t actions;
    load_control_second(state, &inputs, &actions);
    return actions;
}

static nd_load_actions_t tick_command(uint8_t command)
{
    load_control_set_command(state, command);
    return tick();
}

static void test_first_tick_primes_without_actuating(void)
{
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_NULL);

    TEST_ASSERT_FALSE(actions.connect);
    TEST_ASSERT_FALSE(actions.disconnect);
}

static void test_remote_off_disconnects_once_and_reports(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_OFF);
    TEST_ASSERT_TRUE(actions.disconnect);
    TEST_ASSERT_TRUE(actions.transition);
    TEST_ASSERT_TRUE(actions.event_switch_off);

    actions = tick();
    TEST_ASSERT_FALSE(actions.disconnect);
    TEST_ASSERT_FALSE(actions.transition);
}

static void test_remote_on_connects_once_and_reports(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.event_switch_on);

    actions = tick();
    TEST_ASSERT_FALSE(actions.connect);
}

static void test_meter_clear_suppresses_the_switch_on_event(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    load_control_set_meter_clear(state, 1);
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_ON);

    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.transition);
    TEST_ASSERT_FALSE(actions.event_switch_on);
}

static void test_voltage_out_of_range_blocks_the_command(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    inputs.factory_mode = 0;
    inputs.voltage = 50;
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_OFF);
    TEST_ASSERT_FALSE(actions.disconnect);

    inputs.voltage = 220;
    actions = tick();
    TEST_ASSERT_TRUE(actions.disconnect);
}

static void test_factory_mode_ignores_the_voltage_gate(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    inputs.factory_mode = 1;
    inputs.voltage = 50;
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_OFF);

    TEST_ASSERT_TRUE(actions.disconnect);
}

static void test_pretrip_direct_reconnects(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_PRETRIP_DIRECT);

    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.event_switch_on);
    TEST_ASSERT_EQUAL_UINT8(0, load_control_command(state));
}

static void test_pretrip_allow_becomes_on_allow_and_waits(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_PRETRIP_ALLOW);
    TEST_ASSERT_FALSE(actions.disconnect);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON_ALLOW, load_control_command(state));

    actions = tick();
    TEST_ASSERT_FALSE(actions.connect);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON_ALLOW, load_control_command(state));
}

static void test_cleared_key_command_connects(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_NULL);

    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.event_switch_on);
}

static void test_repeat_actuates_without_reporting(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    load_control_request_repeat(state);
    nd_load_actions_t actions = tick();

    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_FALSE(actions.transition);
    TEST_ASSERT_FALSE(actions.event_switch_on);
}

static void test_limit_timer_counts_down_and_is_readable(void)
{
    load_control_set_timers(state, 0, 0, 2);

    tick();
    TEST_ASSERT_EQUAL_UINT32(1, load_control_limit_minutes(state));
    tick();
    TEST_ASSERT_EQUAL_UINT32(0, load_control_limit_minutes(state));
}

static void test_alarm_timer_arms_current_limit_and_blocks_off(void)
{
    load_control_set_timers(state, 2, 0, 0);
    inputs.current = 1000;

    tick_command(ND_LOAD_REMOTE_OFF);
    TEST_ASSERT_EQUAL_UINT32(1, load_control_alarm_minutes(state));

    nd_load_actions_t actions = tick();
    TEST_ASSERT_EQUAL_UINT32(0, load_control_alarm_minutes(state));
    TEST_ASSERT_EQUAL_UINT32(300, load_control_current_limit_minutes(state));
    TEST_ASSERT_FALSE(actions.disconnect);

    inputs.current = 0;
    actions = tick();
    TEST_ASSERT_EQUAL_UINT32(0, load_control_current_limit_minutes(state));
    TEST_ASSERT_TRUE(actions.disconnect);
}

static void test_alarm_flag_blocks_remote_close_until_cleared(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions =
        tick_command(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_ALARM);
    TEST_ASSERT_FALSE(actions.connect);

    actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.event_switch_on);
}

static void test_alarm_flag_blocks_pretrip_direct_close(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions =
        tick_command(ND_LOAD_REMOTE_ALARM | ND_LOAD_REMOTE_PRETRIP_DIRECT);
    TEST_ASSERT_FALSE(actions.connect);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ALARM, load_control_command(state));

    actions = tick();
    TEST_ASSERT_FALSE(actions.connect);

    actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_TRUE(actions.connect);
}

static void test_alarm_flag_blocks_key_cleared_on_allow_close(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions =
        tick_command(ND_LOAD_REMOTE_ON_ALLOW | ND_LOAD_REMOTE_ALARM);
    TEST_ASSERT_FALSE(actions.connect);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON_ALLOW | ND_LOAD_REMOTE_ALARM,
                            load_control_command(state));

    actions = tick_command(ND_LOAD_REMOTE_ALARM); /* the key clears ON_ALLOW */
    TEST_ASSERT_FALSE(actions.connect);

    actions = tick_command(ND_LOAD_REMOTE_NULL); /* alarm cleared */
    TEST_ASSERT_TRUE(actions.connect);
}

static void test_alarm_flag_does_not_block_remote_off(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    nd_load_actions_t actions =
        tick_command(ND_LOAD_REMOTE_OFF | ND_LOAD_REMOTE_ALARM);

    TEST_ASSERT_TRUE(actions.disconnect);
}

static void test_alarm_delay_blocks_remote_close(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    load_control_set_timers(state, 2, 0, 0);
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_FALSE(actions.connect);
    TEST_ASSERT_EQUAL_UINT32(1, load_control_alarm_minutes(state));

    actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_TRUE(actions.connect);
}

static void test_current_limit_alone_does_not_block_remote_close(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    inputs.current = 1000;
    load_control_set_timers(state, 0, 300, 0);
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_ON);

    TEST_ASSERT_TRUE(actions.connect);
}

static void test_voltage_gate_and_alarm_both_block_remote_close(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    inputs.factory_mode = 0;
    inputs.voltage = 50;
    nd_load_actions_t actions =
        tick_command(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_ALARM);
    TEST_ASSERT_FALSE(actions.connect);

    inputs.voltage = 220;
    actions = tick_command(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_ALARM);
    TEST_ASSERT_FALSE(actions.connect);

    actions = tick_command(ND_LOAD_REMOTE_ON);
    TEST_ASSERT_TRUE(actions.connect);
}

static nd_load_feedback_inputs_t base_feedback(void)
{
    nd_load_feedback_inputs_t in;
    in.active_power = 100;
    in.current = 0;
    in.relay_off_elapsed = 0;
    in.relay_on_elapsed = 0;
    in.feedback_threshold = 200;
    in.min_current = 17;
    in.suppress_feedback = 0;
    return in;
}

static void test_feedback_flags_a_closed_relay_while_disconnected(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;
    in.active_power = 100;

    load_control_feedback(state, &in, &feedback);

    TEST_ASSERT_TRUE(feedback.misoperation);
}

static void test_feedback_confirms_disconnect_and_reports_once(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;
    in.active_power = 0;
    in.relay_off_elapsed = 200;

    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_FALSE(feedback.misoperation);
    TEST_ASSERT_TRUE(feedback.event_switch_off);

    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_FALSE(feedback.event_switch_off);
}

static void test_feedback_confirms_connect_and_reports_once(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);
    tick_command(ND_LOAD_REMOTE_ON);

    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;
    in.active_power = 100;

    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_FALSE(feedback.misoperation);
    TEST_ASSERT_TRUE(feedback.event_switch_on);

    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_FALSE(feedback.event_switch_on);
}

static void test_feedback_hides_misoperation_while_suppressed(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;
    in.active_power = 100;
    in.suppress_feedback = 1;

    load_control_feedback(state, &in, &feedback);

    TEST_ASSERT_FALSE(feedback.misoperation);
    TEST_ASSERT_FALSE(feedback.event_switch_off);
}

static void test_feedback_stays_unsettled_while_on_allow(void)
{
    load_control_set_command(state, ND_LOAD_REMOTE_ON_ALLOW);

    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;
    in.active_power = 0;
    in.relay_off_elapsed = 200;

    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_FALSE(feedback.misoperation);
    TEST_ASSERT_FALSE(feedback.event_switch_on);
    TEST_ASSERT_FALSE(feedback.event_switch_off);
}

static void test_feedback_reports_the_detected_relay_state(void)
{
    nd_load_feedback_inputs_t in = base_feedback();
    nd_load_feedback_t feedback;

    in.active_power = 0;
    in.relay_off_elapsed = 200;
    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_EQUAL_UINT8(1, load_control_relay_true(state));

    in.active_power = 100;
    load_control_feedback(state, &in, &feedback);
    TEST_ASSERT_EQUAL_UINT8(0, load_control_relay_true(state));
}


int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_tick_primes_without_actuating);
    RUN_TEST(test_remote_off_disconnects_once_and_reports);
    RUN_TEST(test_remote_on_connects_once_and_reports);
    RUN_TEST(test_meter_clear_suppresses_the_switch_on_event);
    RUN_TEST(test_voltage_out_of_range_blocks_the_command);
    RUN_TEST(test_factory_mode_ignores_the_voltage_gate);
    RUN_TEST(test_pretrip_direct_reconnects);
    RUN_TEST(test_pretrip_allow_becomes_on_allow_and_waits);
    RUN_TEST(test_cleared_key_command_connects);
    RUN_TEST(test_repeat_actuates_without_reporting);
    RUN_TEST(test_alarm_timer_arms_current_limit_and_blocks_off);
    RUN_TEST(test_alarm_flag_blocks_remote_close_until_cleared);
    RUN_TEST(test_alarm_flag_blocks_pretrip_direct_close);
    RUN_TEST(test_alarm_flag_blocks_key_cleared_on_allow_close);
    RUN_TEST(test_alarm_flag_does_not_block_remote_off);
    RUN_TEST(test_alarm_delay_blocks_remote_close);
    RUN_TEST(test_current_limit_alone_does_not_block_remote_close);
    RUN_TEST(test_voltage_gate_and_alarm_both_block_remote_close);
    RUN_TEST(test_limit_timer_counts_down_and_is_readable);
    RUN_TEST(test_feedback_flags_a_closed_relay_while_disconnected);
    RUN_TEST(test_feedback_confirms_disconnect_and_reports_once);
    RUN_TEST(test_feedback_confirms_connect_and_reports_once);
    RUN_TEST(test_feedback_hides_misoperation_while_suppressed);
    RUN_TEST(test_feedback_stays_unsettled_while_on_allow);
    RUN_TEST(test_feedback_reports_the_detected_relay_state);
    return UNITY_END();
}
