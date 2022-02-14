#include "nidavellir.h"
#include "fr_698.h"

#define SetRequestNormal            1
#define SetRequestNormalList        2
#define SetThenGetRequestNormalList 3

int get_SET_and_ACTION_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t rsd_len = 0, rcsd_len = 0;
    int len = 0;
    int i = 0;
    uint16_t axdr_len = 0;
    uint8_t list = 0;
    int offset = 0;
    uint8_t* apdu = instance->apdu_cache;

    if (ctx->choice > 3)
    {
        return -1;
    }

    list = 1;
    if ((ctx->choice == 0x02) || (ctx->choice == 0x03))
    {
        list = apdu[0];
        len = 1;
    }
    for (i = 0; i < list; i++)
    {
        len += 4;
        axdr_len = axdr_get_len(apdu+len);
        len += axdr_len;
        if (ctx->choice == 0x03)
        {
            len += 5;
        }
    }
    return len;
}

int fr_set_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len = 0;
    uint16_t offset = 0;
    int r = 0;

    if (((mesg->ctrl & 0x7f) == 0x09) ||
        (mesg->ctrl & 0x7f) == 0x09)
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

        r = fr_698_set(instance, oad, instance->apdu_cache + 4, len, (void*)ctx);
        //r = fr_698_set(instance, oad, fifo->buf_ptr + fifo->rd, len, (void*)ctx);
        
        cache->wr = offset;
        if (r < 0)
        {
            //p[cache->wr++] = 0x00;//DAR
            p[cache->wr++] = -r;
        }
        else
        {
            p[cache->wr++] = 0x00;//DAR
            //cache->wr += r;
        }
    }

    return r;
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
    uint16_t r = 0;

    uint8_t password[16];

    uint8_t oad_n = mesg->oad_n;
    if (ctx->segment_rx)
    {
        fifo = ctx->apdu_rx;;
    }
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

            r = fr_set_one_object(instance, pipeline, mesg);
            //cache->wr+n;
            fifoBuf_update_rd(fifo, r);
            
            if(SetThenGetRequestNormalList == ctx->choice)
            {
                fifoBuf_getData(fifo, p + cache->wr, 4);
                get_oad = buf_to_uint32_msb(p + cache->wr);

                fifoBuf_getData(fifo, &then_get_delay, 2);
                then_get_delay = buf_to_uint16_msb((uint8_t*)&then_get_delay);
                //延时                
                
                if (fr_698_get(instance, get_oad, p + cache->wr, sizeof(password), NULL) < 0)
                {
                    return DAR_OTHERS_ERROR;
                }
            }
        }
    }
    return 1;
}