#include "domain/display/backlight_control.h"

void nd_rtc_battery_tick(uint32_t voltage, uint32_t low, uint32_t high,
                         nd_rtc_battery_state_t *state)
{
    uint8_t value = 0xFF;
    if (voltage <= low) {
        value = 1;
    } else if (voltage >= high) {
        value = 0;
    }

    if (value != 0xFF && state->undervoltage != value) {
        state->counter++;
    } else {
        state->counter = 0;
    }

    if (state->counter >= 3) {
        state->undervoltage = value;
        state->counter = 0;
    }
}

int nd_backlight_tick(const nd_backlight_inputs_t *inputs, uint16_t *timer)
{
    if (*timer > 0) {
        (*timer)--;
    }

    if (inputs->page_id == 0xFFFF) {
        return 1;
    }
    if (inputs->remote_alarm) {
        return 1;
    }
    if (inputs->alarm_minutes_active || inputs->current_limit_active) {
        return 1;
    }
    if ((inputs->alarm_word & 0x00400000u) == 0x00400000u
        && inputs->rtc_battery_undervoltage) {
        *timer = 3;
    }
    return *timer != 0;
}
