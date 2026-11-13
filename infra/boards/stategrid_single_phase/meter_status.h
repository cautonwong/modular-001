#ifndef ND_METER_STATUS_H
#define ND_METER_STATUS_H

#include <stdint.h>

struct instance_t;

/*
 * Board-facing view of the meter status that lives in the legacy control
 * block. Infra consumers declare what they read and write; the target
 * composition root implements it over the current block layout.
 */
uint16_t nd_meter_status_metertype(struct instance_t *instance);
void nd_meter_status_metertype_set(struct instance_t *instance, uint16_t value);
void nd_meter_status_emu_error_set(struct instance_t *instance, int is_error);
uint8_t nd_meter_status_esam_power(struct instance_t *instance);
void nd_meter_status_esam_power_set(struct instance_t *instance, uint8_t on,
                                    uint16_t close_delay);
uint16_t nd_meter_status_esam_close_delay(struct instance_t *instance);
void nd_meter_status_esam_close_delay_set(struct instance_t *instance,
                                         uint16_t close_delay);

#endif
