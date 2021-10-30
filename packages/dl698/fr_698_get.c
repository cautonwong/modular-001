#include "kernel\nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"


#define GetRequestNormal            1
#define GetRequestNormalList        2
#define GetRequestRecord            3
#define GetRequestRecordList        4
#define GetRequestNext              5
#define GetRequestMD5               6
#define GetRequestSimplifyRecord    23


int8_t _fr_698_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    uint16_t i = 0;
    for(i =0 ; i < buf_size ; i++)
    {
        buf[i] = 0x11*(i+1);
    }
    return buf_size;
}


int fr_serve_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len= 0;
    uint16_t offset = 0;

    if (((mesg->ctrl & 0x7f) == 0x06) ||
        (mesg->ctrl & 0x7f) == 0x07)
    {
        uint8_t type = fifoBuf_getByte(fifo);
        if (type == 0x09)//octet-string
        {
            uint8_t len = fifoBuf_getByte(fifo);
            fifoBuf_getData(fifo, p + cache->wr, len);
            //todo write data
            p[cache->wr++] = 0x00;
        }
    }
    else
    {
        //if (oad->coder->size() > fifoBuf_getFree(cache))
        {
            //return -1;
        }
        offset = cache->wr;
        p[cache->wr++] = 0x01;

        //class的处理开始  找到对应class的编解码器
        if (get_OAD_Class(oad) == 0) //电能量
        {
            len = 4;
            if (get_OAD_Attribute(oad) == 2) 
                p[cache->wr++] = TYPE_double_long;//type encoder
            if (get_OAD_Attribute(oad) == 4)
            {
                p[cache->wr++] = TYPE_long64_unsigned;//type encoder
                len = 8;
            }
            if (get_OAD_Attribute(oad) == 6)
            {
                p[cache->wr++] = TYPE_unsigned;//type encoder
                len = 1;
            }
        }        
           
        if (get_OAD_Class(oad) == 1) //需量
        {
            //p[cache->wr++] = 2;
            //p[cache->wr++]
        }

        int r = _fr_698_get(instance, oad, p + cache->wr, len);
        //fifo->rd += r;
        if (r < 0)
        {
            p[cache->wr++] = 0x00;//DAR
            p[cache->wr++] = -r;
        }
        else
        {
            cache->wr += r;
        }
    }

    return 1;
}


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
    if (oad_n > 10)
        oad_n = 10;
    while (oad_n-- > 0)
    {
        //if (fifoBuf_getFree(cache) < 2)
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;

            if (fr_serve_one_object(instance, pipeline, mesg) < 0)
            {
                return DAR_OTHERS_ERROR;
            }
            cache->wr += n;
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}


static int __fr_handle_get_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
}

static int __fr_handle_get_next_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
}

static int __fr_handle_get_md5_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
}

static int __fr_handle_get_simplify_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
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
        case 0://
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
