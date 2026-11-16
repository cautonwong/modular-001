#include "domain/metering/run_status.h"

#define RUN_STATE_BAT_LOW 0x0020u
#define RUN_STATE_REVERSE 0x0008u
#define RUN_STATE_RELAY_ERROR 0x8000u
#define RUN_STATE_ESAM_ERROR 0x4000u
#define RUN_STATE_EEPROM_ERROR 0x0400u
#define RUN_STATE_CLOCK_ERROR 0x0100u
#define RUN_STATE_NEGATIVE 0x0080u
#define RUN_STATE_IMAX_OVER 0x0008u
#define RUN_STATE_IR_AUTH 0x0010u
#define RUN_STATE_TERM_ID_AUTH 0x2000u
#define RUN_STATE_TERM_AUTH 0x1000u
#define RUN_STATE_MASTER_AUTH 0x0400u
#define RUN_STATE_RELAY_ALARM 0x0001u
#define RUN_STATE_RELAY_COMMAND 0x0002u
#define RUN_STATE_PROTECTED_SUPPLY 0x0800u
#define RUN_STATE_RELAY_OFF 0x0008u
#define RUN_STATE_COVER_OPEN 0x4000u
#define RUN_STATE_POWER_OFF 0x0004u

static void set_bit(uint16_t *word, uint16_t mask, int on)
{
    if (on) {
        *word = (uint16_t)(*word | mask);
    } else {
        *word = (uint16_t)(*word & (uint16_t)~mask);
    }
}

void nd_run_status_update(const nd_run_status_inputs_t *inputs, uint16_t words[8])
{
    int reverse = inputs->power_direction == 1;

    set_bit(&words[1], RUN_STATE_BAT_LOW, inputs->battery_mv < 300);
    set_bit(&words[1], RUN_STATE_REVERSE, reverse);
    set_bit(&words[2], RUN_STATE_NEGATIVE, reverse);
    set_bit(&words[1], RUN_STATE_RELAY_ERROR, inputs->switch_error != 0);
    set_bit(&words[1], RUN_STATE_ESAM_ERROR, inputs->esam_error != 0);
    set_bit(&words[1], RUN_STATE_EEPROM_ERROR, inputs->memory_error != 0);
    set_bit(&words[1], RUN_STATE_CLOCK_ERROR, inputs->rtc_error != 0);

    set_bit(&words[4], RUN_STATE_IMAX_OVER, inputs->over_current != 0);

    set_bit(&words[3], RUN_STATE_IR_AUTH, inputs->infrared_authenticated != 0);
    set_bit(&words[3], RUN_STATE_TERM_ID_AUTH,
            inputs->terminal_authenticated != 0);
    set_bit(&words[3], RUN_STATE_TERM_AUTH, inputs->terminal_security != 0);
    set_bit(&words[3], RUN_STATE_MASTER_AUTH, inputs->master_security != 0);
    set_bit(&words[3], RUN_STATE_RELAY_ALARM, inputs->relay_alarm != 0);
    set_bit(&words[3], RUN_STATE_RELAY_COMMAND,
            inputs->relay_commanded_off != 0 && inputs->protected_supply == 0);
    set_bit(&words[3], RUN_STATE_PROTECTED_SUPPLY, inputs->protected_supply != 0);
    set_bit(&words[3], RUN_STATE_RELAY_OFF, inputs->relay_off != 0);

    set_bit(&words[7], RUN_STATE_COVER_OPEN, inputs->cover_open != 0);
    set_bit(&words[7], RUN_STATE_POWER_OFF, inputs->power_off != 0);
}
