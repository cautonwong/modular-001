#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "fr_698_internal.h"

#define SetRequestNormal            1
#define SetRequestNormalList        2
#define SetActionThenGetRequestNormalList 3

#define ActionRequestNormal             1
#define ActionRequestNormalList         2
#define ActionThenGetRequestNormalList  3


extern int security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad);


int get_SET_and_ACTION_Request_len(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    int len = 0;
    int i = 0;
    uint16_t tmp_num = 1, axdr_len = 0;
    uint8_t list = 0;
    uint8_t *apdu = ctx->apdu_d_rx + ctx->apdu_rx_offset;

    if (ctx->choice > 3)
    {
        return -1;
    }

    list = 1;
    if ((ctx->choice == 0x02) || (ctx->choice == 0x03))
    {
        len += axdr_decode(TYPE_octet_string, apdu, &tmp_num);  
        list = tmp_num;
        //list = apdu[0];
        //len = 1;
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

static int __fr_handle_one_object(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len = 0;
    uint16_t offset = 0;
    uint8_t req_type = 0;
    int r = 0;

    if ((mesg->ctrl & 0x7f) == 0x09)
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

        if(ACTION_Request == ctx->service)
            req_type = 2;
        if(SET_Request == ctx->service)
            req_type = 1;
        
        r = security_mode_word_check(instance, pipeline, req_type, oad);
        if(r < 0)
        {
            //ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
            p[cache->wr++] = -r;
            if(ACTION_Request == ctx->service)
            {
                p[cache->wr++] = 0;
            }
            r = axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
            return r; 
        }  
        
        if(2 != ctx->timeTag_valid){
            if(ACTION_Request == ctx->service){
                r = fr_698_action(instance, oad, apdu_rx + ctx->apdu_rx_offset, len, (void*)ctx);
            }
            
            if(SET_Request == ctx->service){
                r = fr_698_set(instance, oad, apdu_rx + ctx->apdu_rx_offset, len, (void*)ctx);
            }
        }else{
            //app_handle_timeTag_invalid(instance, oad);
            r = -DAR_TIME_TAG_INVALID;
        }
        
        //cache->wr = offset;
        if (r < 0)
        {
            p[cache->wr++] = -r;//DAR
            if(ACTION_Request == ctx->service){
                p[cache->wr++] = 0;
            }
            r = axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
        }
        else
        {
            if(cache->wr > offset)
            {
                p[offset++] = 0;
                if(ACTION_Request == ctx->service){
                    p[offset++] = 1;
                }
            }
            else
            {
                p[cache->wr++] = 0;
                if(ACTION_Request == ctx->service){
                    p[cache->wr++] = 0;
                }
            }
            fr_698_MeterWr_program_judge(instance, pipeline, mesg); //编程记录
        }
    }

    return r;
}


const uint32_t support_broadcast_action_cmd_tab[] =
{
    0x40007f00, //广播校时
    0xf2077f00, //多功能端子投切
    0x50000300, //瞬时冻结触发
};


int support_broadcast_addr_cmd_check(uint8_t cmd, uint32_t oad)
{
    uint8_t i, number;
    const uint32_t *p;

    if (cmd == ACTION_Request)
    {
        p = support_broadcast_action_cmd_tab;
        number = (sizeof(support_broadcast_action_cmd_tab) / sizeof(support_broadcast_action_cmd_tab[0]));
    }
    else
    {
        return -1;
    }

    for (i = 0; i < number; i++)
    {
        if (oad == *p)
        {
            return 1;
        }

        p++;
    }

    return -1;
}



int fr_handle_set_action_request(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;

    uint32_t get_oad = 0;
    uint16_t offset = 0;
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


            if (3 == get_addr_type(ctx->SA)) 
            {
                r = support_broadcast_addr_cmd_check(ctx->service, mesg->id);
                if (r < 0)
                {
                    return -DAR_RW_REJECT;
                }
            }

            r = __fr_handle_one_object(instance, pipeline, mesg);
            if(r >= 0)
                ctx->apdu_rx_offset += r;

            if(SetActionThenGetRequestNormalList == ctx->choice)
            {
                get_oad = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
                memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
                cache->wr += 4;
                ctx->apdu_rx_offset += 4;
            
                ctx->apdu_rx_offset++;
                //延时                
                
                r = security_mode_word_check(instance, pipeline, 0, get_oad);
                if(r < 0)
                {
                    ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                    p[offset] = 0x00;//DAR
                    p[cache->wr++] = -r;
                    return 1;
                }  
                
                offset = cache->wr;
                p[cache->wr++] = 0x01;
                
                r = fr_698_get(instance, get_oad, p+cache->wr, 0, NULL);
                if (r < 0)
                {
                    p[offset] = 0x00;//DAR
                    p[cache->wr++] = -r;
                    //r = axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                }
                else
                {
                    cache->wr += r;//DAR
                }
        
            }
        }
    }
    return 1;
}
