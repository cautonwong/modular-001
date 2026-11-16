#include "nidavellir.h"

const struct filter_t *id_to_filter(const struct filter_t *filters,size_t n, filter_id_t id)
{
    const struct filter_t* filter = NULL;

    for (size_t i = 0; i < n; i++)
    {
        filter = &filters[i];
        if (id == filter->id)
        {
            return filter;
        }
    }

    return NULL;
}

int connect_pipe(struct instance_t *instance,struct message_t *mesg)
{
    //连接到已有的pipe
    //构建matrix
    return 0;
}

int run_pipeline(struct instance_t *instance,struct message_t *mesg)
{
    return 0;
}

int set_pipeline_context(struct instance_t* instance, struct pipeline_t *pipeline,void *ctx)
{
    pipeline->ctx = ctx;
    return 0;
}
