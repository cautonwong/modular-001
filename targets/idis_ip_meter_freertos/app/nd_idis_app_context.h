#ifndef ND_IDIS_FREERTOS_APP_CONTEXT_H
#define ND_IDIS_FREERTOS_APP_CONTEXT_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "components/services/maintenance/nd_maintenance.h"
#include "infra/drivers/storage/block/block_ops.h"
#include "infra/storage/pulse_store.h"
#include "targets/idis_ip_meter_freertos/board/board_state.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_clock_port.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"

#include "board_ports.h"

typedef struct {
    nd_cosem_port_t port;
    nd_clock_port_t clock_port;
    nd_measurement_port_t measurement;
    board_measurement_calibration_t measurement_calibration;
    nd_maintenance_t maintenance;
    nd_pulse_store_storage_t energy_pulse_storage;
    nd_eeprom_block_t energy_eeprom_block;
    nd_flash_block_t energy_flash_block;
    /* cm0_systick_init() programs a 10 ms SysTick, and ndv_timer_tick() uses
     * that same unit. Keep the second counter separate from the timer ticks. */
    uint64_t energy_pulse_fraction;
    int energy_commit_pending;
    int energy_appends_pending;
} nd_idis_composition_t;

/* Board-owned hardware buffers stay in board_ports.c. */
typedef struct {
    nd_idis_objects_t objects;
    nd_idis_composition_t composition;
    uint32_t last_save;
    int power_failed;
    int cover_open;
    int key_down;
    int backlight;
    int relay_connected;
    int relay_feedback_pending;
    unsigned relay_feedback_seconds;
    int battery_low;
    uint32_t app_last_second;
    TaskHandle_t app_task_handle;
    TaskHandle_t worker_task_handle;
    QueueHandle_t save_queue;
    SemaphoreHandle_t save_done;
    volatile uint8_t critical_pending;
    nd_idis_board_state_t board;
    board_measurement_ctx_t measurement_ctx;
} nd_idis_app_context_t;

nd_idis_app_context_t *nd_idis_app_root(void);

#endif
