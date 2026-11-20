#ifndef ND_TESTS_SIM_SIM_H
#define ND_TESTS_SIM_SIM_H

#include "targets/stategrid_meter/app/stategrid_context.h"

#include <stdint.h>
typedef struct {
    unsigned cycles;
    unsigned arena_size;
    unsigned fail_malloc_at;
    const char *eeprom_path;
    const char *flash_path;
    const char *trace_path;
    const char *scenario_path;
    const char *uart_tx_path;
    const char *uart_tx_hex_path;
    const char *relay_log_path;
    const char *backlight_log_path;
    const char *lcd_log_path;
    const char *start_time;
    int verbose;
} sim_config_t;

extern sim_config_t g_sim;

int sim_devices_init(void);
void sim_meter_set_chip_fault(int fault);
void sim_meter_set_adc(uint8_t channel, int32_t value);
/* Fail every write through the board EEPROM driver; used to drive the
 * storage-fault ACK paths from the acceptance scenario grammar. */
void sim_eeprom_set_write_fault(int fail);
void sim_devices_close(void);

int sim_scenario_load(const char *path);
void sim_scenario_apply(stategrid_context_t *instance, unsigned cycle);
void sim_scenario_apply_stage(const char *name);
int sim_scenario_power_on(void);
int sim_scenario_relay_is_off(void);
int sim_scenario_cover_closed(void);
int sim_scenario_key_pressed(void);
int sim_scenario_key_released(void);

int sim_script_open(void);
void sim_script_close(void);
void sim_uart_bind(uint8_t channel, void *tx_fifo, void *rx_fifo);
void sim_uart_tx_byte(uint8_t byte, unsigned index);
void sim_uart_drain(void);

void sim_log_relay(const char *action);
void sim_log_backlight(int on);
void sim_log_lcd(const uint8_t *segments, unsigned len);

void nd_sim_lcd_snapshot(void);

int nd_sim_should_stop(stategrid_context_t *instance);
int nd_sim_malloc_should_fail(void);
unsigned nd_sim_malloc_count(void);
void nd_sim_after_run(stategrid_context_t *instance);
void nd_sim_filter(const char *pipeline, uint8_t filter_id, int ret);
void nd_sim_event(uint8_t from, uint8_t to, uint8_t ctrl, uint16_t len);
void nd_sim_stage(const char *name);
void nd_sim_storage_layout(uint32_t used, uint32_t region);
void nd_sim_system_reset(void);

void sim_trace_open(void);
void sim_trace_close(void);
void sim_trace_printf(const char *format, ...);

#endif /* ND_TESTS_SIM_SIM_H */
