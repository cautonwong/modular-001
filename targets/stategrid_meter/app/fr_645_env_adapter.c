#include "application/adapters/dl645/fr_645_env.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "nidavellir.h"
#include "kernel/legacy_control_block.h"
#include "kernel/resource_ids.h"
#include "infra/kernel/resource_blocks.h"
#include "port.h"

static struct mbase_control_block_t *mb(struct instance_t *instance)
{
    return (struct mbase_control_block_t *)get_pipeline_ctx(instance,
                                                             pid_meter_base);
}

int nd_fr645_mbase_valid(struct instance_t *instance)
{
    return mb(instance) != NULL;
}

uint8_t *nd_fr645_tsa(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_tsa(block) : 0;
}

uint16_t *nd_fr645_metertype(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_metertype(block) : 0;
}

uint32_t *nd_fr645_infrared_auth_timeout(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_infrared_auth_timeout(block) : 0;
}

uint8_t *nd_fr645_factor_mode(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_factor_mode(block) : 0;
}

uint8_t *nd_fr645_pass02_err_cnt(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_Pass02Err_Cnt(block) : 0;
}

uint8_t *nd_fr645_pass04_err_cnt(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_Pass04Err_Cnt(block) : 0;
}

uint16_t *nd_fr645_backlight_tmr(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_BackLightTmr(block) : 0;
}

uint8_t *nd_fr645_lcd_autodis_list_num(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_lcd_autodis_list_num(block) : 0;
}

uint16_t *nd_fr645_lcd_autodis_keep_sec(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_lcd_autodis_keep_sec(block) : 0;
}

uint8_t *nd_fr645_commdisp_ir485(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_commdisp_ir485(block) : 0;
}

uint8_t *nd_fr645_commdisp_plc(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? &nd_mbase_commdisp_plc(block) : 0;
}

uint32_t nd_fr645_password_index(struct instance_t *instance, uint8_t level)
{
    uint32_t index = ((uint32_t)RES_DEV_EEPROM << 24)
                     + ((uint32_t)mbase_block_parameter << 16);

    (void)instance;
    if (level == 0x02)
    {
        index += Meter_02Password;
    }
    else if (level == 0x04)
    {
        index += Meter_04Password;
    }
    return index;
}

uint8_t *nd_fr645_apdu_cache(struct instance_t *instance)
{
    return instance != NULL ? instance->apdu_cache : 0;
}

uint8_t *nd_fr645_handler_code(struct instance_t *instance)
{
    struct mbase_control_block_t *block = mb(instance);

    return block != NULL ? nd_mbase_handler_code(block) : 0;
}

uint32_t nd_fr645_pipeline_id(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->id : 0;
}

void *nd_fr645_pipeline_ctx(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->ctx : NULL;
}

void *nd_fr645_pipeline_in_queue(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->in_queue : NULL;
}

void *nd_fr645_pipeline_out_queue(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->out_queue : NULL;
}
