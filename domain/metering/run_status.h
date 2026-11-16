#ifndef ND_RUN_STATUS_H
#define ND_RUN_STATUS_H

#include <stdint.h>

/* Everything the meter run-state words depend on, gathered at the target. */
typedef struct {
    uint16_t battery_mv;
    uint8_t power_direction;
    uint8_t switch_error;
    uint8_t esam_error;
    uint8_t memory_error;
    uint8_t rtc_error;
    uint8_t over_current;
    uint8_t infrared_authenticated;
    uint8_t terminal_authenticated;
    uint8_t terminal_security;
    uint8_t master_security;
    uint8_t relay_alarm;
    uint8_t relay_commanded_off;
    uint8_t protected_supply;
    uint8_t relay_off;
    uint8_t cover_open;
    uint8_t power_off;
} nd_run_status_inputs_t;

/* Update the owned bits of the eight run-state words in place. */
void nd_run_status_update(const nd_run_status_inputs_t *inputs, uint16_t words[8]);

#endif
