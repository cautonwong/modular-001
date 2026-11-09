#include "nidavellir.h"
extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_86(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_rx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
int fr_sta_rx_from_msta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int r = 0;

    //todo 合法性判断
    //todo 判断ID 看看是不是要转发proxy
    if (ctx->ctrl == 0x14)
    {
        extern int __return_error();
        int r = __return_error();
        if (r < 0)//不回复
        {
            fifoBuf_removeData(fifo, ctx->min_len);
            return 0;
        }
    }
    else if (ctx->ctrl == 0x06)
    {
        fifoBuf_removeData(fifo, ctx->min_len);
        return 0;
    }
    else if (ctx->ctrl == 0x13)
    {
        extern int __return_error();
        int r = __return_error();
        if (r < 0)//不回复
        {
            fifoBuf_removeData(fifo, ctx->min_len);
            return 0;
        }
    }
    //else
    {
        fifoBuf_removeData(fifo, 10);//将数据域前的字节清掉
    }

    switch (ctx->ctrl)
    {
    case 0x91:
        r = fr_645_decode_91(instance, pipeline, mesg);
        break;
    case 0x86:
        r = fr_decode_645_86(instance, pipeline, mesg);
        break;
    default:
        //todo  不认识的报文  转发给基表
        break;
    }

    if (r == 1)
    {
        fr_rx_645_end(instance, pipeline, mesg);
    }

    return 0;
}

struct filter_t fr_sta_rx_from_msta[] =
{
    {0x00,fr_rx_645_entry,0x01,13},
    {0x01,fr_sta_rx_from_msta_gatekeeper,0x00,13},
};

struct pipeline_t pl_sta_rx_from_msta =
{
    1,
    "sta_rx_from_msta",
    &ctx_sta_rx_from_msta,
    &pipe_msta_tx_to_sta,
    NULL,
    sizeof(fr_sta_rx_from_msta) / sizeof(fr_sta_rx_from_msta[0]),
    fr_sta_rx_from_msta,
};
