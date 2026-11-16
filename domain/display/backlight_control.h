#ifndef ND_DISPLAY_BACKLIGHT_CONTROL_H
#define ND_DISPLAY_BACKLIGHT_CONTROL_H

#include <stdint.h>

typedef struct {
    uint8_t counter;
    uint8_t undervoltage;
} nd_rtc_battery_state_t;

typedef struct {
    uint16_t page_id;
    uint8_t remote_alarm;
    uint8_t alarm_minutes_active;
    uint8_t current_limit_active;
    uint32_t alarm_word;
    uint8_t rtc_battery_undervoltage;
} nd_backlight_inputs_t;

/* Hysteresis plus three-sample debounce for the RTC battery voltage. */
void nd_rtc_battery_tick(uint32_t voltage, uint32_t low, uint32_t high,
                         nd_rtc_battery_state_t *state);

/* Returns 1 when the backlight must be on, 0 when it must be off. */
int nd_backlight_tick(const nd_backlight_inputs_t *inputs, uint16_t *timer);

#endif
