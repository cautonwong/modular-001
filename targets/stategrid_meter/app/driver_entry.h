#ifndef ND_TARGET_STATEGRID_DRIVER_ENTRY_H
#define ND_TARGET_STATEGRID_DRIVER_ENTRY_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct fifo_t;

void dev_init(struct instance_t *instance, uint8_t power_mode);
void irq_system_init(void);
void irq_transform_to_instance(struct instance_t *instance);
void communication_chlx_init(struct fifo_t *txfifo, struct fifo_t *rxfifo,
                             uint8_t chl_type, struct instance_t *instance);
void IOStateCheck(struct instance_t *instance, struct pipeline_t *pipeline);

extern const uint8_t init_Meter_Type[];

#endif
