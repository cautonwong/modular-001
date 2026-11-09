#include "domain/events/over_current.h"

nd_event_transition_t nd_over_current_step(nd_over_current_state_t *state,
                                           const nd_over_current_rule_t *rule,
                                           int32_t current)
{
    uint8_t condition =
        rule->threshold != 0 && current > (int32_t)rule->threshold;

    nd_event_transition_t transition =
        nd_event_duration_step(&state->debounce, rule->debounce_seconds, condition);

    if (state->debounce.active) {
        state->accumulated_seconds++;
    }
    return transition;
}
