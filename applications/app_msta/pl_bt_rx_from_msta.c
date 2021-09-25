#include "nidavellir.h"
extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_enable_tx(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);

int fr_bt_rx_from_msta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct cache_t* cache = instance->cache2;
    int r = 0;
    //todo 第一层合法性判断 识别通道
    fifoBuf_removeData(pipeline->in_queue, 10);//剥掉外层的头
    ctx_msta_proxy_to_bt.step = 0;
    ctx_msta_proxy_to_bt.cs = 0;
    ctx_msta_proxy_to_bt.min_len = 12;
    pipeline->ctx = &ctx_proxy;
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    ctx->min_len = 0;
    if (fr_rx_645_entry(instance, pipeline, mesg) == 0)
    {
        //内层报文格式不对
        pipeline->ctx = &ctx_bt_rx_from_msta;
        return 0;
    }

    //todo 合法性判断
    //todo 判断ID 看看是不是要转发proxy
    fifoBuf_removeData(fifo, 10);//将内层的数据域前的字节清掉

    switch (ctx->ctrl)
    {
    case 0x91:
        r = fr_645_decode_91(instance, pipeline, mesg);
        break;
    default:
        //todo  不认识的报文  转发给基表
        break;
    }

    pipeline->ctx = &ctx_bt_rx_from_msta;
    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);
    return 0;
}

struct filter_t fr_bt_rx_from_msta[] =
{
    {0x00,fr_rx_645_entry,1,13},
    {0x01,fr_bt_rx_from_msta_gatekeeper,0x00,13},
};

struct pipeline_t pl_bt_rx_from_msta = 
{
    1,
    "bt_rx_from_msta",
    &ctx_bt_rx_from_msta,
    &pipe_msta_tx_to_sta,
    NULL,
    sizeof(fr_bt_rx_from_msta)/sizeof(fr_bt_rx_from_msta[0]),
    fr_bt_rx_from_msta,
};
