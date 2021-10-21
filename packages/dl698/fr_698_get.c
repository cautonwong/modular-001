#include "nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"

#define GetRequestNormal            1
#define GetRequestNormalList        2
#define GetRequestRecord            3
#define GetRequestRecordList        4
#define GetRequestNext              5
#define GetRequestMD5               6
#define GetRequestSimplifyRecord    23

//请求读取一个对象属性 [1] GetRequestNormal，
//请求读取若干个对象属性 [2] GetRequestNormalList，
static int __fr_handle_get_normal_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t n = 0;
    uint8_t password[16];
    uint8_t oad_n = mesg->oad_n;
    if (oad_n == 0) oad_n = 1;
    while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            fr_serve_one_object(instance, pipeline, mesg);
            if (nd_get(instance, mesg->id, cache->wr, sizeof(password)) < 0)
            {
                return DAR_OTHERS_ERROR;
            }
            cache->wr+n;
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}


static int __fr_handle_get_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
}

static int __fr_handle_get_next_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
}

static int __fr_handle_get_md5_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
}

static int __fr_handle_get_simplify_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
}

int fr_handle_get_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;

    if(GET_Request != ctx->service)
        return 0;

    switch (ctx->choice)
    {
        case GetRequestNormal:
        case GetRequestNormalList:
            __fr_handle_get_normal_request(instance, pipeline, mesg);
            break;   
        case GetRequestRecord:  
        case GetRequestRecordList:
            __fr_handle_get_record_request(instance, pipeline, mesg);
            break;
        case GetRequestNext:
            __fr_handle_get_next_request(instance, pipeline, mesg);
            break;
        case GetRequestMD5:
            __fr_handle_get_md5_request(instance, pipeline, mesg);
            break;
        case GetRequestSimplifyRecord:
            __fr_handle_get_simplify_record_request(instance, pipeline, mesg);
            break;
        default:
            break;
    }


#if 0
    uint8_t oad_n = mesg->oad_n;
    if (oad_n == 0) oad_n = 1;
    while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            fr_serve_one_object(instance, pipeline, mesg);
        }
    }
#endif
    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}
