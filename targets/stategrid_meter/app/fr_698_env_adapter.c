#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "application/ports/pipeline_view.h"

struct tm *nd_fr698_time(void *instance)
{
    return instance != NULL ? &((stategrid_context_t *)instance)->time : 0;
}

uint8_t *nd_fr698_apdu_cache(void *instance)
{
    return instance != NULL ? ((stategrid_context_t *)instance)->apdu_cache : 0;
}

uint8_t *nd_fr698_power_state(void *instance)
{
    return instance != NULL ? &((stategrid_context_t *)instance)->ac_power_status : 0;
}

uint8_t *nd_fr698_reset_flag(void *instance)
{
    stategrid_context_t *ctx = instance != NULL ? ((stategrid_context_t *)instance) : 0;

    return ctx != NULL ? &ctx->reset_flag : 0;
}

uint32_t *nd_fr698_ac_power_off_sec_cnt(void *instance)
{
    stategrid_context_t *ctx = instance != NULL ? ((stategrid_context_t *)instance) : 0;

    return ctx != NULL ? &ctx->ac_power_off_sec_cnt : 0;
}

uint32_t nd_fr698_ms(void *instance)
{
    return instance != NULL ? ((stategrid_context_t *)instance)->ms : 0;
}

void *nd_fr698_plc_ctx(void *instance)
{
    return instance != NULL ? ((stategrid_context_t *)instance)->plc_698_ctx : 0;
}

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    const nd_pipeline_view_t *p = pipeline;

    return p != NULL ? p->ctx : NULL;
}

uint32_t nd_fr698_pipeline_id(const void *pipeline)
{
    const nd_pipeline_view_t *p = pipeline;

    return p != NULL ? p->id : 0;
}

void *nd_fr698_pipeline_in_queue(const void *pipeline)
{
    const nd_pipeline_view_t *p = pipeline;

    return p != NULL ? p->in_queue : NULL;
}

void *nd_fr698_pipeline_out_queue(const void *pipeline)
{
    const nd_pipeline_view_t *p = pipeline;

    return p != NULL ? p->out_queue : NULL;
}

void *nd_instance_upgrade_info(void *instance)
{
    return instance != NULL ? &((stategrid_context_t *)instance)->upgrade_info : 0;
}
