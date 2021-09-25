#include "nidavellir.h"

extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_86(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_06(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_93_from_meter(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91_from_meter(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_rx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int fr_msta_rx_from_meter_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int r = 0;

    //todo 合法性判断
    //todo 判断ID 看看是不是要转发proxy
    fifoBuf_removeData(fifo, 10);//将数据域前的字节清掉

    switch (ctx->ctrl)
    {
    case 0x91:
        r = fr_645_decode_91_from_meter(instance, pipeline, mesg);
        break;
    case 0x93:
        r = fr_645_decode_93_from_meter(instance, pipeline, mesg);
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

//msta接收到来自meter的报文处理流水线
struct filter_t fr_msta_rx_from_meter[] =
{
    {0x00,fr_rx_645_entry,1,13},
    {0x01,fr_msta_rx_from_meter_gatekeeper,0x00,13},
};

struct pipeline_t pl_msta_rx_from_meter =
{
    1,
    "msta_rx_from_meter",
    &ctx_msta_rx_from_meter,
    &pipe_msta_rx_from_meter,
    &pipe_msta_tx_to_meter,
    sizeof(fr_msta_rx_from_meter) / sizeof(fr_msta_rx_from_meter[0]),
    fr_msta_rx_from_meter,
};
