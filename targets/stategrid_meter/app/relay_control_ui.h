#ifndef ND_TARGET_STATEGRID_RELAY_CONTROL_UI_H
#define ND_TARGET_STATEGRID_RELAY_CONTROL_UI_H

#include <stdint.h>

typedef struct relay_control_ui_state {
    uint8_t key_ticks;
    uint8_t led_flash;
} relay_control_ui_state_t;

typedef struct relay_control_led {
    uint8_t led_on;
    uint8_t led_blink;
} relay_control_led_t;

void relay_control_ui_key_tick(relay_control_ui_state_t *ui, uint8_t *command,
                               uint8_t key_pressed, uint8_t voltage_ok);

relay_control_led_t relay_control_ui_led(relay_control_ui_state_t *ui,
                                         uint8_t command, uint8_t relay_true,
                                         uint8_t second_parity);

#endif
