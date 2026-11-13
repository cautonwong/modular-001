#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_tx_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_change_bt_len(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int __update_cs(struct context_645_t* ctx, uint8_t* byte);
extern int __update_scrambler_and_cs(struct context_645_t* ctx, uint8_t* byte);

int fr_bt_tx_to_msta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;
    uint8_t byte;
    ctx_proxy.cs = 0;
    ctx_proxy.step = 0;
    ctx_proxy.min_len = 12;
    struct cache_t* cache = instance->cache2;
    struct fifo_t* fifo = pipeline->out_queue;
    struct context_645_t* ctx = &ctx_proxy;
    append_byte_to_cache(cache, 0x68, __update_cs, ctx);
    append_data_to_cache(cache, ctx->dst_addr, 6, __update_cs, ctx);
    append_byte_to_cache(cache, 0x68, __update_cs, ctx);
    append_byte_to_cache(cache, mesg->ctrl, __update_cs, ctx);
    append_byte_to_cache(cache, mesg->len, __update_cs, ctx);

    for (size_t i = 0; i < 4; i++)
    {
        byte = (mesg->id>>(8*i))&0xff;
        append_byte_to_cache(cache, byte, __update_scrambler_and_cs, ctx);
    }

    for (size_t i = 0; i < mesg->len-4; i++)
    {
        byte = fifoBuf_getByte(pipeline->in_queue);
        append_byte_to_cache(cache, byte, __update_scrambler_and_cs, ctx);
    }
    append_byte_to_cache(cache, ctx->cs, __update_cs, ctx);
    append_byte_to_cache(cache, 0x16, __update_cs, ctx);

    pipeline->ctx = &ctx_bt_rx_from_msta;
    ctx = pipeline->ctx;
    fr_pack_645_head(instance, pipeline, mesg);
    append_byte(fifo, 0x11, __update_cs, ctx);
    append_byte(fifo, cache->used, __update_cs, ctx);
    append_data(fifo, instance->cache2->buf,
        cache->used, __update_scrambler_and_cs, ctx);
    fr_pack_645_tail(instance, pipeline, mesg);
    return 0;
}

//蓝牙主站方
struct filter_t fr_bt_tx_to_msta[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_bt_tx_to_msta_gatekeeper,3,13},
};

struct pipeline_t pl_bt_tx_to_msta = 
{
    2,
    "bt_tx_to_msta",
    NULL,
    &pipe_sta_tx_to_msta_req,
    &pipe_msta_rx_from_sta,
    sizeof(fr_bt_tx_to_msta)/sizeof(fr_bt_tx_to_msta[0]),
    fr_bt_tx_to_msta,
};