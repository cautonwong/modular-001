#include "nidavellir.h"
#include "fr_698.h"

#define ActionRequestNormal             1
#define ActionRequestNormalList         2
#define ActionThenGetRequestNormalList  3


int fr_action_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}

int fr_handle_action_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    //uint8_t* p_apdu = NULL;
    uint16_t then_get_delay = 0; //读取延时
    uint32_t get_oad = 0;
    uint16_t len = 0;
    uint16_t data_len = 0;
    uint16_t dar_offset = 0;
    uint8_t dar = DAR_SUCCESS;

    p[cache->wr++] = 0x01;
    p[cache->wr++] = 0x05;//type encoder

    uint8_t password[16];

    uint8_t oad_n = mesg->oad_n;

    while (oad_n-- > 0)
    {
        //apdu分帧处理
       // if (fifoBuf_getFree(cache) < size(oad))
        {
            //omd
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            //fr_serve_one_object(instance, pipeline, mesg);
            //data
            len = axdr_get_len(fifo->buf_ptr + fifo->rd);
            cache->wr++;
            dar_offset = cache->wr;
            p[dar_offset] = dar;
            if (data_len = fr_action_one_object(instance, pipeline, mesg) < 0)
            {
                p[dar_offset]  = DAR_OTHERS_ERROR;
                return DAR_OTHERS_ERROR;
            }
            p[dar_offset] = dar;
            cache->wr += data_len;
            
            if(ActionThenGetRequestNormalList == ctx->choice)
            {
                //len 数据需要消耗掉
                //oad
                fifoBuf_getData(fifo, p + cache->wr, 4);
                get_oad = buf_to_uint32_msb(p + cache->wr);

                fifoBuf_getData(fifo, &then_get_delay, 2);
                then_get_delay = buf_to_uint16_msb((uint8_t *)&then_get_delay);
                //延时                
                
                if (fr_698_get(instance, get_oad, p+cache->wr, sizeof(password)) < 0)
                {
                    return DAR_OTHERS_ERROR;
                }
            }
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}