#include "domain/events/switch_err.h"

nd_event_transition_t nd_switch_err_step(nd_switch_err_state_t *state,
                                         uint8_t debounce_seconds,
                                         uint8_t error, uint8_t hold)
{
    if (hold && error) {
        if (state->event.debounce.active) {
            state->event.accumulated_seconds++;
        }
        return ND_EVENT_NONE;
    }
    return nd_generic_event_step(&state->event, debounce_seconds, error);
}
