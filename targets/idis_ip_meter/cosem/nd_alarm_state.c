#include "targets/idis_ip_meter/cosem/nd_alarm_state.h"

#include <string.h>

void nd_alarm_init(nd_alarm_state_t *state, const nd_idis_event_rule_t *rules,
                   uint16_t rule_count)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->rules = rules;
    state->rule_count = rule_count;
    state->filters[0] = UINT32_MAX;
    state->filters[1] = UINT32_MAX;
}

int nd_alarm_raise(nd_alarm_state_t *state, uint16_t event_code)
{
    if (state == NULL) {
        return -1;
    }
    for (uint16_t i = 0; i < state->rule_count; i++) {
        const nd_idis_event_rule_t *rule = &state->rules[i];
        if (rule->event_code != event_code) {
            continue;
        }
        if (rule->register_index > 1 || rule->bit > 31) {
            return -1;
        }
        uint32_t mask = (uint32_t)1u << rule->bit;
        if ((state->filters[rule->register_index] & mask) != 0) {
            state->registers[rule->register_index] |= mask;
        }
        return 0;
    }
    return -1;
}

int nd_alarm_clear(nd_alarm_state_t *state, uint8_t register_index,
                   uint32_t mask)
{
    if (state == NULL || register_index > 1) {
        return -1;
    }
    state->registers[register_index] &= ~mask;
    return 0;
}

int nd_alarm_filter_set(nd_alarm_state_t *state, uint8_t register_index,
                        uint32_t mask)
{
    if (state == NULL || register_index > 1) {
        return -1;
    }
    state->filters[register_index] = mask;
    return 0;
}

uint32_t nd_alarm_bits(const nd_alarm_state_t *state, uint8_t register_index)
{
    if (state == NULL || register_index > 1) {
        return 0;
    }
    return state->registers[register_index];
}
