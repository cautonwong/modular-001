#ifndef ND_DOMAIN_EVENTS_COVER_OPEN_H
#define ND_DOMAIN_EVENTS_COVER_OPEN_H

#include <stdint.h>

#include "domain/events/duration.h"

typedef struct {
    uint8_t debounce_seconds;
} nd_cover_open_rule_t;

typedef struct {
    nd_event_duration_t debounce;
    uint32_t accumulated_seconds;
    uint32_t accumulated_low_power_seconds;
} nd_cover_open_state_t;

nd_event_transition_t nd_cover_open_step(nd_cover_open_state_t *state,
                                         const nd_cover_open_rule_t *rule,
                                         uint8_t cover_open, uint8_t low_power);

#endif
