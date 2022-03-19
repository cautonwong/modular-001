#include "nidavellir.h"
#include "fr_698.h"

#define ActionRequestNormal             1
#define ActionRequestNormalList         2
#define ActionThenGetRequestNormalList  3


int fr_action_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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
        //p[cache->wr++] = 0x01;
        
        
        r = security_mode_word_check(instance, pipeline, 2, oad);
        if(r < 0){
            return r;
        }

        r = fr_698_action(instance, oad, apdu_rx + ctx->apdu_rx_offset, len, (void*)ctx);
        if (r < 0)
        {
            p[cache->wr++] = -r;//DAR
            r = axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
        }
        else
        {
            if(cache->wr > offset)
            {
                p[offset++] = 0;
                p[offset++] = 1;
            }
            else
            {
                p[cache->wr++] = 0;
                p[cache->wr++] = 0;
            }
        }
    }

    return r;
}

int fr_handle_action_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;

    uint16_t then_get_delay = 0; //读取延时
    uint32_t get_oad = 0;
    uint16_t len = 0;
    int16_t data_len = 0;
    uint16_t dar_offset = 0;
    uint8_t dar = DAR_SUCCESS;

    uint8_t password[16];
    uint8_t oad_n = mesg->oad_n;
    int r = 0;

    while (oad_n-- > 0)
    {
        //apdu分帧处理
       // if (fifoBuf_getFree(cache) < size(oad))
        {
            //omd
            mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
            cache->wr += 4;
            ctx->apdu_rx_offset += 4;
            
            data_len = fr_action_one_object(instance, pipeline, mesg);
            ctx->apdu_rx_offset += data_len;
            
            if(ActionThenGetRequestNormalList == ctx->choice)
            {
                //len 数据需要消耗掉
                //oad
                get_oad = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
                memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
                cache->wr += 4;
                ctx->apdu_rx_offset += 4;
            
                then_get_delay = buf_to_uint16_msb(apdu_rx+ctx->apdu_rx_offset);
                ctx->apdu_rx_offset += 2;
                //延时                
                
                r = security_mode_word_check(instance, pipeline, 0, get_oad);
                if(r < 0){
                    return r;
                }        
                
                if (fr_698_get(instance, get_oad, p+cache->wr, sizeof(password), NULL) < 0)
                {
                    return DAR_OTHERS_ERROR;
                }
            }
        }
    }
    return 1;
}