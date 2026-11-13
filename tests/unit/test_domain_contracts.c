#include "unity.h"

#include "domain/common/clock_value.h"
#include "domain/events/event_record.h"
#include "domain/freeze/capture.h"
#include "domain/load_control/disconnect.h"
#include "domain/metering/demand_history.h"
#include "domain/metering/energy_set.h"
#include "domain/metering/measurement.h"
#include "domain/tariff/activity_calendar.h"

void setUp(void) {}
void tearDown(void) {}

static void test_measurement_value_keeps_semantics(void)
{
    nd_measurement_value_t value = {
        .value = -1234,
        .scaler = -2,
        .unit = ND_MEASUREMENT_UNIT_VOLT,
        .phase = ND_MEASURE_PHASE_L2,
        .status = 0,
    };

    TEST_ASSERT_EQUAL_INT64(-1234, value.value);
    TEST_ASSERT_EQUAL_INT8(-2, value.scaler);
    TEST_ASSERT_EQUAL_UINT8(ND_MEASURE_PHASE_L2, value.phase);
}

static void test_event_record_is_domain_only(void)
{
    nd_event_record_t event = {
        .kind = ND_EVENT_OVER_CURRENT,
        .time = {.year = 2026, .month = 11, .day = 11},
        .source = 1,
        .severity = 2,
        .duration_seconds = 5,
        .value = 123,
    };

    TEST_ASSERT_EQUAL_INT(ND_EVENT_OVER_CURRENT, event.kind);
    TEST_ASSERT_EQUAL_UINT32(5, event.duration_seconds);
    TEST_ASSERT_EQUAL_INT32(123, event.value);
}

static void test_clock_and_disconnect_contracts(void)
{
    nd_clock_value_t clock = {
        .time = {.year = 2026, .month = 11, .day = 11},
        .hundredths = 25,
        .deviation = 60,
        .status = 0,
    };
    nd_disconnect_snapshot_t disconnect = {
        .control_state = ND_DISCONNECT_CONNECTED,
        .output_state = 1,
        .control_mode = 2,
        .last_trigger = ND_DISCONNECT_REMOTE_RECONNECT,
    };

    TEST_ASSERT_EQUAL_UINT8(25, clock.hundredths);
    TEST_ASSERT_EQUAL_INT16(60, clock.deviation);
    TEST_ASSERT_EQUAL_INT(ND_DISCONNECT_CONNECTED, disconnect.control_state);
    TEST_ASSERT_EQUAL_UINT8(1, disconnect.output_state);
}

static void test_energy_demand_and_calendar_contracts(void)
{
    nd_energy_set_t energy = {0};
    energy.active_import.slots[1].integer = 100;
    energy.reactive_q4.slots[2].fraction = 7;

    nd_demand_history_t demand = {
        .last_value = 500,
        .last_capture_time = {.year = 2026, .month = 11, .day = 11},
        .period_start = {.year = 2026, .month = 11, .day = 1},
        .status = 0,
    };

    nd_calendar_action_t action = {
        .start = {.hour = 8, .minute = 0},
        .script_ln = {0, 0, 10, 0, 0, 255},
        .script_selector = 1,
    };
    nd_calendar_day_t day = {.day_id = 1, .action_count = 1, .actions = &action};
    nd_calendar_bank_t bank = {
        .name = {.len = 3, .data = {'A', 'B', 'C'}},
        .day_count = 1,
        .days = &day,
    };
    nd_activity_calendar_t calendar = {.active = bank, .has_passive = 0};

    TEST_ASSERT_EQUAL_UINT32(100, energy.active_import.slots[1].integer);
    TEST_ASSERT_EQUAL_UINT16(7, energy.reactive_q4.slots[2].fraction);
    TEST_ASSERT_EQUAL_UINT32(500, demand.last_value);
    TEST_ASSERT_EQUAL_UINT8(1, calendar.active.day_count);
    TEST_ASSERT_EQUAL_UINT16(1, calendar.active.days[0].actions[0].script_selector);
}

static void test_capture_value_is_tagged(void)
{
    nd_capture_value_t values[2] = {
        {.type = ND_CAPTURE_UNSIGNED, .value.unsigned_value = 42},
        {.type = ND_CAPTURE_CLOCK,
         .value.clock = {.time = {.year = 2026, .month = 11, .day = 11}}},
    };
    nd_capture_snapshot_t snapshot = {
        .capture_time = {.time = {.year = 2026, .month = 11, .day = 11}},
        .count = 2,
        .values = values,
    };

    TEST_ASSERT_EQUAL_INT(ND_CAPTURE_UNSIGNED, snapshot.values[0].type);
    TEST_ASSERT_EQUAL_UINT64(42, snapshot.values[0].value.unsigned_value);
    TEST_ASSERT_EQUAL_UINT16(2026, snapshot.values[1].value.clock.time.year);
    TEST_ASSERT_EQUAL_UINT8(2, snapshot.count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_measurement_value_keeps_semantics);
    RUN_TEST(test_event_record_is_domain_only);
    RUN_TEST(test_clock_and_disconnect_contracts);
    RUN_TEST(test_energy_demand_and_calendar_contracts);
    RUN_TEST(test_capture_value_is_tagged);
    return UNITY_END();
}
