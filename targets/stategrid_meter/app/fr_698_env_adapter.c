#include "application/adapters/dl698/fr_698_env.h"
#include "nidavellir.h"

struct tm *nd_fr698_time(struct instance_t *instance)
{
    return instance != NULL ? &instance->time : 0;
}

uint8_t *nd_fr698_apdu_cache(struct instance_t *instance)
{
    return instance != NULL ? instance->apdu_cache : 0;
}

uint8_t *nd_fr698_power_state(struct instance_t *instance)
{
    return instance != NULL ? &instance->ac_power_statue : 0;
}

uint8_t *nd_fr698_reset_flag(struct instance_t *instance)
{
    return instance != NULL ? &instance->reset_flag : 0;
}

uint32_t *nd_fr698_ac_power_off_sec_cnt(struct instance_t *instance)
{
    return instance != NULL ? &instance->ac_power_off_sec_cnt : 0;
}

uint32_t nd_fr698_ms(struct instance_t *instance)
{
    return instance != NULL ? instance->ms : 0;
}

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->ctx : NULL;
}

uint32_t nd_fr698_pipeline_id(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->id : 0;
}

void *nd_fr698_pipeline_in_queue(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->in_queue : NULL;
}

void *nd_fr698_pipeline_out_queue(const void *pipeline)
{
    const struct pipeline_t *p = pipeline;

    return p != NULL ? p->out_queue : NULL;
}
