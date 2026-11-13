#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg);
extern int fr_decode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_14(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_rx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_goto_pack_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg);
extern int fr_answer(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_msta_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int _fr_decode_645_07(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;
    fifoBuf_removeData(pipeline->in_queue, 9);
    ctx->tx_len = fifoBuf_getByte(pipeline->in_queue);
    uint8_t sa = fifoBuf_getByte(pipeline->in_queue);
    uint8_t buf[200];
    fifoBuf_getData(pipeline->in_queue, buf, ctx->tx_len - 1);
    fifoBuf_putData(&pipe_msta_tx_to_meter, buf, ctx->tx_len - 1);
    fifoBuf_getData(pipeline->in_queue, buf, 4);
    return 0;
}

int fr_msta_rx_from_sta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int r = 0;

    //todo 合法性判断
    //todo 判断ID 看看是不是要转发proxy
    if (ctx->ctrl == 0x14)
    {
//        extern int __return_error();
//        int r = __return_error();
        if (r < 0)//2????′
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
//        extern int __return_error();
//        int r = __return_error();
        if (r < 0)//2????′
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
    case 0x11:
        r = fr_decode_645_11(instance, pipeline, mesg);
        break;
    case 0x08:
        r = fr_decode_645_08(instance, pipeline, mesg);
        break;
    case 0x14:
        r = fr_decode_645_14(instance, pipeline, mesg);
        break;
    case 0x13:
        r = fr_msta_decode_645_13(instance, pipeline, mesg);
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

//entry ==> gatekeeper ==> service ==> exit
struct filter_t filters_msta_rx_from_sta[] =
{
    {0x00,fr_rx_645_entry,0x01,13},
    {0x01,fr_msta_rx_from_sta_gatekeeper,0x00,13},
};

struct pipeline_t pl_msta_rx_from_sta =
{
    1,
    "msta_rx_from_sta",
    &ctx_msta_rx_from_sta,
    &pipe_msta_rx_from_sta,
    &pipe_msta_tx_to_sta,
    sizeof(filters_msta_rx_from_sta) / sizeof(filters_msta_rx_from_sta[0]),
    filters_msta_rx_from_sta,
};
