#include "nidavellir.h"

struct filter_t *id_to_filter(struct filter_t *filters,size_t n, filter_id_t id)
{
    struct filter_t* filter = NULL;

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

struct pipeline_t* pipeline_create(struct instance_t* instance, pid_t pid, char *name,
    void (*entry)(void *ctx), void* ctx)
{
    struct pipeline_t* pipeline = malloc(sizeof(struct pipeline_t));
    pipeline->ctx = ctx;
    pipeline->id = pid;
    pipeline->name = name;
    return pipeline;
}
