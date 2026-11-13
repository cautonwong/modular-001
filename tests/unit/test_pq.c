#include "unity.h"

#include "domain/events/pq.h"

static nd_pq_state_t state;
static nd_event_record_t events[4];

void setUp(void)
{
    nd_pq_config_t config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 2,
        .phase_count = 3,
    };
    nd_pq_init(&state, &config);
}

void tearDown(void) {}

static nd_measurement_value_t value(int64_t v)
{
    return (nd_measurement_value_t){.value = v, .scaler = -1,
                                    .unit = ND_MEASUREMENT_UNIT_VOLT};
}

static void test_sag_needs_debounce(void)
{
    state.config.unbalance_permille = 0;
    nd_measurement_value_t voltage[3] = {value(1900), value(2300), value(2300)};
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_UINT8(0, nd_pq_on_second(&state, voltage, NULL, &now,
                                                events, 4));
    TEST_ASSERT_EQUAL_UINT8(1, nd_pq_on_second(&state, voltage, NULL, &now,
                                                events, 4));
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_VOLTAGE_SAG, events[0].kind);
    TEST_ASSERT_EQUAL_UINT8(1, events[0].source);
    TEST_ASSERT_EQUAL_INT32(1900, events[0].value);
}

static void test_frequency_and_unbalance(void)
{
    nd_measurement_value_t voltage[3] = {value(2300), value(2000), value(2100)};
    nd_measurement_value_t frequency = value(4800);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_UINT8(0, nd_pq_on_second(&state, voltage, &frequency,
                                                &now, events, 4));
    TEST_ASSERT_EQUAL_UINT8(2, nd_pq_on_second(&state, voltage, &frequency,
                                                &now, events, 4));
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_FREQUENCY_LOW, events[0].kind);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_VOLTAGE_UNBALANCE, events[1].kind);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sag_needs_debounce);
    RUN_TEST(test_frequency_and_unbalance);
    return UNITY_END();
}
