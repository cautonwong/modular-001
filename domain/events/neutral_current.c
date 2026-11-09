#include "domain/events/neutral_current.h"

static uint32_t magnitude(int32_t value)
{
    return value < 0 ? (uint32_t)(-(value + 1)) + 1u : (uint32_t)value;
}

nd_event_transition_t nd_neutral_current_step(nd_neutral_current_state_t *state,
                                              const nd_neutral_current_rule_t *rule,
                                              int32_t phase_current,
                                              int32_t neutral_current)
{
    uint32_t phase = magnitude(phase_current);
    uint32_t neutral = magnitude(neutral_current);
    uint32_t larger = phase > neutral ? phase : neutral;
    uint32_t difference = phase > neutral ? phase - neutral : neutral - phase;

    uint8_t invalid = rule->rate_threshold == 0
                      || (phase < rule->minimum_current
                          && neutral < rule->minimum_current);
    uint32_t rate = larger == 0
                        ? 0
                        : (uint32_t)(((uint64_t)difference * 10000) / larger);
    uint8_t condition = !invalid && rate > rule->rate_threshold;

    nd_event_transition_t transition =
        nd_event_duration_step(&state->debounce, rule->debounce_seconds, condition);

    if (state->debounce.active) {
        state->accumulated_seconds++;
    }
    return transition;
}
