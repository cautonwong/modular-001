#include "nidavellir.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_07(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int __update_cs(void* ctx, uint8_t* byte);
extern int __update_scrambler_and_cs(void* pctx, uint8_t* byte);

int fr_msta_tx_data_to_bt(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)&ctx_msta_proxy_to_bt;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    struct cache_t* cache = instance->cache2;
		cache->used = 0;
    ctx->cs = 0;
    append_byte_to_cache(cache, 0x68, __update_cs, (void*)ctx);
    append_data_to_cache(cache, ctx->dst_addr, 6, __update_cs, (void*)ctx);
    append_byte_to_cache(cache, 0x68, __update_cs, (void*)ctx);
    uint8_t len = 0;
    uint8_t* p = cache->buf + 10;
    uint32_to_buf_lsb(mesg->id, p);
    len = 4;

    if (mesg->len == 0x04)
    {
        r = __read(instance,mesg->id, p + len, 4);
        len += r;
    }
    else
    {
        fifoBuf_getData(fifo, p + len, mesg->len - 4);
        len += mesg->len - 4;
    }

//    append_byte_to_cache(cache, 0x91, __update_cs, ctx);
    append_byte_to_cache(cache, mesg->ctrl, __update_cs, ctx);
    append_byte_to_cache(cache, len, __update_cs, ctx);
    append_data_to_cache(cache, p, len, __update_scrambler_and_cs, ctx);
    append_byte_to_cache(cache, ctx->cs, __update_cs, ctx);
    append_byte_to_cache(cache, 0x16, __update_cs, ctx);
    return 1;
}

int fr_msta_tx_to_bt_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;
    uint8_t byte;
    struct cache_t* cache = instance->cache2;
    struct fifo_t* fifo = pipeline->out_queue;
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;

    fr_msta_tx_data_to_bt(instance, pipeline, mesg);
   
    pipeline->ctx = &ctx_bt_rx_from_msta;
    ctx = pipeline->ctx;
    fr_pack_645_head(instance, pipeline, mesg);
    append_byte(fifo, 0x11, __update_cs, ctx);
    append_byte(fifo, cache->used, __update_cs, ctx);
    append_data(fifo, instance->cache2->buf,
        cache->used, __update_scrambler_and_cs, ctx);
    fr_pack_645_tail(instance, pipeline, mesg);
	  uart_ctrl_enable(pipeline->id);
    return 0;
}
struct filter_t fr_msta_tx_to_bt[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_msta_tx_to_bt_gatekeeper,3,13},
};

//msta接收到来自meter的报文处理流水线
struct pipeline_t pl_msta_tx_to_bt = 
{
    2,
    "msta_tx_to_bt",
    NULL,
    &pipe_msta_tx_to_bt_req,
    NULL,
    sizeof(fr_msta_tx_to_bt)/sizeof(fr_msta_tx_to_bt[0]),
    fr_msta_tx_to_bt,
};