#include "unity.h"

#include "targets/idis_ip_meter/cosem/nd_alarm_state.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"

void setUp(void) {}
void tearDown(void) {}

static const nd_idis_event_rule_t rules[] = {
    {.event_code = 10, .register_index = 0, .bit = 3},
    {.event_code = 11, .register_index = 0, .bit = 4},
    {.event_code = 20, .register_index = 1, .bit = 7},
};

static void test_raise_sets_register_bits(void)
{
    nd_alarm_state_t state;
    nd_alarm_init(&state, rules, 3);

    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 10));
    TEST_ASSERT_EQUAL_UINT32(1u << 3, nd_alarm_bits(&state, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 20));
    TEST_ASSERT_EQUAL_UINT32(1u << 7, nd_alarm_bits(&state, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_alarm_raise(&state, 99));
}

static void test_filter_blocks_and_clear_removes(void)
{
    nd_alarm_state_t state;
    nd_alarm_init(&state, rules, 3);

    TEST_ASSERT_EQUAL_INT(0, nd_alarm_filter_set(&state, 0, 1u << 4));
    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 10));
    TEST_ASSERT_EQUAL_UINT32(0, nd_alarm_bits(&state, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 11));
    TEST_ASSERT_EQUAL_UINT32(1u << 4, nd_alarm_bits(&state, 0));

    TEST_ASSERT_EQUAL_INT(0, nd_alarm_clear(&state, 0, 1u << 4));
    TEST_ASSERT_EQUAL_UINT32(0, nd_alarm_bits(&state, 0));
}

static void test_invalid_register_index_is_rejected(void)
{
    nd_alarm_state_t state;
    nd_alarm_init(&state, rules, 3);

    TEST_ASSERT_EQUAL_INT(-1, nd_alarm_clear(&state, 2, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_alarm_filter_set(&state, 2, 1));
    TEST_ASSERT_EQUAL_UINT32(0, nd_alarm_bits(&state, 2));
}

static void test_generated_event_map_sets_known_bits(void)
{
    nd_alarm_state_t state;
    nd_alarm_init(&state, nd_idis_event_rules, nd_idis_event_rule_count);

    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 7));
    TEST_ASSERT_EQUAL_UINT32(1u << 1, nd_alarm_bits(&state, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 100));
    TEST_ASSERT_EQUAL_UINT32((1u << 1) | (1u << 16), nd_alarm_bits(&state, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_alarm_raise(&state, 68));
    TEST_ASSERT_EQUAL_UINT32(1u << 31, nd_alarm_bits(&state, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_raise_sets_register_bits);
    RUN_TEST(test_filter_blocks_and_clear_removes);
    RUN_TEST(test_invalid_register_index_is_rejected);
    RUN_TEST(test_generated_event_map_sets_known_bits);
    return UNITY_END();
}
