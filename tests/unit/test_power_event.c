#include "unity.h"

#include "domain/events/power_event.h"

void setUp(void) {}
void tearDown(void) {}

static void test_power_event_close_time_selector(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_event_close_uses_poweron(0x302C02AAu));
    TEST_ASSERT_EQUAL_UINT8(0, nd_event_close_uses_poweron(0x30050700u));
}

static void test_power_resume_folds_and_clears_low_power_counters(void)
{
    nd_event_power_resume_state_t state = {
        .power_off_acc = 10,
        .power_off_low = 1,
        .cover_open_acc = 20,
        .cover_open_low = 2,
        .power_abnormal_acc = 30,
        .power_abnormal_low = 3,
    };

    nd_event_power_resume(&state);
    TEST_ASSERT_EQUAL_UINT32(11, state.power_off_acc);
    TEST_ASSERT_EQUAL_UINT32(22, state.cover_open_acc);
    TEST_ASSERT_EQUAL_UINT32(33, state.power_abnormal_acc);
    TEST_ASSERT_EQUAL_UINT32(0, state.power_off_low);
    TEST_ASSERT_EQUAL_UINT32(0, state.cover_open_low);
    TEST_ASSERT_EQUAL_UINT32(0, state.power_abnormal_low);

    nd_event_power_resume(&state);
    TEST_ASSERT_EQUAL_UINT32(11, state.power_off_acc);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_event_close_time_selector);
    RUN_TEST(test_power_resume_folds_and_clears_low_power_counters);
    return UNITY_END();
}
