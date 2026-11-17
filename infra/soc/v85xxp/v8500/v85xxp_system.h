#ifndef ND_INFRA_OR_SOC_V85XXP_SYSTEM_H
#define ND_INFRA_OR_SOC_V85XXP_SYSTEM_H

#include <stddef.h>
#include <stdint.h>

void lcd_init(uint8_t power_mode);
void MComp1Init(void);
void v85xxp_clear_watchdog(void);
void fnINTTC3_Init(uint8_t cmd);
void start_emu_signal_pro(void);
uint8_t write_mcu_rtc_time(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t write_mcu_rtc_date(uint8_t addr, uint8_t *buf, uint16_t len);
void v85xxp_spi_ctrl(uint8_t ctrl, size_t index);

#endif
