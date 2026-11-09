#ifndef ND_DOMAIN_EVENTS_POWER_OFF_H
#define ND_DOMAIN_EVENTS_POWER_OFF_H

#include <stdint.h>

#include "domain/events/duration.h"

typedef struct {
    uint8_t debounce_seconds;
} nd_power_off_rule_t;

typedef struct {
    nd_event_duration_t debounce;
    uint32_t accumulated_seconds;
    uint32_t accumulated_low_power_seconds;
} nd_power_off_state_t;

/*
 * The event holds while every phase stays below the threshold. An empty phase
 * list never raises the event.
 */
nd_event_transition_t nd_power_off_step(nd_power_off_state_t *state,
                                        const nd_power_off_rule_t *rule,
                                        const uint32_t *phase_voltages,
                                        uint8_t phase_count, uint32_t threshold,
                                        uint8_t low_power);

#endif
