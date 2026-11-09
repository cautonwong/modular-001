#include "nidavellir.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_14(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg);

int __fr_pre_pack_07(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    struct fifo_t *fifo = (struct fifo_t *)pipeline->out_queue;
    fifoBuf_putByte(fifo, 0x07);
    fifoBuf_putByte(fifo, 0x04);//长度
    struct context_645_t *ctx = (struct context_645_t *)pipeline->ctx;
    ctx->tx_len += 2;
    pipeline->ctx = &ctx_proxy;
    return 1;
};
int __fr_pack_tx_645_head_07(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;

    ctx->tx_len = 0;
    ctx->cs = 0x68;
    fifoBuf_putByte(fifo, 0x04);//SA

    fifoBuf_putByte(fifo, 0x68);
    //todo 地址转换  帧序号处理
    fifoBuf_putData(fifo, ctx->dst_addr, 6);
    for (uint8_t i = 0; i <= 6; i++)
    {
        ctx->cs += ctx->dst_addr[i];
    }
    fifoBuf_putByte(fifo, 0x68);
    ctx->cs += 0x68;
    ctx->tx_len += 9;
    return 1;
}

int __fr_encode_645_07_data(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    //todo  要调试
    struct context_645_t *ctx = (struct context_645_t *)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;
    //struct ipc_object_t *ipc_object = container_of(object,struct ipc_object_t,object);
    fifoBuf_putByte(fifo, 0x11);//ipc_object->ctrl);
    ctx->cs += 0x11;//ipc_object->ctrl;//0x11
    fifoBuf_putByte(fifo, 0x04);
    ctx->cs += 0x04;
    //fifoBuf_putData(fifo, &ipc_object->id, 4);
    //ctx->cs += ipc_object->id;
    //ctx->cs += (ipc_object->id>>8);
    //ctx->cs += (ipc_object->id>>16);
    //ctx->cs += (ipc_object->id>>24);
    //ctx->tx_len += 6;
    return 1;
}
int __fr_change_bt_pl(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint8_t cs = ctx->cs;
    pipeline->ctx = &ctx_sta_rx_from_msta;
    ctx = (struct context_645_t*)pipeline->ctx;
    ctx->cs += cs + 0x10;
    uint8_t buf[200];
    cs = 0;
    uint16_t n = fifoBuf_getDataPeek(fifo, buf, 200);
    for (size_t i = 0; i < n; i++)
    {
        cs += buf[i];
    }
    ctx->cs = cs;
    return 1;
};
int fr_encode_645_07(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    __fr_pre_pack_07(instance,pipeline, mesg);
    __fr_pack_tx_645_head_07(instance,pipeline, mesg);
    __fr_encode_645_07_data(instance,pipeline, mesg);
    fr_pack_645_tail(instance,pipeline, mesg);
    //fr_change_bt_len(instance,pipeline, mesg);
    __fr_change_bt_pl(instance,pipeline, mesg);
    return 1;
}
int fr_sta_tx_to_msta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;

    switch (mesg->ctrl)
    {
    case 0x11:
        r = fr_encode_645_11(instance, pipeline, mesg);
        break;
    case 0x08:
        r = fr_encode_645_08(instance, pipeline, mesg);
        break;
    case 0x14:
        r = fr_encode_645_14(instance, pipeline, mesg);
        break;
    case 0x07:
        r = fr_encode_645_14(instance, pipeline, mesg);
        break;
    default:
        //todo  不认识的报文  
        break;
    }

    if (r == 1)
    {
        fr_tx_645_end(instance, pipeline, mesg);
    }

    return 0;
}


struct filter_t fr_sta_tx_to_msta[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_sta_tx_to_msta_gatekeeper,3,13},
};

struct pipeline_t pl_sta_tx_to_msta =
{
    2,
    "sta_tx_to_msta",
    NULL,
    &pipe_sta_tx_to_msta_req,
    &pipe_msta_rx_from_sta,
    sizeof(fr_sta_tx_to_msta) / sizeof(fr_sta_tx_to_msta[0]),
    fr_sta_tx_to_msta,
};

