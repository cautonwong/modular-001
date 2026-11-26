#include "targets/stategrid_meter/board/meter_status.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/meter_wire_status.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include <stddef.h>

static struct mbase_control_block_t *mb(stategrid_context_t *instance)
{
    return ((stategrid_context_t *)instance)->mbase;
}

uint16_t nd_meter_status_metertype(void *ctx)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_metertype(block) : 0;
}

void nd_meter_status_metertype_set(void *ctx, uint16_t value)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_metertype(block) = value;
    }
}

void nd_meter_status_emu_error_set(void *ctx, int is_error)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_EmuErrS_status(block) = is_error ? ND_WIRE_STATUS_SET : ND_WIRE_STATUS_RESET;
    }
}

uint8_t nd_meter_status_esam_power(void *ctx)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_esam_power_status(block) : 0;
}

void nd_meter_status_esam_power_set(void *ctx, uint8_t on,
                                    uint16_t close_delay)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_esam_power_status(block) = on;
        nd_mbase_esam_power_close_dly(block) = (uint8_t)close_delay;
    }
}

uint16_t nd_meter_status_esam_close_delay(void *ctx)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_esam_power_close_dly(block) : 0;
}

void nd_meter_status_esam_close_delay_set(void *ctx,
                                          uint16_t close_delay)
{
    stategrid_context_t *instance = (stategrid_context_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_esam_power_close_dly(block) = (uint8_t)close_delay;
    }
}
