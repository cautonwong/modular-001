#include "domain/load_control/load_control.h"

#include <string.h>

struct load_control_state {
    uint8_t command;
    uint32_t alarm_minutes;
    uint32_t current_limit_minutes;
    uint32_t limit_minutes;
    uint32_t key_ticks;
    uint8_t switch_on;
    uint8_t last_switch_on;
    uint8_t last_switch_known;
    uint8_t led_flash;
    uint8_t relay_do;
    uint8_t meter_clear;
    uint8_t switch_on_success;
    uint8_t switch_off_success;
    uint8_t relay_error_record;
};

static uint32_t magnitude(int32_t value)
{
    return value < 0 ? (uint32_t)(-(value + 1)) + 1u : (uint32_t)value;
}

load_control_state_t *load_control_create(nd_allocator_t *allocator)
{
    load_control_state_t *state = nd_alloc(allocator, sizeof(*state));
    if (state == NULL) {
        return NULL;
    }
    state->switch_on = 1;
    return state;
}

void load_control_destroy(load_control_state_t *state, nd_allocator_t *allocator)
{
    nd_dealloc(allocator, state, sizeof(*state));
}

void load_control_set_command(load_control_state_t *state, uint8_t command)
{
    state->command = command;
}

uint8_t load_control_command(const load_control_state_t *state)
{
    return state->command;
}

void load_control_set_meter_clear(load_control_state_t *state, uint8_t meter_clear)
{
    state->meter_clear = meter_clear;
}

void load_control_set_timers(load_control_state_t *state, uint32_t alarm_minutes,
                             uint32_t current_limit_minutes, uint32_t limit_minutes)
{
    state->alarm_minutes = alarm_minutes;
    state->current_limit_minutes = current_limit_minutes;
    state->limit_minutes = limit_minutes;
}

uint32_t load_control_alarm_minutes(const load_control_state_t *state)
{
    return state->alarm_minutes;
}

uint32_t load_control_current_limit_minutes(const load_control_state_t *state)
{
    return state->current_limit_minutes;
}

void load_control_request_repeat(load_control_state_t *state)
{
    state->relay_do = 1;
}

static void tick_timers(load_control_state_t *state, const nd_load_inputs_t *inputs)
{
    if (state->alarm_minutes > 0) {
        state->alarm_minutes--;
        if (state->alarm_minutes == 0) {
            uint32_t limit = inputs->protection_current / 10;
            if (inputs->protection_current != 0 && magnitude(inputs->current) > limit) {
                state->current_limit_minutes = inputs->protection_minutes * 60;
            } else {
                state->current_limit_minutes = 0;
            }
        }
    } else if (state->current_limit_minutes > 0) {
        state->current_limit_minutes--;
        uint32_t limit = inputs->protection_current / 10;
        if (inputs->protection_current == 0 || magnitude(inputs->current) < limit) {
            state->current_limit_minutes = 0;
        }
    }

    if (state->current_limit_minutes == 0 && state->limit_minutes > 0) {
        state->limit_minutes--;
    }
}

void load_control_second(load_control_state_t *state, const nd_load_inputs_t *inputs,
                         nd_load_actions_t *actions)
{
    memset(actions, 0, sizeof(*actions));
    tick_timers(state, inputs);

    if (!inputs->factory_mode
        && (inputs->voltage < inputs->volt_low || inputs->voltage > inputs->volt_high)) {
        return;
    }

    uint8_t command = state->command;
    if ((command & ND_LOAD_REMOTE_ON) || command == ND_LOAD_REMOTE_NULL) {
        if ((command & ND_LOAD_REMOTE_ON)
            && ((command & ND_LOAD_REMOTE_OFF) || (command & ND_LOAD_REMOTE_ON_ALLOW))) {
            if (inputs->key_pressed) {
                if (state->key_ticks < 3) {
                    state->key_ticks++;
                }
                if (state->key_ticks >= 3) {
                    state->switch_on = 1;
                    command &= (uint8_t)~ND_LOAD_REMOTE_OFF;
                    command &= (uint8_t)~ND_LOAD_REMOTE_ON_ALLOW;
                    state->key_ticks = 0;
                    state->led_flash = 2;
                }
            } else {
                state->key_ticks = 0;
            }
        } else {
            state->switch_on = 1;
        }
    } else if (command & ND_LOAD_REMOTE_OFF) {
        if (state->alarm_minutes == 0 && state->current_limit_minutes == 0) {
            state->switch_on = 0;
        }
    } else if (command
               & (ND_LOAD_REMOTE_PRETRIP_DIRECT | ND_LOAD_REMOTE_PRETRIP_ALLOW)) {
        if (state->current_limit_minutes == 0) {
            state->switch_on = 0;
            if (state->limit_minutes == 0) {
                if (command & ND_LOAD_REMOTE_PRETRIP_DIRECT) {
                    state->switch_on = 1;
                } else if (command & ND_LOAD_REMOTE_PRETRIP_ALLOW) {
                    command |= ND_LOAD_REMOTE_ON_ALLOW;
                }
                command &= (uint8_t)~(ND_LOAD_REMOTE_PRETRIP_DIRECT
                                      | ND_LOAD_REMOTE_PRETRIP_ALLOW);
            }
        }
    } else if (command & ND_LOAD_REMOTE_ON_ALLOW) {
        if (inputs->key_pressed) {
            if (state->key_ticks < 3) {
                state->key_ticks++;
            }
            if (state->key_ticks >= 3) {
                state->switch_on = 1;
                command &= (uint8_t)~ND_LOAD_REMOTE_ON_ALLOW;
                state->key_ticks = 0;
                state->led_flash = 2;
            }
        } else {
            state->key_ticks = 0;
        }
    } else {
        state->switch_on = 1;
    }

    if (!state->last_switch_known) {
        state->last_switch_on = state->switch_on;
        state->last_switch_known = 1;
    }

    if (state->last_switch_on != state->switch_on) {
        if (state->switch_on) {
            actions->connect = 1;
            if (state->meter_clear != 0x5A) {
                actions->event_switch_on = 1;
                state->switch_on_success = 0x5A;
            }
        } else {
            actions->disconnect = 1;
            actions->event_switch_off = 1;
            state->switch_off_success = 0x5A;
        }
        state->relay_error_record = 0x5A;
        state->last_switch_on = state->switch_on;
        state->relay_do = 0;
    }
    state->meter_clear = 0;

    if (state->relay_do) {
        if (state->switch_on) {
            actions->connect = 1;
        } else {
            actions->disconnect = 1;
        }
        state->relay_do = 0;
    }

    state->command = command;
}
