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
extern int GET_record_sel_1(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t buf_size,  void *ctx);
extern int GET_record_sel_2(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t buf_size,  void *ctx);
extern int GET_record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t buf_size,  void *ctx);

int get_GET_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t tmp_num = 1, rsd_len = 0, rcsd_len = 0;
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

            len += axdr_decode(TYPE_octet_string, apdu, &tmp_num);  
            list = tmp_num;     
            //list = apdu[0];
            //len++;
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
            ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
            p[offset] = 0x00;//DAR
            p[cache->wr++] = -r;
            return 1;
        }  

        if(2 != ctx->timeTag_valid){
            r = fr_698_get(instance, oad, instance->apdu_cache, len, (void*)ctx); // p + cache->wr   改为协议临时缓存
        }else{
            r = -DAR_TIME_TAG_INVALID;
        }
        if (r < 0)
        {
            p[offset] = 0x00;//DAR
            p[cache->wr++] = -r;
            //r = axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
        }
        else
        {

            if (apdu_cache_getFree(cache) < r)
            {
                p[offset] = 0x00;//DAR
                p[cache->wr++] = DAR_BUFFER_OVERFLOW; //越界
            }
            else
            {
                ndv_memcopy((p + cache->wr), instance->apdu_cache, r);            
                cache->wr += r;//DAR
            }
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
        if (apdu_cache_getFree(cache) > 6){   
            mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
            cache->wr += 4;
            ctx->apdu_rx_offset += 4;

            if (fr_get_one_object(instance, pipeline, mesg) < 0)
            {
                return DAR_OTHERS_ERROR;
            }
            cache->wr += n;
#if 0
            cache->wr += 1200;            
            ctx->next_index = 0;
#endif
        }
    }


    return 1;   
}



static int __fr_handle_get_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;

    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t* p = cache->buf_ptr;
    uint16_t offset = 0;
    uint8_t Selector = 0;
    uint8_t oad_n = mesg->oad_n;
    uint32_t rcsd_list[(REQ_RCSD_MAXNUM+1)]; //个数 + n个OAD  //[0][]协议筛选  [1][]事件全关联对象
    struct rsd_info_t rsd_info;
    int ret, rsd_ret, rcsd_ret;
        
    while (oad_n-- > 0)
    {
        if (apdu_cache_getFree(cache) > 8) //待调整
        {
            //OAD
            mesg->id = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 4);
            cache->wr += 4;
            ctx->apdu_rx_offset += 4;
            
            //检查 读取的记录型oad合法性
            ret = check_record_table_oad__support(instance, mesg->id);
            if (ret < 0)
            {
                ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                p[cache->wr++] = 0x00; //rcsd
                p[cache->wr++] = 0x00;//DAR
                p[cache->wr++] = -ret;
                return ret;  
            }
            //RSD
            rsd_ret = rsd_decode(instance, pipeline, mesg->id, &rsd_info);
            if (rsd_ret < 0)
            {
                ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                p[cache->wr++] = 0x00; //rcsd
                p[cache->wr++] = 0x00;//DAR
                p[cache->wr++] = -rsd_ret;
                return rsd_ret;  
            }
            else
            {
                
            }  

            Selector = rsd_info.select;

            //RCSD
            rcsd_ret = rcsd_decode(instance, pipeline,mesg->id, rcsd_list);

            if(rsd_ret == 0)
            {
                //ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                if (rcsd_ret < 0)
                {
                    //p[cache->wr++] = 0x00; //rcsd
                }
                p[cache->wr++] = 0x00;//DAR
                p[cache->wr++] = DAR_TYPE_NOT_MATCH;
                return -DAR_TYPE_NOT_MATCH;  
            }
            
            if ((rcsd_ret < 0 ))
            {   

                ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                //p[cache->wr++] = 0x00; //rcsd
                p[cache->wr++] = 0x00;//DAR
                p[cache->wr++] = -rcsd_ret;
                return rcsd_ret;  // 想返回dar  如何处理 ？？
            }


            //安全模式判断
            ret = security_mode_word_check(instance, pipeline, 0, mesg->id);
            if(ret < 0){
                ctx->apdu_rx_offset += axdr_get_len(apdu_rx + ctx->apdu_rx_offset);
                p[cache->wr++] = 0x00;//DAR
                p[cache->wr++] = -ret;
                return ret;
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
                        ret = GET_record_sel_1(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list, apdu_cache_getFree(cache), ctx); //指定时间或序号
                        break;
                    case 2:
                        ret = GET_record_sel_2(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list, apdu_cache_getFree(cache), ctx);// 按序号、时间范围搜索，带间隔
                        break;
                    case 9:
                        ret = GET_record_sel_9(instance, mesg->id, p + cache->wr, &rsd_info, rcsd_list, apdu_cache_getFree(cache), ctx);// 上X次
                        break;
                    default:
                        break;
                }
                
                if (ret < 0)
                {
                    p[offset] = 0;
                    p[offset + 1] = -ret; // 无记录
                    //p[offset+1] = ret;
                    cache->wr += 1;
                    return -DAR_OTHERS_ERROR;
                }
                else
                {
                    p[offset] = 1;// 有记录
                    cache->wr += ret;
                    #if 0
                    cache->wr += 1200;                   
                    ctx->next_index = 1;
                    #endif
                }
            }            
        }
    }

    return 1;
}

uint8_t cnt = 0;

static int __fr_handle_get_next_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    
    // 恢复上一个读取上下文信息
    
    //调用api获取内容
    //    return record_data_package(instance, oad, buf, rsd_info, rcsd_info,  ctx);
    //  ctx->next_que = 1; 几条
    //判断是否大于1800
  
#if 0
    cnt++;
    ctx->next_index = 1;
    if(cnt>10){
        ctx->next_last_frm = 1;
    }
    //剩余记录为0，清 ctx->next_index = 0;
    cache->wr = 600;
#endif
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
    struct fifo_t* cache = ctx->apdu;
    uint8_t data[16] = {0};
    uint8_t next_choice = 0; 
    
    if(GET_Request != ctx->service){
        return 0;
    }
    
    switch (ctx->choice)
    {
        case GetRequestNormal:
        case GetRequestNormalList:
            ctx->next_index = 0;
            __fr_handle_get_normal_request(instance, pipeline, mesg);
            next_choice = 1;
            ctx->next_last_frm = 0;
            ctx->next_index2_frm = 0;
            cnt = 0;
            break;   
        case GetRequestRecord:  
        case GetRequestRecordList:
            ctx->next_index = 0;
            __fr_handle_get_record_request(instance, pipeline, mesg);
            next_choice = 2;
            ctx->next_last_frm = 0;
            ctx->next_index2_frm = 0;
            cnt = 0;
            break;
        case GetRequestNext:
            __fr_handle_get_next_request(instance, pipeline, mesg);
            ctx->next_index2_frm++;
            next_choice = ctx->next_choice;
            break;
        case GetRequestMD5:
            ctx->next_index = 0;
            __fr_handle_get_md5_request(instance, pipeline, mesg);
            break;
        case GetRequestSimplifyRecord:
            ctx->next_index = 0;
            __fr_handle_get_simplify_record_request(instance, pipeline, mesg);
            break;
        default:
            return -2;
            break;
    }
        
    if(1 == ctx->next_index){
        ctx->next_index1_frm=0;
        ctx->next_choice = next_choice;
        ctx->next_que = 1;
        if(0 == ctx->next_choice){
            ctx->next_que = 0;
        }        
        data[0] = 0x85;
        data[1] = 0x05;
        data[2] = cache->buf_ptr[2];
        memcpy(data + 3, &ctx->next_last_frm, 5);
        memcpy(ctx->apdu->buf_ptr, data+5, 3);
        fifoBuf_update_rd(ctx->apdu, 5, 0);
        memcpy(ctx->apdu->buf_ptr + ctx->apdu->rd, data, 5);
    }
    return 1;
}
