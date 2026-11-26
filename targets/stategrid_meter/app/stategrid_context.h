#ifndef ND_TARGET_STATEGRID_CONTEXT_H
#define ND_TARGET_STATEGRID_CONTEXT_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "application/adapters/dl698/fr_698.h"
#include "application/adapters/dl698/parameter_port.h"
#include "application/ports/meter_base.h"
#include "application/ports/platform.h"
#include "application/ports/record_storage.h"
#include "application/ports/resource_access.h"
#include "components/core/fifo/fifo.h"
#include "components/core/types/list.h"
#include "domain/common/timer.h"
#include "targets/stategrid_meter/app/display/backlight.h"
#include "domain/load_control/relay_status.h"
#include "targets/stategrid_meter/app/relay_control_ui.h"
#include "domain/metering/metering_input.h"
#include "infra/drivers/storage/block/block_ops.h"
#include "infra/memory/arena.h"
#include "infra/storage/pulse_snapshot.h"
#include "targets/stategrid_meter/board/board_state.h"
#include "targets/stategrid_meter/upgrade/upgrade_info.h"

struct mbase_control_block_t;
struct nd_demand_adapter;

struct nd_fr698_report_state {
    uint8_t rtc_cnt;
    uint16_t report_flag;
};

/* The stategrid_meter composition context: the resource lists, the protocol
 * channel state and the persistent cache pointers used by the product. */
typedef struct stategrid_context_t
{
    struct list_head dev_list;
    struct list_head map_list;
    uint8_t ram_device;
    uint8_t resource_block;
    uint8_t *apdu_cache;
    struct tm time;
    struct upgrade_info_t upgrade_info;
    struct fifo_t pipe_rtc;
    struct fifo_t pipe_key;
    struct fifo_t pipe_uart[4];
    uint8_t ac_power_status;
    uint8_t reset_flag;
    uint8_t need_data_recovery;
    uint32_t ac_power_off_sec_cnt;
    uint32_t ms;
    struct mbase_control_block_t *mbase;
    uint8_t demand_arena_buffer[256];
    nd_arena_t demand_arena;
    struct nd_demand_adapter *demand;
    /* Energy snapshot: the block adapter and the store storage live in the
     * instance, not in file-scope globals. legacy_energy_offset is the
     * measured 172-byte power-off record; the flag and EC2 indices are the
     * other sources the first-boot adoption mirrors from Check_EC_recover. */
    nd_eeprom_block_t energy_block;
    nd_pulse_snapshot_storage_t energy_store_storage;
    nd_pulse_store_t *energy_store;
    uint32_t legacy_energy_offset;
    uint32_t legacy_energy_flag_offset;
    uint32_t legacy_energy_ec2_energy_index;
    uint32_t legacy_energy_ec2_month_index;
    uint8_t storage_fault;
    struct fifo_t *meter_events;
    struct fifo_t *lowpower_events;
    struct fifo_t *records;
    void *plc_698_ctx;
    nd_platform_port_t platform;
    nd_record_storage_port_t record_storage;
    nd_resource_port_t resource;
    nd_timer_port_t timer;
    nd_backlight_port_t backlight;
    nd_relay_status_port_t relay_status;
    nd_metering_input_port_t metering_input;
    nd_dl698_parameter_port_t parameter;
    const nd_meter_base_port_t *meter_base;
    struct nd_fr698_report_state fr698_report;
    relay_control_ui_state_t relay_ui;
    struct connect_request_t cfg_connect;
    stategrid_board_state_t *board;
} stategrid_context_t;

static inline const nd_platform_port_t *stategrid_platform_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->platform : NULL;
}

static inline const nd_record_storage_port_t *stategrid_record_storage_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->record_storage : NULL;
}

static inline const nd_resource_port_t *stategrid_resource_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->resource : NULL;
}

static inline const nd_timer_port_t *stategrid_timer_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->timer : NULL;
}

static inline const nd_backlight_port_t *stategrid_backlight_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->backlight : NULL;
}

static inline const nd_relay_status_port_t *stategrid_relay_status_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->relay_status : NULL;
}

static inline const nd_metering_input_port_t *stategrid_metering_input_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? &ctx->metering_input : NULL;
}

static inline const nd_meter_base_port_t *stategrid_meter_base_port(
    const stategrid_context_t *ctx)
{
    return ctx != NULL ? ctx->meter_base : NULL;
}

void stategrid_context_init(stategrid_context_t *ctx);
void stategrid_context_set_mbase(stategrid_context_t *ctx,
                                 struct mbase_control_block_t *mbase);

#endif
