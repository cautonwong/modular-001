#include "sim.h"
#include "infra/boards/stategrid_single_phase/relay_config.h"

#include <stdint.h>

#include "cpu_surport.h"
#include "driver_entry.h"
#include "nidavellir.h"
#include "infra/boards/stategrid_single_phase/ndvh_rn8209.h"
#include "infra/boards/stategrid_single_phase/board_io.h"

/*
 * Hardware entry points that are not linked into the simulation. They are
 * deliberately inert: the simulation exercises the firmware's control flow,
 * resource manager, persistence and domain logic, not the MCU peripherals.
 */

const uint32_t baud_table[11] = {
    300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200,
};

void mcu_init(void *init_type) { (void)init_type; }
void mcu_clear_watchdog(void) {}
uint8_t mcu_delay_us(volatile uint16_t count) { (void)count; return 0; }
int mcu_delay_ms_power_down_quit(volatile uint16_t count) { (void)count; return 0; }
uint8_t ac_power_off_monitor(void) { return (uint8_t)!sim_scenario_power_on(); }
uint8_t ac_power_on_monitor(void) { return (uint8_t)sim_scenario_power_on(); }
uint8_t mcu_uart_set_baud(uint32_t baud, uint8_t index) { (void)baud; (void)index; return 0; }
uint8_t mcu_uart_init(void *init_config, uint8_t index, uint8_t infrared_enable)
{
    (void)init_config;
    (void)index;
    (void)infrared_enable;
    return 0;
}
uint8_t mcu_spi_bus_init(uint8_t spi_index, uint8_t mode) { (void)spi_index; (void)mode; return 0; }
uint8_t mcu_spi_send_byte(uint8_t byte, uint8_t spi_index) { (void)byte; (void)spi_index; return 0; }
uint8_t mcu_spi_recive_byte(uint8_t *byte, uint8_t spi_index) { (void)byte; (void)spi_index; return 0; }
static uint8_t sim_rtc_set[7];
static int sim_rtc_set_valid;

uint8_t write_mcu_rtc(uint8_t addr, uint8_t *buf, uint16_t len)
{
    (void)addr;
    if (buf != NULL && len >= sizeof(sim_rtc_set)) {
        memcpy(sim_rtc_set, buf, sizeof(sim_rtc_set));
        sim_rtc_set_valid = 1;
    }
    return 0;
}
uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *isr, uint8_t irq)
{
    (void)isr;
    (void)irq;
    return 0;
}
void cm0_systick_init(uint8_t cmd) { (void)cmd; }
static int32_t sim_adc_values[256];
static uint8_t sim_adc_valid[256];

void sim_meter_set_adc(uint8_t channel, int32_t value)
{
    sim_adc_values[channel] = value;
    sim_adc_valid[channel] = 1;
}

uint8_t mcu_adc_start_once(uint8_t chl_type) { (void)chl_type; return 1; }
uint32_t read_mcu_adc_measure_value(uint8_t chl_type, int32_t *adc_measure)
{
    if (adc_measure == NULL || !sim_adc_valid[chl_type]) {
        return 0;
    }
    *adc_measure = sim_adc_values[chl_type];
    return 1;
}
void opreate_second_pluse(uint8_t mode) { (void)mode; }
void cpu_io_lowpower_cfg_chk(struct instance_t *instance) { (void)instance; }
void mcu_uart_send_enable(uint8_t enable, size_t index) { (void)enable; (void)index; }
void mcu_uart_recive_enable(uint8_t enable, size_t index) { (void)enable; (void)index; }
int mcu_uart_send_byte(uint8_t byte, size_t index)
{
    sim_uart_tx_byte(byte, (unsigned)index);
    return 0;
}
int mcu_uart_recive_byte(uint8_t *byte, size_t index) { (void)byte; (void)index; return -1; }
uint8_t mcu_uart_close(uint8_t index) { (void)index; return 0; }
uint8_t mcu_uart_get_status(uint8_t index, uint8_t status) { (void)index; (void)status; return 0; }
void mcu_timer_ctrl(uint32_t period_ms, uint8_t index, uint8_t stop_or_start)
{
    (void)period_ms;
    (void)index;
    (void)stop_or_start;
}
void mcu_main_frequency_mode(uint8_t clk_mode) { (void)clk_mode; }
void mcu_enter_lowpower_then_sleep(void) {}
void mcu_exit_sleep_then_enter_normal(void) {}
void mcu_main_power_on_config(uint8_t power_mode) { (void)power_mode; }
void mcu_main_power_off_config(void) {}
void mcu_main_power_off_config1(void) {}
void mcu_main_power_on_config1(void) {}
void mcu_power_off(void) {}
void mcu_power_on(void) {}
uint8_t mcu_get_power_status(void) { return 0; }

uint8_t read_mcu_rtc(uint8_t addr, struct instance_t *instance)
{
    (void)addr;
    if (instance != NULL && sim_rtc_set_valid) {
        instance->time.tm_year = 2000 + ((sim_rtc_set[5] >> 4) * 10)
                                 + (sim_rtc_set[5] & 0x0F) - 1900;
        instance->time.tm_mon = ((sim_rtc_set[4] >> 4) * 10) + (sim_rtc_set[4] & 0x0F) - 1;
        instance->time.tm_mday = ((sim_rtc_set[3] >> 4) * 10) + (sim_rtc_set[3] & 0x0F);
        instance->time.tm_hour = ((sim_rtc_set[2] >> 4) * 10) + (sim_rtc_set[2] & 0x0F);
        instance->time.tm_min = ((sim_rtc_set[1] >> 4) * 10) + (sim_rtc_set[1] & 0x0F);
        instance->time.tm_sec = ((sim_rtc_set[0] >> 4) * 10) + (sim_rtc_set[0] & 0x0F);
        sim_rtc_set_valid = 0;
        return 0;
    }
    /*
     * Rtc_Prossecc() increments tm_sec in software except at second 20, when
     * it re-synchronizes from the hardware RTC. A real RTC has advanced by
     * then; model that by advancing the simulated clock one second.
     */
    if (instance != NULL && instance->time.tm_sec == 20) {
        instance->time.tm_sec++;
        rtc_tm_normalize(&instance->time);
    }
    return 0;
}

void cpu_sleep_manage(struct instance_t *instance, struct pipeline_t *pipeline)
{
    (void)instance;
    (void)pipeline;
}

void device_power_close(struct instance_t *instance) { (void)instance; }
void device_power_close1(struct instance_t *instance) { (void)instance; }
void device_power_open(struct instance_t *instance, struct pipeline_t *pipeline)
{
    (void)instance;
    (void)pipeline;
}

void irq_transform_to_instance(struct instance_t *instance) { (void)instance; }
void irq_system_init(void) {}
void communication_chlx_init(struct fifo_t *txfifo, struct fifo_t *rxfifo,
                             uint8_t chl_type, struct instance_t *instance)
{
    (void)instance;
    sim_uart_bind(chl_type, txfifo, rxfifo);
}
extern uint16_t relay_off_chk, relay_on_chk;

void IOStateCheck(struct instance_t *instance, struct pipeline_t *pipeline)
{
    (void)instance;
    (void)pipeline;
    /* shortcut: advance one ISR tick per simulated second; Relay_Ck_Time is
     * counted 100x slower than the 10 ms SysTick, upgrade if timing matters. */
    if (sim_scenario_relay_is_off()) {
        if (relay_off_chk < Relay_Ck_Time) {
            relay_off_chk++;
        }
        relay_on_chk = 0;
    } else {
        if (relay_on_chk < Relay_Ck_Time) {
            relay_on_chk++;
        }
        relay_off_chk = 0;
    }
}
void rs485_init(struct instance_t *instance, struct pipeline_t *pipeline)
{
    (void)instance;
    (void)pipeline;
}
void start_emu_signal_pro(void) {}
unsigned char WriteRn8209(unsigned char chip_addr, unsigned char *buf,
                          unsigned char length, const struct dev_info_t *dev)
{
    (void)chip_addr;
    (void)buf;
    (void)length;
    (void)dev;
    return 0;
}
void gpio_configuration_AC(void) {}
void fnINTTC3_Init(uint8_t cmd) { (void)cmd; }
uint8_t Magnetic_hold_relay_Init(void) { return 0; }

uint8_t DEVICE_ERR = 0;
struct Rn8209AdjK_t Rn8209Adj_K;

uint8_t ANA_GetStatus(uint32_t status_mask) { (void)status_mask; return 0; }
void CLK_AHBPeriphralCmd(uint32_t peripheral, uint32_t state)
{
    (void)peripheral;
    (void)state;
}
unsigned char CmdRn8209(unsigned char cmd, const struct dev_info_t *dev)
{
    (void)cmd;
    (void)dev;
    return 0;
}
int nd_board_cover_closed(void) { return sim_scenario_cover_closed(); }
void nd_board_cover_input_enable(void) {}
int nd_board_key_released(void) { return sim_scenario_key_released(); }
int openM_ioctl(int fd, int request, void *param)
{
    (void)fd;
    (void)request;
    (void)param;
    return -1;
}
void report_port_open(struct instance_t *instance) { (void)instance; }
void reset_all_channel_baud(struct instance_t *instance) { (void)instance; }
uint8_t write_mcu_rtc_date(uint8_t addr, uint8_t *buf, uint16_t len)
{
    (void)addr;
    (void)buf;
    (void)len;
    return 0;
}
uint8_t write_mcu_rtc_time(uint8_t addr, uint8_t *buf, uint16_t len)
{
    (void)addr;
    (void)buf;
    (void)len;
    return 0;
}
void v85xxp_clear_watchdog(void) {}

uint8_t Magnetic_hold_relay_connect(void)
{
    sim_log_relay("connect");
    return 0;
}
uint8_t Magnetic_hold_relay_disconnect(void)
{
    sim_log_relay("disconnect");
    return 0;
}
void dev_init(struct instance_t *instance, uint8_t power_mode)
{
    (void)instance;
    (void)power_mode;
}
void lcd_init(uint8_t power_mode) { (void)power_mode; }
void nd_board_backlight_set(int on) { sim_log_backlight(on); }
void nd_board_plc_event_set(int on) { (void)on; }
void nd_board_relay_led_set(int on) { (void)on; }
void nd_board_relay_feedback_input_enable(void) {}
void nd_board_key_input_enable(void) {}
int nd_board_key_pressed(void) { return sim_scenario_key_pressed(); }
int nd_board_relay_is_off(void) { return sim_scenario_relay_is_off(); }
