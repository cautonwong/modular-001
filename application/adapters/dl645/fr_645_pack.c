#include "application/adapters/dl645/fr_645_pack.h"
#include <string.h>
#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_env.h"
#include "application/ports/fifo.h"

static uint16_t fr645_fifo_used(const fr_645_fifo_t *fifo)
{
    uint16_t rd = fifo->rd;
    uint16_t wr = fifo->wr;
    uint16_t size = fifo->buf_size;

    return wr < rd ? (uint16_t)((size - rd) + wr) : (uint16_t)(wr - rd);
}

static uint16_t fr645_fifo_pop_data(fr_645_fifo_t *fifo, uint16_t len,
                                    int (*callback)(void *, uint8_t *),
                                    void *ctx)
{
    uint16_t rd = fifo->rd;
    uint16_t left;
    uint16_t copied = 0;

    if (fr645_fifo_used(fifo) < len) {
        return 0;
    }
    left = len;
    while (left > 0) {
        if (callback != NULL) {
            callback(ctx, fifo->buf_ptr + rd);
        }
        copied++;
        left--;
        if (++rd >= fifo->buf_size) {
            rd = 0;
        }
    }
    fifo->rd = rd;
    return copied;
}

int __update_cs(void* pctx, uint8_t* byte)
{
    struct context_645_t* ctx = pctx;
    ctx->tx_len++;
    ctx->cs += *byte;
    return 0;
}
int __update_scrambler_and_cs(void* pctx, uint8_t* byte)
{
    struct context_645_t* ctx = pctx;
    ctx->tx_len++;
    *byte += 0x33;
    ctx->cs += *byte;
    return 0;
}
int __pack_apdu_byte_645(void* pctx, uint8_t* byte)
{
    struct pipeline_t* pipeline = pctx;
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_out_queue(pipeline);

    append_byte(fifo, *byte, __update_scrambler_and_cs, ctx);
    return 0;
}
int fr_pack_645_apdu(struct instance_t*, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    fr645_fifo_pop_data(&ctx->apdu, mesg->len, __pack_apdu_byte_645, pipeline);
    return 1;
}
int fr_pack_645_head(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_out_queue(pipeline);
    ctx->cs = 0;
    ctx->tx_len = 0;
    uint8_t buf[6];


    for(int i = 0; i < 4; i ++){
        fifoBuf_putByte(fifo, 0xfe);
    }
    append_byte(fifo, 0x68, __update_cs, ctx);
    //todo ��ַת��
    if (mesg->ctrl == 0x08)
    {
        memset(buf, 0x99, 6);
    }
    else if (mesg->ctrl == 0x13)
    {
        memset(buf, 0xAA, 6);
    }
    else
    {
        memcpy(buf, nd_fr645_tsa(instance), 6);
        //memcpy(buf, &msta_context.addr, 6);
    }
    append_data(fifo, buf, 6, __update_cs, ctx);
    append_byte(fifo, 0x68, __update_cs, ctx);
    return 1;
}
int fr_pack_645_data(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_out_queue(pipeline);
    uint8_t len = mesg->len;

    if ((ctx->segment_tx) && (ctx->segment_tx_no != 0)) //后续帧情况下 发帧序号
    {
        len = mesg->len + 1;
    }


    append_byte(fifo, mesg->ctrl, __update_cs, ctx);
    append_byte(fifo, len, __update_cs, ctx);

    if(mesg->len)
        fr_pack_645_apdu(instance, pipeline, mesg);

    //append_data(fifo, ctx->apdu_tx, mesg->len, __update_scrambler_and_cs, ctx);

    if ((ctx->segment_tx)&&(ctx->segment_tx_no != 0)) //后续帧情况下 发帧序号
    {
        append_data(fifo, &ctx->segment_tx_no, 1, __update_scrambler_and_cs, ctx);
    }

    return 1;
}
int fr_pack_645_tail(struct instance_t*, struct pipeline_t* pipeline, nd_dl645_msg_t*)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_out_queue(pipeline);
    fifoBuf_putByte(fifo, ctx->cs);
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len += 2;
    return 1;
}
