#ifndef ND_IDIS_APP_BOARD_PORTS_H
#define ND_IDIS_APP_BOARD_PORTS_H

#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/nd_measurement_port.h"

nd_store_t *board_store_port(void);
nd_store_t *board_flash_port(void);
void board_store_init(void);
void board_flash_init(void);
void board_uart_init(void);
void board_gpio_init(void);
int board_key_event_take(void);
void board_rtc_init(void);

void board_measurement_init(void);
int board_measurement_read(void *ctx, nd_measure_quantity_t quantity,
                           uint8_t phase, nd_measurement_value_t *out);
int board_adc_read(uint8_t channel, int32_t *value);
void board_relay_apply(int connected);

#endif
