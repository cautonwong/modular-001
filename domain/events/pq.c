#include "domain/events/pq.h"

#include <string.h>

void nd_pq_init(nd_pq_state_t *state, const nd_pq_config_t *config)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    if (config != NULL) {
        state->config = *config;
    }
    if (state->config.phase_count == 0) {
        state->config.phase_count = 3;
    }
}

static uint8_t emit(nd_event_record_t *events, uint8_t max_events,
                    uint8_t count, nd_event_kind_t kind,
                    const nd_datetime_t *now, uint8_t source, int32_t value,
                    uint32_t duration)
{
    if (count >= max_events) {
        return count;
    }
    events[count] = (nd_event_record_t){
        .kind = kind,
        .time = *now,
        .source = source,
        .severity = 2,
        .duration_seconds = duration,
        .value = value,
    };
    return (uint8_t)(count + 1);
}

static uint8_t debounce(uint16_t *counter, uint8_t *active, uint16_t limit,
                        uint8_t condition)
{
    if (!condition) {
        *counter = 0;
        *active = 0;
        return 0;
    }
    if (*counter < limit) {
        (*counter)++;
    }
    if (*counter >= limit && !*active) {
        *active = 1;
        return 1;
    }
    return 0;
}

static uint16_t effective_time(uint16_t specific, uint16_t fallback)
{
    uint16_t value = specific != 0 ? specific : fallback;
    return value == 0 ? 1 : value;
}

static void phase_stat(uint16_t *timer, uint8_t *active, uint16_t *count,
                       uint32_t *duration, int32_t *magnitude, uint16_t limit,
                       uint8_t condition, int32_t value, int sag)
{
    if (condition) {
        if (*timer < 0xFFFF) {
            (*timer)++;
        }
        if (*timer >= limit && !*active) {
            *active = 1;
            (*count)++;
            *magnitude = value;
        }
        if (*active) {
            if (sag ? value < *magnitude : value > *magnitude) {
                *magnitude = value;
            }
            *duration = *timer;
        }
    } else {
        if (*active) {
            *duration = *timer;
        }
        *active = 0;
        *timer = 0;
    }
}

uint8_t nd_pq_on_second(nd_pq_state_t *state,
                        const nd_measurement_value_t voltage[3],
                        const nd_measurement_value_t *frequency,
                        const nd_datetime_t *now,
                        nd_event_record_t *events, uint8_t max_events)
{
    if (state == NULL || voltage == NULL || now == NULL || events == NULL) {
        return 0;
    }
    uint16_t fallback = state->config.debounce_seconds;
    uint16_t sag_time =
        effective_time(state->config.sag_debounce_seconds, fallback);
    uint16_t swell_time =
        effective_time(state->config.swell_debounce_seconds, fallback);
    uint16_t cut_time =
        effective_time(state->config.cut_debounce_seconds, fallback);
    uint16_t other_time = effective_time(0, fallback);
    uint8_t count = 0;
    int32_t min_voltage = voltage[0].value;
    int32_t max_voltage = voltage[0].value;
    uint8_t min_phase = 1;
    uint8_t max_phase = 1;
    int64_t sum = 0;
    uint8_t phases = state->config.phase_count == 0 ? 3
                                                    : state->config.phase_count;
    for (uint8_t phase = 0; phase < phases; phase++) {
        if (voltage[phase].value < min_voltage) {
            min_voltage = voltage[phase].value;
            min_phase = (uint8_t)(phase + 1);
        }
        if (voltage[phase].value > max_voltage) {
            max_voltage = voltage[phase].value;
            max_phase = (uint8_t)(phase + 1);
        }
        sum += voltage[phase].value;
        if (phase < 3) {
            phase_stat(&state->sag_phase_timer[phase],
                       &state->sag_phase_active[phase],
                       &state->sag_phase_count[phase],
                       &state->sag_phase_duration[phase],
                       &state->sag_phase_magnitude[phase], sag_time,
                       state->config.voltage_sag > 0
                           && voltage[phase].value < state->config.voltage_sag,
                       voltage[phase].value, 1);
            phase_stat(&state->swell_phase_timer[phase],
                       &state->swell_phase_active[phase],
                       &state->swell_phase_count[phase],
                       &state->swell_phase_duration[phase],
                       &state->swell_phase_magnitude[phase], swell_time,
                       state->config.voltage_swell > 0
                           && voltage[phase].value > state->config.voltage_swell,
                       voltage[phase].value, 0);
        }
    }
    uint8_t sag = state->config.voltage_sag > 0
        && min_voltage < state->config.voltage_sag;
    if (debounce(&state->sag_count, &state->sag_active, sag_time, sag)) {
        count = emit(events, max_events, count, ND_EVENT_VOLTAGE_SAG, now,
                     min_phase, min_voltage, sag_time);
    }
    uint8_t swell = state->config.voltage_swell > 0
        && max_voltage > state->config.voltage_swell;
    if (debounce(&state->swell_count, &state->swell_active, swell_time,
                 swell)) {
        count = emit(events, max_events, count, ND_EVENT_VOLTAGE_SWELL, now,
                     max_phase, max_voltage, swell_time);
    }
    uint8_t cut = state->config.voltage_cut > 0
        && min_voltage < state->config.voltage_cut;
    if (debounce(&state->cut_count, &state->cut_active, cut_time, cut)) {
        count = emit(events, max_events, count, ND_EVENT_VOLTAGE_MISSING, now,
                     min_phase, min_voltage, cut_time);
    }
    if (frequency != NULL) {
        uint8_t low = state->config.frequency_min > 0
            && frequency->value < state->config.frequency_min;
        if (debounce(&state->frequency_low_count,
                     &state->frequency_low_active, other_time, low)) {
            count = emit(events, max_events, count, ND_EVENT_FREQUENCY_LOW, now,
                         0, (int32_t)frequency->value, other_time);
        }
        uint8_t high = state->config.frequency_max > 0
            && frequency->value > state->config.frequency_max;
        if (debounce(&state->frequency_high_count,
                     &state->frequency_high_active, other_time, high)) {
            count = emit(events, max_events, count, ND_EVENT_FREQUENCY_HIGH,
                         now, 0, (int32_t)frequency->value, other_time);
        }
    }
    if (phases == 3 && state->config.unbalance_permille > 0) {
        int32_t average = (int32_t)(sum / phases);
        int32_t deviation = 0;
        for (uint8_t phase = 0; phase < phases; phase++) {
            int32_t diff = voltage[phase].value - average;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff > deviation) {
                deviation = diff;
            }
        }
        int32_t permille = average == 0 ? 0 : deviation * 1000 / average;
        uint8_t unbalance = permille > state->config.unbalance_permille;
        if (debounce(&state->unbalance_count, &state->unbalance_active,
                     other_time, unbalance)) {
            count = emit(events, max_events, count,
                         ND_EVENT_VOLTAGE_UNBALANCE, now, 0, permille,
                         other_time);
        }
    }
    return count;
}
