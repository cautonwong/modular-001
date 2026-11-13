#ifndef ND_ALARM_STATE_H
#define ND_ALARM_STATE_H

#include <stdint.h>

typedef struct {
    uint16_t event_code;
    uint8_t register_index; /* 0 or 1 */
    uint8_t bit;            /* 0..31 */
} nd_idis_event_rule_t;

typedef struct {
    uint32_t registers[2];
    uint32_t filters[2];
    const nd_idis_event_rule_t *rules;
    uint16_t rule_count;
} nd_alarm_state_t;

void nd_alarm_init(nd_alarm_state_t *state, const nd_idis_event_rule_t *rules,
                   uint16_t rule_count);
int nd_alarm_raise(nd_alarm_state_t *state, uint16_t event_code);
int nd_alarm_clear(nd_alarm_state_t *state, uint8_t register_index,
                   uint32_t mask);
int nd_alarm_filter_set(nd_alarm_state_t *state, uint8_t register_index,
                        uint32_t mask);
uint32_t nd_alarm_bits(const nd_alarm_state_t *state, uint8_t register_index);

#endif
