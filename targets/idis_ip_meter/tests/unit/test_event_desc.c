#include "unity.h"

#include "targets/idis_ip_meter/cosem/nd_idis_event_desc.h"

void setUp(void) {}
void tearDown(void) {}

static void test_describes_known_events(void)
{
    const nd_idis_event_descriptor_t *power =
        nd_idis_event_describe(1);
    TEST_ASSERT_NOT_NULL(power);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_POWER_OFF, power->kind);
    TEST_ASSERT_EQUAL_UINT8(1, power->source);
    TEST_ASSERT_EQUAL_UINT8(3, power->severity);

    const nd_idis_event_descriptor_t *cover =
        nd_idis_event_describe(40);
    TEST_ASSERT_NOT_NULL(cover);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_COVER_OPEN, cover->kind);

    const nd_idis_event_descriptor_t *voltage =
        nd_idis_event_describe(76);
    TEST_ASSERT_NOT_NULL(voltage);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_VOLTAGE, voltage->kind);

    const nd_idis_event_descriptor_t *neutral =
        nd_idis_event_describe(89);
    TEST_ASSERT_NOT_NULL(neutral);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_NEUTRAL_CURRENT, neutral->kind);
    const nd_idis_event_descriptor_t *over =
        nd_idis_event_describe(91);
    TEST_ASSERT_NOT_NULL(over);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_OVER_CURRENT, over->kind);

    TEST_ASSERT_NULL(nd_idis_event_describe(999));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_describes_known_events);
    return UNITY_END();
}
