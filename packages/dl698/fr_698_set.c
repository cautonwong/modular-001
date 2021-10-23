#include "kernel\nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"


#define SetRequestNormal            1
#define SetRequestNormalList        2
#define SetThenGetRequestNormalList 3

int fr_set_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    //if (nd_set(instance, mesg->id, cache->wr, sizeof(password)) < 0)
    {
        return DAR_OTHERS_ERROR;
    }
    //cache->wr+n;
}

int fr_handle_set_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t then_get_delay = 0; //读取延时
    uint32_t get_oad = 0;
    uint16_t n = 0;

    uint8_t password[16];

    uint8_t oad_n = mesg->oad_n;
    if (oad_n == 0) oad_n = 1;
    while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            //oad
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            //fr_serve_one_object(instance, pipeline, mesg);
            //data

            fr_set_one_object(instance, pipeline, mesg);
            cache->wr+n;
            
            if(SetThenGetRequestNormalList == ctx->choice)
            {
                fifoBuf_getData(fifo, p + cache->wr, 4);
                get_oad = buf_to_uint32_msb(p + cache->wr);

                fifoBuf_getData(fifo, &then_get_delay, 2);
                then_get_delay = buf_to_uint16_msb(then_get_delay);
                //延时                
                
                if (nd_get(instance, get_oad, cache->wr, sizeof(password)) < 0)
                {
                    return DAR_OTHERS_ERROR;
                }
            }
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}