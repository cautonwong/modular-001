#ifndef ND_TARGET_STATEGRID_DRIVER_ENTRY_H
#define ND_TARGET_STATEGRID_DRIVER_ENTRY_H

#include "targets/stategrid_meter/app/stategrid_context.h"

#include <stdint.h>
struct fifo_t;

void dev_init(stategrid_context_t *instance, uint8_t power_mode);
void irq_system_init(void);
void irq_transform_to_instance(stategrid_context_t *instance);
void communication_chlx_init(struct fifo_t *txfifo, struct fifo_t *rxfifo,
                             uint8_t chl_type, stategrid_context_t *instance);
void IOStateCheck(stategrid_context_t *instance, void *pipeline);
void timing_ftr(void *param);
void IO_Interrupt(void *param);
void rs485_init(stategrid_context_t *instance, void *pipeline);
void device_power_close(stategrid_context_t *instance);
void device_power_close1(stategrid_context_t *instance);
void device_power_open(stategrid_context_t *instance, void *pipeline);
void cpu_sleep_manage(stategrid_context_t *instance, void *pipeline);
extern const uint32_t baud_table[11];

extern const uint8_t init_Meter_Type[];

#endif
