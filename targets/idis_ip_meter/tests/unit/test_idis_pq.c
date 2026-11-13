#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"
#include "targets/idis_ip_meter/cosem/nd_idis_pq.h"

void setUp(void) {}
void tearDown(void) {}

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    int64_t value = 5001;
    if (quantity == ND_MEASURE_VOLTAGE) {
        value = phase == 1 ? 1900 : 2300;
    }
    *out = (nd_measurement_value_t){
        .value = value,
        .scaler = -1,
        .unit = quantity == ND_MEASURE_FREQUENCY
                    ? ND_MEASUREMENT_UNIT_HERTZ
                    : ND_MEASUREMENT_UNIT_VOLT,
        .phase = phase,
    };
    return 0;
}

static void test_sag_raises_domain_event(void)
{
    nd_measurement_port_t measurement = {.read = fake_measurement};
    nd_pq_state_t pq;
    nd_pq_config_t config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 0,
        .debounce_seconds = 1,
        .phase_count = 3,
    };
    nd_idis_pq_init(&pq, &config);
    nd_idis_event_state_t events = {0};
    nd_idis_event_init(&events, nd_idis_event_rules, nd_idis_event_rule_count);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_pq_on_second(&pq, &measurement, &events,
                                                   &now));
    const nd_event_record_t *record = nd_event_log_latest(&events.log);
    TEST_ASSERT_NOT_NULL(record);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_VOLTAGE_SAG, record->kind);
    TEST_ASSERT_EQUAL_UINT8(76, events.latest_event_code);
}


static void test_missing_voltage_raises_cut_event(void)
{
    nd_measurement_port_t measurement = {.read = fake_measurement};
    nd_pq_state_t pq;
    nd_pq_config_t config = {
        .voltage_cut = 2000,
        .debounce_seconds = 1,
        .phase_count = 3,
    };
    nd_idis_pq_init(&pq, &config);
    nd_idis_event_state_t events = {0};
    nd_idis_event_init(&events, nd_idis_event_rules, nd_idis_event_rule_count);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_pq_on_second(&pq, &measurement, &events,
                                                   &now));
    const nd_event_record_t *record = nd_event_log_latest(&events.log);
    TEST_ASSERT_NOT_NULL(record);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_VOLTAGE_MISSING, record->kind);
    TEST_ASSERT_EQUAL_UINT8(82, events.latest_event_code);
    TEST_ASSERT_EQUAL_UINT8(1, record->source);
}

static void test_per_phase_statistics(void)
{
    nd_measurement_port_t measurement = {.read = fake_measurement};
    nd_pq_state_t pq;
    nd_pq_config_t config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .debounce_seconds = 1,
        .phase_count = 3,
    };
    nd_idis_pq_init(&pq, &config);
    nd_idis_event_state_t events = {0};
    nd_idis_event_init(&events, nd_idis_event_rules, nd_idis_event_rule_count);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    (void)nd_idis_pq_on_second(&pq, &measurement, &events, &now);
    TEST_ASSERT_EQUAL_UINT16(1, pq.sag_phase_count[0]);
    TEST_ASSERT_EQUAL_INT32(1900, pq.sag_phase_magnitude[0]);
    TEST_ASSERT_EQUAL_UINT32(1, pq.sag_phase_duration[0]);
    TEST_ASSERT_EQUAL_UINT16(0, pq.sag_phase_count[1]);
    TEST_ASSERT_EQUAL_UINT16(0, pq.swell_phase_count[0]);
}

static void test_pq_config_persists(void)
{
    static uint8_t memory[128];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_pq_state_t pq;
    nd_pq_config_t config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 3,
        .phase_count = 3,
    };
    nd_idis_pq_init(&pq, &config);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_pq_save(&pq, nd_byte_store_port(&store), 0));
    nd_pq_state_t restored;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_pq_load(&restored,
                                              nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_INT32(2000, restored.config.voltage_sag);
    TEST_ASSERT_EQUAL_UINT16(3, restored.config.debounce_seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sag_raises_domain_event);
    RUN_TEST(test_per_phase_statistics);
    RUN_TEST(test_missing_voltage_raises_cut_event);
    RUN_TEST(test_pq_config_persists);
    return UNITY_END();
}
