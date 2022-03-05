#include "nidavellir.h"
#include "fr_698.h"

#define SetRequestNormal            1
#define SetRequestNormalList        2
#define SetThenGetRequestNormalList 3

int get_SET_and_ACTION_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    int len = 0;
    int i = 0;
    uint16_t axdr_len = 0;
    uint8_t list = 0;
    uint8_t *apdu = ctx->apdu_d_rx + ctx->apdu_rx_offset;

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
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len = 0;
    uint16_t offset = 0;
    int r = 0;

    if (((mesg->ctrl & 0x7f) == 0x09) ||
        (mesg->ctrl & 0x7f) == 0x09)
    {
        uint8_t type = apdu_rx[ctx->apdu_rx_offset++];
        if (type == 0x09)//octet-string
        {
            uint8_t len = apdu_rx[ctx->apdu_rx_offset++];
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, len);
            ctx->apdu_rx_offset += len;
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
        
        r = security_mode_word_check(instance, pipeline, 1, oad);
        //安全模式不匹配 待调整
        
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
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t then_get_delay = 0; //读取延时
    uint32_t get_oad = 0;
    uint16_t r = 0;

    uint8_t password[16];

    uint8_t oad_n = mesg->oad_n;

    while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            //oad
            mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
            cache->wr += 4;
            ctx->apdu_rx_offset += 4;

            r = fr_set_one_object(instance, pipeline, mesg);
            ctx->apdu_rx_offset += r;
            
            if(SetThenGetRequestNormalList == ctx->choice)
            {
                get_oad = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
                memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
                cache->wr += 4;
                ctx->apdu_rx_offset += 4;
            
                then_get_delay = buf_to_uint16_msb(apdu_rx+ctx->apdu_rx_offset);
                ctx->apdu_rx_offset += 2;
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