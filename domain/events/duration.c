#include "domain/events/duration.h"

nd_event_transition_t nd_event_duration_step(nd_event_duration_t *state,
                                             uint8_t debounce_seconds,
                                             uint8_t condition)
{
    if (condition) {
        if (state->active) {
            state->duration = debounce_seconds;
        }
        if (state->duration < debounce_seconds) {
            state->duration++;
        } else if (!state->active) {
            state->active = 1;
            return ND_EVENT_STARTED;
        }
    } else {
        if (!state->active) {
            state->duration = 0;
        }
        if (state->duration > 0) {
            state->duration--;
        } else if (state->active) {
            state->active = 0;
            return ND_EVENT_ENDED;
        }
    }
    return ND_EVENT_NONE;
}

nd_event_transition_t nd_event_duration_start(nd_event_duration_t *state)
{
    if (state->active) {
        return ND_EVENT_NONE;
    }
    state->active = 1;
    return ND_EVENT_STARTED;
}
