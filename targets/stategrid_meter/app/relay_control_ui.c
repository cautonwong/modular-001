#include "targets/stategrid_meter/app/relay_control_ui.h"

#include "domain/load_control/load_control.h"

void relay_control_ui_key_tick(relay_control_ui_state_t *ui, uint8_t *command,
                               uint8_t key_pressed, uint8_t voltage_ok)
{
    uint8_t value;

    if (!voltage_ok) {
        return;
    }
    value = *command;
    if ((value & ND_LOAD_REMOTE_ON) || value == ND_LOAD_REMOTE_NULL) {
        if ((value & ND_LOAD_REMOTE_ON)
            && (value & (ND_LOAD_REMOTE_OFF | ND_LOAD_REMOTE_ON_ALLOW))) {
            if (key_pressed) {
                if (ui->key_ticks < 3) {
                    ui->key_ticks++;
                }
                if (ui->key_ticks >= 3) {
                    value &= (uint8_t)~(ND_LOAD_REMOTE_OFF
                                        | ND_LOAD_REMOTE_ON_ALLOW);
                    value |= ND_LOAD_REMOTE_ON;
                    ui->key_ticks = 0;
                    ui->led_flash = 2;
                }
            } else {
                ui->key_ticks = 0;
            }
        }
    } else if (value & ND_LOAD_REMOTE_OFF) {
    } else if (value
               & (ND_LOAD_REMOTE_PRETRIP_DIRECT | ND_LOAD_REMOTE_PRETRIP_ALLOW)) {
    } else if (value & ND_LOAD_REMOTE_ON_ALLOW) {
        if (key_pressed) {
            if (ui->key_ticks < 3) {
                ui->key_ticks++;
            }
            if (ui->key_ticks >= 3) {
                value &= (uint8_t)~ND_LOAD_REMOTE_ON_ALLOW;
                ui->key_ticks = 0;
                ui->led_flash = 2;
            }
        } else {
            ui->key_ticks = 0;
        }
    }
    *command = value;
}

relay_control_led_t relay_control_ui_led(relay_control_ui_state_t *ui,
                                         uint8_t command, uint8_t relay_true,
                                         uint8_t second_parity)
{
    relay_control_led_t led;

    led.led_blink = (uint8_t)(((command & ND_LOAD_REMOTE_ON_ALLOW) && relay_true)
                              || ((command & ND_LOAD_REMOTE_ON)
                                  && (command & ND_LOAD_REMOTE_OFF))
                              || ui->led_flash > 0);
    if (led.led_blink && ui->led_flash > 0) {
        ui->led_flash--;
    }
    led.led_on = led.led_blink ? (uint8_t)(second_parity & 1) : relay_true;
    return led;
}
