#ifndef ND_DOMAIN_EVENTS_GENERIC_H
#define ND_DOMAIN_EVENTS_GENERIC_H

#include <stdint.h>

#include "domain/events/duration.h"

/* For events whose condition is already a single boolean. */
typedef struct {
    nd_event_duration_t debounce;
    uint32_t accumulated_seconds;
} nd_generic_event_state_t;

nd_event_transition_t nd_generic_event_step(nd_generic_event_state_t *state,
                                            uint8_t debounce_seconds,
                                            uint8_t condition);

#endif
