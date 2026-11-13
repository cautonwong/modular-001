#ifndef ND_DOMAIN_EVENTS_SWITCH_ERR_H
#define ND_DOMAIN_EVENTS_SWITCH_ERR_H

#include <stdint.h>

#include "domain/events/generic.h"

/*
 * Switch-error event. While the caller holds the event open, an active
 * condition keeps accumulating seconds without advancing the debounce state.
 */
typedef struct {
    nd_generic_event_state_t event;
} nd_switch_err_state_t;

nd_event_transition_t nd_switch_err_step(nd_switch_err_state_t *state,
                                         uint8_t debounce_seconds,
                                         uint8_t error, uint8_t hold);

#endif
