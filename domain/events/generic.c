#include "domain/events/generic.h"

nd_event_transition_t nd_generic_event_step(nd_generic_event_state_t *state,
                                            uint8_t debounce_seconds,
                                            uint8_t condition)
{
    nd_event_transition_t transition =
        nd_event_duration_step(&state->debounce, debounce_seconds, condition);

    if (state->debounce.active) {
        state->accumulated_seconds++;
    }
    return transition;
}
