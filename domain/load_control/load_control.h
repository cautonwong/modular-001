#ifndef ND_DOMAIN_LOAD_CONTROL_LOAD_CONTROL_H
#define ND_DOMAIN_LOAD_CONTROL_LOAD_CONTROL_H

#include <stdint.h>

#include "domain/common/allocator.h"

#define ND_LOAD_REMOTE_NULL 0x00u
#define ND_LOAD_REMOTE_ON 0x01u
#define ND_LOAD_REMOTE_OFF 0x02u
#define ND_LOAD_REMOTE_ALARM 0x04u
#define ND_LOAD_REMOTE_PRETRIP_DIRECT 0x08u
#define ND_LOAD_REMOTE_PRETRIP_ALLOW 0x10u
#define ND_LOAD_REMOTE_ON_ALLOW 0x20u
#define ND_LOAD_REMOTE_ON_DIRECT 0x40u

typedef struct load_control_state load_control_state_t;

typedef struct {
    int32_t current;
    uint32_t voltage;
    uint32_t volt_low;
    uint32_t volt_high;
    uint32_t protection_current;
    uint32_t protection_minutes;
    uint8_t factory_mode;
    uint8_t key_pressed;
} nd_load_inputs_t;

typedef struct {
    uint8_t connect;
    uint8_t disconnect;
    uint8_t event_switch_on;
    uint8_t event_switch_off;
} nd_load_actions_t;

typedef struct {
    int32_t active_power;
    int32_t current;
    uint32_t relay_off_elapsed;
    uint32_t relay_on_elapsed;
    uint32_t feedback_threshold;
    uint32_t min_current;
    uint8_t second_parity;
    uint8_t full_display;
} nd_load_feedback_inputs_t;

typedef struct {
    uint8_t led_on;
    uint8_t led_blink;
    uint8_t misoperation;
    uint8_t event_switch_on;
    uint8_t event_switch_off;
} nd_load_feedback_t;

load_control_state_t *load_control_create(nd_allocator_t *allocator);

void load_control_destroy(load_control_state_t *state, nd_allocator_t *allocator);

void load_control_set_command(load_control_state_t *state, uint8_t command);

uint8_t load_control_command(const load_control_state_t *state);

void load_control_set_meter_clear(load_control_state_t *state, uint8_t meter_clear);

void load_control_set_timers(load_control_state_t *state, uint32_t alarm_minutes,
                             uint32_t current_limit_minutes, uint32_t limit_minutes);

uint32_t load_control_alarm_minutes(const load_control_state_t *state);

uint32_t load_control_current_limit_minutes(const load_control_state_t *state);

void load_control_request_repeat(load_control_state_t *state);

void load_control_second(load_control_state_t *state, const nd_load_inputs_t *inputs,
                         nd_load_actions_t *actions);

void load_control_feedback(load_control_state_t *state,
                           const nd_load_feedback_inputs_t *inputs,
                           nd_load_feedback_t *feedback);

#endif
