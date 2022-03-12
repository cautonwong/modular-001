#include "nidavellir.h"
#include "fr_698.h"


int fr_follow_report_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    
    return 0;
    //读取3320 判断是否跟随上报
    //{
    //    p[cache->wr++] = 0x01;//follow_report
    //}
}
