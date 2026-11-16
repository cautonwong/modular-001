#ifndef ND_INFRA_OR_SOC_V85XXP_SYSTEM_H
#define ND_INFRA_OR_SOC_V85XXP_SYSTEM_H

#include <stdint.h>

struct bus_ops_t;

void lcd_init(uint8_t power_mode);
void MComp1Init(void);
void v85xxp_clear_watchdog(void);
void fnINTTC3_Init(uint8_t cmd);
void start_emu_signal_pro(void);
uint8_t write_mcu_rtc_time(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t write_mcu_rtc_date(uint8_t addr, uint8_t *buf, uint16_t len);

extern const struct bus_ops_t mcu_uart_bus_ops;

#endif
