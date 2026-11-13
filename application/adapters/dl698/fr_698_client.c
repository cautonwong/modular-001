#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "fr_698_internal.h"

int fr_gatekeeper_698_client(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);

    //todo 合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 0){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }

    if (is_segment_set(ctx->ctrl)) { //分帧标志位
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        if (is_last_segment(ctx->segment) > 0) { //收到的是最后帧
            //收齐了  解析 todo
            //get-next
        }else{
            //发送分帧传输的确认帧
        }
        return 0;
    }else{
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
    }
    fifoBuf_removeData(fifo, APDU_OFFSET(ctx->SA));

    ctx->service = fifoBuf_getByte(fifo);
    ctx->choice = fifoBuf_getByte(fifo);
    ctx->PIID = fifoBuf_getByte(fifo);
    //ctx->PIID |= (1 << 6);//PIID-ACD

    if (ctx->forward != NULL) {
        uint16_t len = fifoBuf_getUsed(fifo);

        if (len > Max698_apdu_rx) {
            len = Max698_apdu_rx;
        }
        fifoBuf_getData(fifo, ctx->apdu_d_rx, len);
        if (fifoBuf_getUsed(fifo) > 0) {
            fifoBuf_removeData(fifo, fifoBuf_getUsed(fifo));
        }
        ctx->apdu_rx_len = len;
        mesg->len = len;
        mesg->payload = ctx->apdu_d_rx;
        mesg->oad_n = 0;
        mesg->ctrl = ctx->ctrl;
        mesg->qos = ctx->choice;
        mesg->id = 0;
        mesg->from = nd_fr698_pipeline_id(pipeline);
        ctx->forward(instance, pipeline, mesg);
    } else {
        fifoBuf_removeData(fifo, (fifo->wr - fifo->rd));
    }

    return 0;
}
