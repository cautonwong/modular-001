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
    inputs.key_pressed = 0;
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
    TEST_ASSERT_TRUE(actions.event_switch_off);

    actions = tick();
    TEST_ASSERT_FALSE(actions.disconnect);
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

    load_control_set_meter_clear(state, 0x5A);
    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_ON);

    TEST_ASSERT_TRUE(actions.connect);
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

static void test_pretrip_allow_becomes_on_allow_and_needs_the_key(void)
{
    tick_command(ND_LOAD_REMOTE_ON);
    tick_command(ND_LOAD_REMOTE_OFF);

    nd_load_actions_t actions = tick_command(ND_LOAD_REMOTE_PRETRIP_ALLOW);
    TEST_ASSERT_FALSE(actions.disconnect);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON_ALLOW, load_control_command(state));

    inputs.key_pressed = 1;
    tick();
    tick();
    actions = tick();
    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_TRUE(actions.event_switch_on);
    TEST_ASSERT_EQUAL_UINT8(0, load_control_command(state));
}

static void test_repeat_actuates_without_reporting(void)
{
    tick_command(ND_LOAD_REMOTE_ON);

    load_control_request_repeat(state);
    nd_load_actions_t actions = tick();

    TEST_ASSERT_TRUE(actions.connect);
    TEST_ASSERT_FALSE(actions.event_switch_on);
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
    RUN_TEST(test_pretrip_allow_becomes_on_allow_and_needs_the_key);
    RUN_TEST(test_repeat_actuates_without_reporting);
    RUN_TEST(test_alarm_timer_arms_current_limit_and_blocks_off);
    return UNITY_END();
}
