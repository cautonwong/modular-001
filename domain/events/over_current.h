#ifndef ND_DOMAIN_EVENTS_OVER_CURRENT_H
#define ND_DOMAIN_EVENTS_OVER_CURRENT_H

#include <stdint.h>

#include "domain/events/duration.h"

typedef struct {
    uint32_t threshold; /* caller applies the legacy /10 scaling */
    uint8_t debounce_seconds;
} nd_over_current_rule_t;

typedef struct {
    nd_event_duration_t debounce;
    uint32_t accumulated_seconds;
} nd_over_current_state_t;

nd_event_transition_t nd_over_current_step(nd_over_current_state_t *state,
                                           const nd_over_current_rule_t *rule,
                                           int32_t current);

#endif
