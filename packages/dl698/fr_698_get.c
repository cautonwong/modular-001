#include "nidavellir.h"
#include "fr_698.h"

#define GetRequestNormal            1
#define GetRequestNormalList        2
#define GetRequestRecord            3
#define GetRequestRecordList        4
#define GetRequestNext              5
#define GetRequestMD5               6
#define GetRequestSimplifyRecord    23




int fr_get_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint32_t oad = mesg->id;
    uint16_t len= 0;
    uint16_t offset = 0;

    if (((mesg->ctrl & 0x7f) == 0x06) ||
        ((mesg->ctrl & 0x7f) == 0x07))
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

        int r = fr_698_get(instance, oad, p + cache->wr, len);
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


static int __fr_handle_get_normal_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t n = 0;

    uint8_t oad_n = mesg->oad_n;

    while (oad_n-- > 0)
    {
        //if (fifoBuf_getFree(cache) < 2)
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;

            if (fr_get_one_object(instance, pipeline, mesg) < 0)
            {
                return DAR_OTHERS_ERROR;
            }
            cache->wr += n;
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}

#define REQ_RCSD_MAXNUM 50 
static int __fr_handle_get_record_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t n = 0;
    uint16_t offset = 0;
    uint8_t Selector = 0;
    uint8_t last_no = 0;
    uint8_t oad_n = mesg->oad_n;
    uint32_t rcsd_list[4*(REQ_RCSD_MAXNUM+1)]; //个数 + n个OAD  //[0][]协议筛选  [1][]事件全关联对象

    
    uint32_t i;
        
    while (oad_n-- > 0)
    {
        //if (fifoBuf_getFree(cache) < 2)
        {
            //OAD
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;

            Selector = fifoBuf_getByte(fifo);
            if ((Selector == 0) || (Selector == 1) || (Selector == 2))
            {
                //RSD
                //RCSD
            }
            if (Selector == 9)
            {
                //RSD
                last_no = fifoBuf_getByte(fifo);
                //RCSD
                rcsd_list[0] = fifoBuf_getByte(fifo); 
                
                if (rcsd_list[0] > REQ_RCSD_MAXNUM) // 请求的rcsd个数超出，返回错误 
                {
                     // 待添加
                }
                else // 赋值rcsd  
                {
                    if (rcsd_list[0] == 0) //全选
                    {
                        get_record_rcsd_list(instance, mesg->id, rcsd_list, 3);
                    
                        p[cache->wr++] = rcsd_list[0];
                        for (i = 0; i < rcsd_list[0]; i++ )
                        {
                            p[cache->wr++] = 0; // rcsd:oad
                            uint32_to_buf_msb(rcsd_list[1 + i], &p[cache->wr]);
                            cache->wr += 4;
                        }
                    }
                    else
                    {
                        p[cache->wr++] = rcsd_list[0];
                        for (i = 0; i < rcsd_list[0]; i++ )
                        {
                            p[cache->wr++] = fifoBuf_getByte(fifo); // csd
                            fifoBuf_getData(fifo, &p[cache->wr], 4);
                            rcsd_list[1 + i] = buf_to_uint32_msb(&p[cache->wr]);
                            cache->wr += 4;
                        }
                    }
                }

                offset = cache->wr++;
                int ret = GET_record_sel_9(instance, mesg->id, p + cache->wr, last_no, &rcsd_list[0]);
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

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
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
