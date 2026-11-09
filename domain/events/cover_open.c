#include "domain/events/cover_open.h"

nd_event_transition_t nd_cover_open_step(nd_cover_open_state_t *state,
                                         const nd_cover_open_rule_t *rule,
                                         uint8_t cover_open, uint8_t low_power)
{
    nd_event_transition_t transition =
        nd_event_duration_step(&state->debounce, rule->debounce_seconds, cover_open);

    if (state->debounce.active) {
        if (low_power) {
            state->accumulated_low_power_seconds++;
        } else {
            state->accumulated_seconds++;
        }
    }
    return transition;
}
