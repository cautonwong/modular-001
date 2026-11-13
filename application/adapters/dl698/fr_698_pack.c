#include "application/adapters/dl698/fr_698_pack.h"
#include "application/ports/fifo.h"
#include "fr_698.h"
#include "fr_698_env.h"

static uint16_t pack_crc16(uint16_t init, uint8_t byte)
{
    uint16_t crc = (uint16_t)(init ^ byte);
    for (uint8_t i = 0; i < 8; i++) {
        crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0x8408u)
                         : (uint16_t)(crc >> 1);
    }
    return crc;
}

static uint16_t pack_crc_finish(uint16_t cs)
{
    return (uint16_t)(cs ^ 0xFFFFu);
}

int update_hcs_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->hcs = pack_crc16(ctx->hcs, *byte);
    ctx->fcs = pack_crc16(ctx->fcs, *byte);
    return 0;
}

int update_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->fcs = pack_crc16(ctx->fcs, *byte);
    return 0;
}

int __pack_apdu_byte(void* pctx, uint8_t* byte)
{
    struct pipeline_t* pipeline = pctx;
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_out_queue(pipeline);
    if (is_scrambler_set(ctx->ctrl)){
        *byte += 0x33;
    }
    append_byte(fifo, *byte, update_fcs, ctx);
    return 0;
}




int fr_pack_698_head(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_out_queue(pipeline);
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;

    for(int i = 0; i < 4; i ++){
        fifoBuf_putByte(fifo, 0xfe);
    }
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;

    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != FR_698_PID_RX){
        //ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = pack_crc_finish(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_tx_head(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_out_queue(pipeline);
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != FR_698_PID_RX){
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = pack_crc_finish(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_apdu(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    fifo_popData(ctx->apdu, mesg->len, __pack_apdu_byte, pipeline);
    return 1;
}

int fr_pack_698_tail(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_out_queue(pipeline);
    ctx->fcs = pack_crc_finish(ctx->fcs);
    fifoBuf_putByte(fifo, ctx->fcs & 0xff);
    ctx->tx_len++;
    fifoBuf_putByte(fifo, ((ctx->fcs >> 8) & 0xff));
    ctx->tx_len++;
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len++;
    return 1;
}


