#include "nidavellir.h"
#include "fr_698.h"

#define GetRequestNormal            1
#define GetRequestNormalList        2
#define GetRequestRecord            3
#define GetRequestRecordList        4
#define GetRequestNext              5
#define GetRequestMD5               6
#define GetRequestSimplifyRecord    23

extern uint16_t axdr_record_len_get(uint8_t* data, uint16_t* rsdLen, uint16_t* rcsdLen);

int get_GET_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t rsd_len = 0, rcsd_len = 0;
    int len = 0;
    int i = 0;
    uint8_t list = 0;

    uint8_t *apdu = ctx->apdu_d_rx + ctx->apdu_rx_offset;

    if (ctx->choice > GetRequestNext){
        return -1;
    }
    if (ctx->choice == GetRequestNext){
        len = 2;
    }else{
        list = 1;
        if ((ctx->choice == 0x02) || (ctx->choice == 0x04)){
            list = apdu[0];
            len++;
        }

        for (i = 0; i < list; i++){
            len += 4;
            if ((ctx->choice == 0x03) || (ctx->choice == 0x04)){
                rsd_len = 0;
                rcsd_len = 0;
                len = axdr_record_len_get(apdu, &rsd_len, &rcsd_len);
                if (rsd_len == 0 || rcsd_len == 0){
                    return -1;
                }
            }
        }
    }
    return len;
}


int fr_get_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len= 0;
    uint16_t offset = 0;
    int r = 0;
    
    if (((mesg->ctrl & 0x7f) == 0x06) ||
        ((mesg->ctrl & 0x7f) == 0x07))
    {
        uint8_t type = apdu_rx[ctx->apdu_rx_offset++];
        if (type == 0x09)//octet-string
        {
            uint8_t len = apdu_rx[ctx->apdu_rx_offset++];
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, len);
            ctx->apdu_rx_offset += len;
            //todo write data
            p[cache->wr++] = 0x00;
        }
    }
    else
    {
        offset = cache->wr;
        p[cache->wr++] = 0x01;
        
        r = security_mode_word_check(instance, pipeline, 0, oad);
        if(r < 0){
            return r;
        }  
        
        r = fr_698_get(instance, oad, p + cache->wr, len, (void*)ctx);
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

    return 1;
}

static int __fr_handle_get_normal_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t* p = cache->buf_ptr;
    uint16_t n = 0;

    uint8_t oad_n = mesg->oad_n;

    while (oad_n-- > 0)
    {
        mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
        memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
        cache->wr += 4;
        ctx->apdu_rx_offset += 4;

        if (fr_get_one_object(instance, pipeline, mesg) < 0)
        {
            return DAR_OTHERS_ERROR;
        }
        cache->wr += n;
    }

    return 1;
}



static int __fr_handle_get_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;

    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t* p = cache->buf_ptr;
    uint16_t n = 0;
    uint16_t offset = 0;
    uint8_t Selector = 0;
    uint8_t last_no = 0;
    uint8_t oad_n = mesg->oad_n;
    uint32_t rcsd_list[(REQ_RCSD_MAXNUM+1)]; //个数 + n个OAD  //[0][]协议筛选  [1][]事件全关联对象
    struct rsd_info_t rsd_info;
    
    int ret;
    uint32_t i;
        
    while (oad_n-- > 0)
    {
        //if (fifoBuf_getFree(cache) < 2)
        {
            //OAD
            mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
            cache->wr += 4;
            ctx->apdu_rx_offset += 4;
            
            //RSD
            ret = rsd_decode(instance, pipeline, mesg->id, &rsd_info);
            if (ret < 0)
            {
                //p[offset] = 0; // 无记录
                //p[offset+1] = -ret;
                return DAR_RW_REJECT;  // 想返回dar  如何处理 ？？
            }
            else
            {
                //cache->wr += ret;
            }  
            Selector = rsd_info.select;
            
            //RCSD
            ret = rcsd_decode(instance, pipeline,mesg->id, rcsd_list);
            if (ret < 0)
            {
                //p[offset] = 0; // 无记录
                //p[offset+1] = -ret;
                return DAR_RW_REJECT;  // 想返回dar  如何处理 ？？
            }
            else
            {
                //cache->wr += ret;
            } 
            
            if ((Selector == 0) || (Selector == 1) || (Selector == 2)|| (Selector == 9))
            {
                offset = cache->wr++;
                switch(Selector)
                {
                    case 0:
                        //ret = GET_record_sel_0(instance, pipeline, mesg);
                        break;
                    case 1:
                        ret = GET_record_sel_1(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list); //指定时间或序号
                        break;
                    case 2:
                        ret = GET_record_sel_2(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list);// 按序号、时间范围搜索，带间隔
                        break;
                    case 9:
                        ret = GET_record_sel_9(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list);// 上X次
                        break;
                    default:
                        break;
                }
                
                if (ret < 0)
                {
                    p[offset] = 0; // 无记录
                    p[offset+1] = -ret;
                    return DAR_OTHERS_ERROR;
                }
                else
                {
                    p[offset] = 1;// 有记录
                    cache->wr += ret;
                }
            }            
        }
    }

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

    if(GET_Request != ctx->service){
        return 0;
    }
    
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
            return -2;
            break;
    }

    return 1;
}
