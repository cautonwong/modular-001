#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "meter_base_view.h"
#include "fr_698_internal.h"


int fr_rx_698_entry(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    ctx->step = 0;
#ifdef __DEBUG_LOG__
    uint16_t val_rd = fifo->rd;
    uint16_t val_wr = fifo->wr;
#endif 
    int r = ringbuffer_filter(fifo, __matcher_698, ctx);
    if (r > 0){
        n_assert(r == ctx->min_len + 2);
#ifdef WIN32
        log_hex(fifo->buf_ptr + fifo->rd, fifo->wr - fifo->rd);
#endif
        return 1;
    }
#ifdef __DEBUG_LOG__
    uint8_t i = 0;
    log_err("rd:%d, wr:%d, step=%d, err_type=%d, r=%d.\n", val_rd, val_wr, ctx->step, ctx->err_type, r);
    for(i =0; i<4; i++){
        log_hex(fifo->buf_ptr+128*i, 128);
    }
    log_hex(fifo->buf_ptr+128*i, 32);
#endif
    return r;
}
