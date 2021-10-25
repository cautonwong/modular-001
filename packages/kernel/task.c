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

int run_task(struct instance_t *instance,struct message_t *mesg)
{
    return 0;
}

int set_task_context(struct instance_t* instance, struct task_t *task,void *ctx)
{
    task->control_block = ctx;
    return 0;
}

int task_init(struct instance_t* instance,struct task_t* task)
{
    return 0;
}

int setTaskFilterChain(struct instance_t* instance,struct task_t* task,struct filter_chain_t* filter_chain)
{
    task->filter_chain = filter_chain;
}

int task_add_filter(struct task_t* task, uint32_t mesg_id,filter_t filter)
{
    assert(mesg_id < 4);
    assert(filter != NULL);
    return 0;
}

int send_mesg_v2(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data)
{
    int r = 0;
    pid_t from = kernel;
    struct  fifo_t* fifo = get_task_stdin(instance, mesg->to);
    if (!task) from = task->id;
    
    mesg->head = 0xa55a;
    mesg->from = from;
    mesg->ts = rtc_mktime(&instance->time);
    fifoBuf_putData(fifo,mesg,sizeof(struct message_t));
    fifoBuf_putData(fifo,data,mesg->len);//data
    //fifoBuf_putByte(fifo,0x5a);//tail

    return 0;
}

int __mesg_filter(struct message_t* ctx, uint8_t* b)
{
    uint8_t byte = *b;

    switch(byte)
    {
        case 0:
            if(byte != 0xa5) return -1;
            ctx->step = 1;
            return 0;
        case 1:
            ctx->step = 0;
            if(byte != 0x5a) return -1;
            return 1;
    }

    assert(0);
    return -1;
}

int recv_mesg_v2(struct instance_t* instance, struct task_t* task, struct message_t* mesg)
{
    struct fifo_t* std_in = (struct fifo_t *)task->std_in;
    int r = ringbuffer_filter(std_in, __mesg_filter, mesg);
    if(r > 0)
    {
        r = fifoBuf_getData(std_in, mesg, sizeof(struct message_t));
        if(mesg->head == 0xa55a)
        {
            return mesg->len;
        }
    }

    return 0;
}

int send_kernel_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data)
{
    mesg->mesg_id = 0x00;
    return send_mesg_v2(instance,task,mesg,data);
}

int send_timeout_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data)
{
    mesg->mesg_id = 0x01;
    return send_mesg_v2(instance,task,mesg,data);
}

int send_command_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data)
{
    mesg->mesg_id = 0x02;
    return send_mesg_v2(instance,task,mesg,data);
}

int send_query_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data)
{
    mesg->mesg_id = 0x03;
    return send_mesg_v2(instance,task,mesg,data);
}
