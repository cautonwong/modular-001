#include "unity.h"

#include "domain/load_control/load_control.h"
#include "targets/stategrid_meter/app/relay_control_ui.h"

static relay_control_ui_state_t ui;

void setUp(void)
{
    ui.key_ticks = 0;
    ui.led_flash = 0;
}

void tearDown(void) {}

static uint8_t tick_key(uint8_t command, uint8_t key_pressed, uint8_t voltage_ok)
{
    relay_control_ui_key_tick(&ui, &command, key_pressed, voltage_ok);
    return command;
}

static void test_three_ticks_clear_off_and_on_allow(void)
{
    uint8_t command = ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF
                      | ND_LOAD_REMOTE_ON_ALLOW;

    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF
                                | ND_LOAD_REMOTE_ON_ALLOW,
                            tick_key(command, 1, 1));
    tick_key(command, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON, tick_key(command, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(2, ui.led_flash);
    TEST_ASSERT_EQUAL_UINT8(0, ui.key_ticks);
}

static void test_three_ticks_on_allow_clear_the_allow_bit(void)
{
    uint8_t command = ND_LOAD_REMOTE_ON_ALLOW;

    tick_key(command, 1, 1);
    tick_key(command, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(0, tick_key(command, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(2, ui.led_flash);
}

static void test_release_resets_the_hold_count(void)
{
    uint8_t command = ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF;

    tick_key(command, 1, 1);
    tick_key(command, 1, 1);
    tick_key(command, 0, 1);
    tick_key(command, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF,
                            tick_key(command, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(2, ui.key_ticks);
}

static void test_voltage_gate_ignores_the_key(void)
{
    uint8_t command = ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF;

    TEST_ASSERT_EQUAL_UINT8(ND_LOAD_REMOTE_ON | ND_LOAD_REMOTE_OFF,
                            tick_key(command, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(0, ui.key_ticks);
}

static void test_led_blinks_while_on_allow_and_uses_the_second_parity(void)
{
    relay_control_led_t led;

    led = relay_control_ui_led(&ui, ND_LOAD_REMOTE_ON_ALLOW, 1, 1);
    TEST_ASSERT_TRUE(led.led_blink);
    TEST_ASSERT_TRUE(led.led_on);

    led = relay_control_ui_led(&ui, ND_LOAD_REMOTE_ON_ALLOW, 1, 0);
    TEST_ASSERT_TRUE(led.led_blink);
    TEST_ASSERT_FALSE(led.led_on);
}

static void test_led_follows_the_detected_relay_state(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, relay_control_ui_led(&ui, 0, 1, 0).led_on);
    TEST_ASSERT_EQUAL_UINT8(0, relay_control_ui_led(&ui, 0, 0, 1).led_on);
}

static void test_led_blinks_twice_after_a_key_release(void)
{
    relay_control_led_t led;
    uint8_t command = ND_LOAD_REMOTE_ON_ALLOW;

    tick_key(command, 1, 1);
    tick_key(command, 1, 1);
    tick_key(command, 1, 1);
    TEST_ASSERT_EQUAL_UINT8(2, ui.led_flash);

    led = relay_control_ui_led(&ui, 0, 0, 0);
    TEST_ASSERT_TRUE(led.led_blink);
    TEST_ASSERT_EQUAL_UINT8(1, ui.led_flash);
    led = relay_control_ui_led(&ui, 0, 0, 0);
    TEST_ASSERT_TRUE(led.led_blink);
    TEST_ASSERT_EQUAL_UINT8(0, ui.led_flash);
    led = relay_control_ui_led(&ui, 0, 0, 0);
    TEST_ASSERT_FALSE(led.led_blink);
    TEST_ASSERT_FALSE(led.led_on);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_three_ticks_clear_off_and_on_allow);
    RUN_TEST(test_three_ticks_on_allow_clear_the_allow_bit);
    RUN_TEST(test_release_resets_the_hold_count);
    RUN_TEST(test_voltage_gate_ignores_the_key);
    RUN_TEST(test_led_blinks_while_on_allow_and_uses_the_second_parity);
    RUN_TEST(test_led_follows_the_detected_relay_state);
    RUN_TEST(test_led_blinks_twice_after_a_key_release);
    return UNITY_END();
}
