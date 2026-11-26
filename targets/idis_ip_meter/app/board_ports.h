#ifndef ND_IDIS_APP_BOARD_PORTS_H
#define ND_IDIS_APP_BOARD_PORTS_H

#include "components/protocol/dlms/include/nd_axdr.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter/board/board_state.h"
#include "targets/idis_ip_meter/cosem/nd_idis_calibration.h"
#include "targets/idis_ip_meter/cosem/nd_measurement_port.h"

void board_bind(nd_idis_board_state_t *state);
nd_store_t *board_store_port(nd_idis_board_state_t *state);
nd_store_t *board_flash_port(nd_idis_board_state_t *state);
void board_store_init(nd_idis_board_state_t *state);
void board_flash_init(nd_idis_board_state_t *state);
void board_uart_init(nd_idis_board_state_t *state);
/* Returns 1 when a byte arrived on the local optical port, 0 when idle. */
int board_ir_read(uint8_t *byte);
void board_gpio_init(nd_idis_board_state_t *state);
int board_key_event_take(nd_idis_board_state_t *state);
int board_rtc_read(nd_dlms_datetime_t *now);

typedef struct {
    int32_t voltage;
    int32_t current;
    int32_t power;
} board_measurement_calibration_t;

typedef struct {
    board_measurement_calibration_t *calibration;
    nd_idis_board_state_t *state;
} board_measurement_ctx_t;

/* Returns the factory calibration result so the composition root can raise
 * the NV memory fault event. Scaling is usable for every status; the compiled
 * defaults stay active unless the record loaded or was adopted. */
nd_idis_calibration_status_t
board_measurement_init(nd_idis_board_state_t *state,
                       board_measurement_calibration_t *calibration);
int board_measurement_read(void *ctx, nd_measure_quantity_t quantity,
                           uint8_t phase, nd_measurement_value_t *out);
int board_adc_read(uint8_t channel, int32_t *value);
void board_relay_apply(int connected);

#endif
