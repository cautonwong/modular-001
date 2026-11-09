#include "domain/events/power_off.h"

nd_event_transition_t nd_power_off_step(nd_power_off_state_t *state,
                                        const nd_power_off_rule_t *rule,
                                        const uint32_t *phase_voltages,
                                        uint8_t phase_count, uint32_t threshold,
                                        uint8_t low_power)
{
    uint8_t condition = phase_count != 0;
    for (uint8_t phase = 0; phase < phase_count; phase++) {
        if (phase_voltages[phase] >= threshold) {
            condition = 0;
            break;
        }
    }

    nd_event_transition_t transition =
        nd_event_duration_step(&state->debounce, rule->debounce_seconds, condition);

    if (state->debounce.active) {
        if (low_power) {
            state->accumulated_low_power_seconds++;
        } else {
            state->accumulated_seconds++;
        }
    }
    return transition;
}
