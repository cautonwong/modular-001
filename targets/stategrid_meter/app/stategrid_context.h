#ifndef ND_TARGET_STATEGRID_CONTEXT_H
#define ND_TARGET_STATEGRID_CONTEXT_H

#include <stdint.h>
#include <time.h>

#include "components/core/fifo/fifo.h"
#include "components/core/types/list.h"
#include "targets/stategrid_meter/upgrade/upgrade_info.h"

struct mbase_control_block_t;

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
    struct fifo_t *meter_events;
    struct fifo_t *lowpower_events;
    struct fifo_t *records;
    void *plc_698_ctx;
} stategrid_context_t;

void stategrid_context_init(stategrid_context_t *ctx);
void stategrid_context_set_mbase(stategrid_context_t *ctx,
                                 struct mbase_control_block_t *mbase);

#endif
