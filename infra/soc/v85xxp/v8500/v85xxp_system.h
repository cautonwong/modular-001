#ifndef ND_INFRA_OR_SOC_V85XXP_SYSTEM_H
#define ND_INFRA_OR_SOC_V85XXP_SYSTEM_H

#include <stdint.h>

void lcd_init(uint8_t power_mode);
void MComp1Init(void);
void v85xxp_clear_watchdog(void);

#endif
