#include "infra/boards/stategrid_single_phase/meter_status.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "domain/common/status.h"
#include "nidavellir.h"
#include "kernel/legacy_control_block.h"
#include <stddef.h>

static struct mbase_control_block_t *mb(struct instance_t *instance)
{
    return (struct mbase_control_block_t *)get_pipeline_ctx(instance,
                                                             pid_meter_base);
}

uint16_t nd_meter_status_metertype(void *ctx)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_metertype(block) : 0;
}

void nd_meter_status_metertype_set(void *ctx, uint16_t value)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_metertype(block) = value;
    }
}

void nd_meter_status_emu_error_set(void *ctx, int is_error)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_EmuErrS_status(block) = is_error ? ND_EVENT_SET : ND_EVENT_RESET;
    }
}

uint8_t nd_meter_status_esam_power(void *ctx)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_esam_power_status(block) : 0;
}

void nd_meter_status_esam_power_set(void *ctx, uint8_t on,
                                    uint16_t close_delay)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_esam_power_status(block) = on;
        nd_mbase_esam_power_close_dly(block) = (uint8_t)close_delay;
    }
}

uint16_t nd_meter_status_esam_close_delay(void *ctx)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_esam_power_close_dly(block) : 0;
}

void nd_meter_status_esam_close_delay_set(void *ctx,
                                          uint16_t close_delay)
{
    struct instance_t *instance = (struct instance_t *)ctx;
    struct mbase_control_block_t *block = mb(instance);

    if (block != NULL)
    {
        nd_mbase_esam_power_close_dly(block) = (uint8_t)close_delay;
    }
}
