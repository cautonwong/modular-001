#ifndef ND_DOMAIN_EVENTS_PQ_H
#define ND_DOMAIN_EVENTS_PQ_H

#include <stdint.h>

#include "domain/events/event_record.h"
#include "domain/metering/measurement.h"

typedef struct {
    int32_t voltage_sag; /* event when any phase is below this value */
    int32_t voltage_swell; /* event when any phase is above this value */
    int32_t frequency_min;
    int32_t frequency_max;
    int32_t unbalance_permille; /* max deviation / average * 1000 */
    uint16_t debounce_seconds; /* default when a specific time is zero */
    uint8_t phase_count; /* 1 or 3 */
    int32_t voltage_cut; /* missing-voltage threshold; 0 disables */
    uint16_t sag_debounce_seconds;
    uint16_t swell_debounce_seconds;
    uint16_t cut_debounce_seconds;
} nd_pq_config_t;

typedef struct {
    nd_pq_config_t config;
    uint16_t sag_count;
    uint16_t swell_count;
    uint16_t frequency_low_count;
    uint16_t frequency_high_count;
    uint16_t unbalance_count;
    uint16_t cut_count;
    uint8_t sag_active;
    uint8_t swell_active;
    uint8_t frequency_low_active;
    uint8_t frequency_high_active;
    uint8_t unbalance_active;
    uint8_t cut_active;
    /* Per-phase statistics exposed as IC3 registers (1-0:32/52/72.x). */
    uint16_t sag_phase_count[3];
    uint32_t sag_phase_duration[3];
    int32_t sag_phase_magnitude[3];
    uint16_t sag_phase_timer[3];
    uint8_t sag_phase_active[3];
    uint16_t swell_phase_count[3];
    uint32_t swell_phase_duration[3];
    int32_t swell_phase_magnitude[3];
    uint16_t swell_phase_timer[3];
    uint8_t swell_phase_active[3];
} nd_pq_state_t;

void nd_pq_init(nd_pq_state_t *state, const nd_pq_config_t *config);
/* Returns the number of events emitted (0..5). */
uint8_t nd_pq_on_second(nd_pq_state_t *state,
                        const nd_measurement_value_t voltage[3],
                        const nd_measurement_value_t *frequency,
                        const nd_datetime_t *now,
                        nd_event_record_t *events, uint8_t max_events);

#endif
