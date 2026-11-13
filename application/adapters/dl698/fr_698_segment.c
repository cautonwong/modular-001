#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "meter_base_view.h"
#include "fr_698_internal.h"

int __get_apdu_data_len(struct context_698_t* ctx){
    return ctx->min_len - 4 - (SA_LEN(ctx->SA) + 1) - 5;
}

#define ND_698_RX_FRAMES 6

static void fr_rx_block_reset(struct context_698_t* ctx)
{
    nd_block_init(&ctx->rx_block);
    ctx->rx_frame_count = 0;
    ctx->apdu_rx_len = 0;
}

static int fr_rx_block_append(struct context_698_t* ctx, uint16_t offset,
                              uint16_t length)
{
    nd_frame_t* frame;

    if (ctx->rx_frame_count >= ND_698_RX_FRAMES) {
        return -1;
    }
    if (offset != ctx->rx_block.total_len) {
        return -1;
    }

    frame = &ctx->rx_frames[ctx->rx_frame_count++];
    frame->data = ctx->apdu_d_rx + offset;
    frame->len = length;
    frame->next = NULL;
    frame->owner = NULL;
    frame->release = NULL;
    if (nd_block_append(&ctx->rx_block, frame) != 0) {
        ctx->rx_frame_count--;
        return -1;
    }
    ctx->apdu_rx_len = (uint16_t)ctx->rx_block.total_len;
    return 0;
}

static void fr_rx_block_truncate(struct context_698_t* ctx, uint16_t length)
{
    nd_frame_t* frame;
    uint8_t count = 0;

    if (nd_block_truncate(&ctx->rx_block, length) != 0) {
        fr_rx_block_reset(ctx);
        return;
    }
    for (frame = ctx->rx_block.first; frame != NULL; frame = frame->next) {
        count++;
    }
    ctx->rx_frame_count = count;
    ctx->apdu_rx_len = (uint16_t)ctx->rx_block.total_len;
}

int fr_segments_handle(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t data_len = 0;
    uint16_t s = 0;
    uint32_t offset=0;
    
    if(ctx->segment_rx == 0xffff){
        ctx->segment_rx = fifoBuf_getByte(fifo);
        ctx->segment_rx += (fifoBuf_getByte(fifo) << 8);
    }
    
    s = ctx->segment_rx;
    if(is_first_segment(s)|| is_middle_segment(s) || is_last_segment(s)){
        data_len = __get_apdu_data_len(ctx) - 2;
        
        if(is_first_segment(s)){
            fr_rx_block_reset(ctx);
            ctx->segment_no = 1;
            fifoBuf_getData(fifo, apdu_rx, data_len);
            if (fr_rx_block_append(ctx, 0, data_len) != 0) {
                fr_rx_block_reset(ctx);
                return -1;
            }
            log_info("segments0: first.\n");
        }else if((get_segment_no(s) == ctx->segment_no) || (get_segment_no(s) == ctx->last_segment)){
            if((is_middle_segment(s) || is_last_segment(s))){
                if(get_segment_no(s) == ctx->last_segment){
                    ctx->segment_no = ctx->last_segment;
                    fr_rx_block_truncate(ctx, ctx->apdu_l_len);
                    log_info("segments1: %d.\n", ctx->segment_no);
                }
                if(get_segment_no(s) == ctx->segment_no){
                    ctx->last_segment = ctx->segment_no;
                    ctx->apdu_l_len = ctx->apdu_rx_len;
                    log_info("segments2: %d.\n", ctx->last_segment);
                }
                log_info("segments3: %d.\n", ctx->segment_no);
                ctx->segment_no++;
                if(is_last_segment(s)){
                    ctx->segment_no = 0;
                }
                offset =  ctx->rx_block.total_len;
                if(offset + data_len > Max698_apdu_rx){
                    log_info("segments4: %d.\n", ctx->segment_no);
                    fifoBuf_update_rd(fifo, data_len, 1);
                    fr_rx_block_reset(ctx);
                    return -1;
                }           
                fifoBuf_getData(fifo, apdu_rx + offset,  data_len);
                if (fr_rx_block_append(ctx, (uint16_t)offset, data_len) != 0) {
                    fr_rx_block_reset(ctx);
                    return -1;
                }
            }
        }else{
            fifoBuf_getData(fifo, apdu_rx, data_len);
            fr_rx_block_reset(ctx);
            ctx->segment_no = 0;
            log_info("segments5: %d.\n", ctx->segment_no);
        }
        
        if (is_last_segment(ctx->segment_rx)) {
            clr_segment(ctx->ctrl);
            log_info("segments6: %d.\n", ctx->segment_no);
            return 1;
        }else{
            ctx->segment_rx = ctx->segment_rx & 0x3fff;
            set_confirm_segment(ctx->segment_rx);
            fifoBuf_putByte(ctx->apdu, ctx->segment_rx & 0x00ff);
            fifoBuf_putByte(ctx->apdu, (ctx->segment_rx >> 8));
            mesg->len = fifoBuf_getUsed(ctx->apdu);
            mesg->from  = nd_fr698_pipeline_id(pipeline);
            mesg->to    = FR_698_PID_TX;
            mesg->ctrl  = ctx->service |(1<<7);
            mesg->qos   = ctx->choice;
            mesg->id    = 0x00;
            fr_rx_698_end(instance, pipeline, mesg);
            log_info("segments7: %d.\n", ctx->segment_no);
            return 0;
        }
    }
    return -1;//异常
}

int handle_scrambler(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    int i = 0;
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    
    if (is_scrambler_set(ctx->ctrl)){
        for(i = 0; i<ctx->apdu_rx_len; i++){
            apdu_rx[i] = apdu_rx[i] - 0x33;
        }
    }
    return 1;
}

int __fr_segment_tx_handle(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t type = 0;
    uint16_t segment_no = 0;
    uint16_t segment = 0;
    
    if((is_segment_set(ctx->ctrl)) && (ctx->segment_tx)){
        fifoBuf_getData(fifo, apdu_rx,  2);
        handle_scrambler(instance, pipeline, mesg);
        
        segment = apdu_rx[0] + (apdu_rx[1]<< 8);
        type = get_segment_type(segment);
        if(type == 2)
        {
            segment_no = get_segment_no(segment);
            if(segment_no == get_segment_no(ctx->segment_tx_no))
            {
                ctx->segment_tx_no++;
                __fr_send_handle(instance, pipeline, mesg);
                return 1;
            }
        }else{
            ctx->segment_rx = segment;
        }
    }
    ctx->segment_tx = 0;
    ctx->segment_tx_no = 0;
    return 0;
}

