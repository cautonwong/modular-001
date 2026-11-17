#ifndef ND_METER_STATUS_H
#define ND_METER_STATUS_H

#include <stdint.h>

/*
 * Board-facing view of the meter status that lives in the legacy control
 * block. Infra consumers declare what they read and write; the target
 * composition root implements it over the current block layout.
 */
uint16_t nd_meter_status_metertype(void *ctx);
void nd_meter_status_metertype_set(void *ctx, uint16_t value);
void nd_meter_status_emu_error_set(void *ctx, int is_error);
uint8_t nd_meter_status_esam_power(void *ctx);
void nd_meter_status_esam_power_set(void *ctx, uint8_t on,
                                    uint16_t close_delay);
uint16_t nd_meter_status_esam_close_delay(void *ctx);
void nd_meter_status_esam_close_delay_set(void *ctx,
                                          uint16_t close_delay);

#endif
