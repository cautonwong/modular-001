#include "nidavellir.h"
#include "fr_698.h"

extern int fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_get_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_set_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_action_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);




uint16_t get_crc16(uint16_t cs)
{
    cs ^= 0xffff;
    return cs;
}

int update_hcs_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->hcs = update_crc16(ctx->hcs, *byte);
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int update_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int __pack_apdu_byte(void* pctx, uint8_t* byte)
{
    struct pipeline_t* pipeline = pctx;
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    if (is_scrambler_set(ctx->ctrl))
    {
        *byte += 0x33;
    }
    append_byte(fifo, *byte, update_fcs, ctx);
    return 0;
}
/*
698.45协议判断过程
1 长度域L需要处理
	a 长度为3FFF位长度
	b 数据帧长度为bit0--bit13
	c bit14 0表示字节，1表示千字节
	d bit15为保留
	e 最小长度11  
	f 长度字节+2 > 接收字节
2 控制域C
	a bit7 传输方向DIR    注意启动方向位，识别判断
	b bit6 启动标志位PRM  判断DIR和PRM的组合形式
	c bit5 分帧标志
	d bit4 保留
	e bit3 干扰码标志sc
	f bit2--bit0 功能码  范围判断 1链路管理，3用户数据
3 地址域 A
    a sa 地址类型,逻辑地址,地址长度
    b 扩展逻辑地址/分路地址
    c 地址  广播地址的长度固定为1字节，广播地址=AAH
*/
int matcher_698(void* pctx, uint8_t* b)
{
    uint8_t byte = *b;
    struct context_698_t* ctx = pctx;

    if((ctx->step > 0) && (ctx->step <= 5))
    {
        //ctx->hcs = update_crc16(0xffff, byte);
        //ctx->fcs = update_crc16(0xffff, byte); 
    }

    switch (ctx->step)
    {
    case 0://帧头68判断
        if (byte != 0x68)
        {
            ctx->err_n++;
            return FRM_HEADER_ERR;
        }
        ctx->step = 1;
        return FRM_OK;
    case 1:
        ctx->hcs = update_crc16(0xffff, byte);
        ctx->fcs = update_crc16(0xffff, byte);
        ctx->min_len = byte;
        ctx->step = 2;
        return FRM_OK;
    case 2:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->min_len |= (byte << 8);
        ctx->min_len &= 0x3fff;
        //bit14：帧数据长度单位，0 表示帧数据长度单位为字节，1 表示帧数据长度单位为千字节；
        ctx->len_unit = (byte & 0x40) >> 6;
        if (ctx->min_len < 11)
        {
            ctx->step = 0;
            ctx->err_n++;
            return FRM_LEN_ERR1;
        }
        //if ((len < 11) || (len + 2 > *bytes) || (0x16 != p[len + 1]))
        ctx->step = 3;
        return FRM_OK;
    case 3:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if ( (FC_CODE(byte) != 1) && (FC_CODE(byte) != 3))
        {
            ctx->step = 0;
            return -1;
        }
        ctx->ctrl = byte;//控制域
        ctx->step = 4;
        return FRM_OK;
    case 4:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->SA = byte;//SA
        ctx->index = 0;
        ctx->step = 6;
        if(SA_LOGIC_TYPE(ctx->SA))
        {
            ctx->step = 5; 
        }
        return FRM_OK;

    case 5: //扩展逻辑地址处理
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->LA = byte;
        ctx->index = 0;
        ctx->step = 6;
        return FRM_OK;

    case 6:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (ctx->index == SA_LEN(ctx->SA) + 1)
        {
            ctx->CA = byte;
            ctx->step = 7;
        }
        else
        {
            ctx->addr[ctx->index++] = byte;
        }
        return FRM_OK;
    case 7:
        ctx->hcs = get_crc16(ctx->hcs);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != (ctx->hcs & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 8;
        return FRM_OK;
    case 8:
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != ((ctx->hcs >> 8) & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->index = 0;
        ctx->step = 9;
        return FRM_OK;
    case 9://用户链路数据
        if (is_scrambler_set(ctx->ctrl))
        {
            *b = byte - 0x33;
        }
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (ctx->index == ctx->min_len - 11 - SA_LEN(ctx->SA))//超过最大长度未测试？？
        {
            ctx->step = 10;
            return FRM_OK;
        }
        ctx->index++;
        return FRM_OK;
    case 10:
        ctx->fcs = get_crc16(ctx->fcs);
        if (byte != (ctx->fcs & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return 2;
        }
        ctx->step = 11;
        return FRM_OK;
    case 11:
        if (byte != ((ctx->fcs >> 8) & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return 2;
        }
        ctx->step = 12;
        return FRM_OK;
    case 12:
        if (byte != 0x16)
        {
            ctx->step = 0;
            return 2;
        }
        return 1;//接收到完整的一帧
    }

    assert(0);
    return -1;
}


int fr_pack_698_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;

    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;

    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698)
    {
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_tx_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698)
    {
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    //struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    //struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

    fifo_popData(ctx->apdu, mesg->len, __pack_apdu_byte, pipeline);
    return 1;
}

int fr_pack_698_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    ctx->fcs = get_crc16(ctx->fcs);
    fifoBuf_putByte(fifo, ctx->fcs & 0xff);
    ctx->tx_len++;
    fifoBuf_putByte(fifo, ((ctx->fcs >> 8) & 0xff));
    ctx->tx_len++;
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len++;
    return 1;
}

int fr_rx_698_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    //struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    //struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    //struct message_t* n = (struct message_t*)instance->cache;

    fifoBuf_getByte(ififo);//fcs-0
    fifoBuf_getByte(ififo);//fcs-1
    assert(fifoBuf_getByte(ififo) == 0x16);//0x16
    fr_pack_698_head(instance, pipeline, mesg);
    
    fr_pack_698_apdu(instance, pipeline, mesg);
    fr_pack_698_tail(instance, pipeline, mesg);
    //使能发送
    return 0;
}

////////////////////////////////////////////////////////////



int fr_pack_698_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct mbase_control_block_t* mbase_cb;

    //合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 1)
    {
        //自己是服务器  收到来自服务器的报文  丢掉
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }
    //控制域判断
    if (!((0 == DIR(ctx->ctrl)) && (1 == PRM(ctx->ctrl))))
        return 0;

    if(is_cmd(ctx->ctrl))
        return 0;

    //地址判断
    if (0 != SA_LOGIC(ctx->SA))
        return 0;


    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(NULL == mbase_cb)
        return 0;
#if WIN32_U_TEST
    if (0 != mem_compare(ctx->addr, mbase_cb->tsa, SA_LEN(ctx->SA) + 1))
    {
        //return 0;
    }
#endif
    //时标判断
    //fr_timeTag()；


    return 1;
}

int fr_apdu_effective_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{//协议一致性检查正常，检查APDU是否为有效数据

    return 1;
}



int fr_encode_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    //struct fifo_t* ofifo = (struct fifo_t*)pipeline->out_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    int r = 0;

    p[cache->wr++] = 0x00;//follow_report
    uint8_t time_tag = fifoBuf_getByte(fifo);
    p[cache->wr++] = time_tag;
    if (time_tag != 0)
    {
        r = fifoBuf_getData(fifo, p + cache->wr, 10);
        cache->wr += r;
    }
    
    return 1;
}


int fr_segments_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    if (is_segment_set(ctx->ctrl))//分帧标志位
    {
        if (fifoBuf_getUsed(ctx->apdu) > 0)
        {
            mesg->len = fifoBuf_getUsed(ctx->apdu);
            if (mesg->len > 512)
            {
                set_segment(ctx->ctrl);
                set_middle_segment_no(ctx->segment);//中间帧
                mesg->len = 512;
            }
            else
            {
                set_segment(ctx->ctrl);
                set_last_segment_no(ctx->segment);//最后帧
            }

            fr_rx_698_end(instance, pipeline, mesg);
        }
        fifoBuf_removeData(fifo, ctx->min_len + 2);//删掉收到的确认帧
        return 0;
    }
    else
    {
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
         return 1;
    }
}

int fr_gatekeeper_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t len = 0;

    //协议一致性判断和apdu合法判断
    if(0 == fr_pack_698_check(instance, pipeline, mesg))
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    
    if(0 == fr_segments_handle(instance, pipeline, mesg))
    {
        return -1;
    }

    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    ctx->service = fifoBuf_getByte(fifo);
    if (SECURITY_Request == ctx->service)//数据解密
    {
        ctx->choice = fifoBuf_getByte(fifo);
        len = fifoBuf_getByte(fifo);
        if (SECURITY_CHOICE_1_Ciphertext == ctx->choice)
        {
            fr_security_decode(instance, pipeline, mesg);
        }
        else if (SECURITY_CHOICE_0_Plaintext == ctx->choice)
        {
            ctx->service = fifoBuf_getByte(fifo);
        }
        else
        {
            return -1;
        }
    }
    if (!((CONNECT_Request == ctx->service) || (RELEASE_Request == ctx->service)))
    {
        ctx->choice = fifoBuf_getByte(fifo);
    }
    if ((ctx->service > ERROR_S_Response) || (ctx->choice > 9))
    {
        ctx->fr_err = FRM_CTRL_ERR;
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }

    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD

    mesg->from = pipeline->id;//从哪里来
    mesg->to = pid_tx_698;//要到哪里去
    mesg->ctrl = ctx->service |(1<<7);
    mesg->qos = ctx->choice;
    mesg->id = 0x00;//暂时没用

    struct fifo_t* cache = ctx->apdu;
    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);

    mesg->oad_n = 1;
    if (((GET_Request == ctx->service) &&((ctx->choice ==2)|| (ctx->choice == 4)))||
        ((SET_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3)))||
        ((ACTION_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3))))
    {
        mesg->oad_n = fifoBuf_getByte(fifo);
        fifoBuf_putByte(cache, mesg->oad_n);
    }
    if(0 == fr_apdu_effective_check(instance, pipeline, mesg))
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    switch (ctx->service)
    {
        case CONNECT_Request:
        case RELEASE_Request:
            fr_handle_connect(instance, pipeline, mesg);
            break;   
        case GET_Request:  
            fr_handle_get_request(instance, pipeline, mesg);
            break;
        case SET_Request:
            fr_handle_set_request(instance, pipeline, mesg);
            break;
        case ACTION_Request:
            fr_handle_action_request(instance, pipeline, mesg);
            break;

        case REPORT_Response://8上报应答 client
            break;
        default:
            return -1;
            break;
    }

    //数据加密
    fr_security_encode(instance, pipeline, mesg);

    fr_encode_optional(instance, pipeline, mesg);

    if (fifoBuf_getUsed(ctx->apdu)  > 0)
    {
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512)
        {
            set_segment(ctx->ctrl);
            set_first_segment_no(ctx->segment);//第一帧
            mesg->len = 512;
        }

        fr_rx_698_end(instance, pipeline, mesg);
    }
    return 0;
}

int fr_gatekeeper_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    //todo 合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 0)
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }

    if (is_segment_set(ctx->ctrl))//分帧标志位
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        if (is_last_segment(ctx->segment) > 0)//收到的是最后帧
        {
            //收齐了  解析 todo
            //get-next
        }
        else
        {
            //发送分帧传输的确认帧
        }
        return 0;
    }
    else
    {
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
    }
    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));

    ctx->service = fifoBuf_getByte(fifo);
    ctx->choice = fifoBuf_getByte(fifo);
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    fifoBuf_removeData(fifo, (fifo->wr - fifo->rd));

    return 0;
#if 0
    fifoBuf_removeData(fifo, ctx->min_len + 2);
    return 0;
    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    uint8_t service = fifoBuf_getByte(fifo);
    uint8_t type = fifoBuf_getByte(fifo);

    mesg->from = pipeline->id;//从哪里来
    mesg->to = pid_tx_698;//要到哪里去
    mesg->ctrl = service | (1 << 7);
    mesg->qos = type;
    mesg->id = 0x00;//暂时没用
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    struct fifo_t* cache = ctx->apdu;
    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);
    if (type == 1)
    {
        mesg->oad_n = 0;
    }
    else
    {
        mesg->oad_n = fifoBuf_getByte(fifo);
        fifoBuf_putByte(cache, mesg->oad_n);
    }

    switch (service)
    {
        case 0x85:
            fr_handle_get_request(instance, pipeline, mesg);
            break;
        case 0x86:
            fr_handle_set_request(instance, pipeline, mesg);
            break;
        case 0x87:
            fr_handle_action_request(instance, pipeline, mesg);
            break;
        case 0x02://connect
        case 0x03://release
            fr_handle_get_request(instance, pipeline, mesg);
            break;
        case 0x08://上报应答 client
            break;
        case 136://上报通知 server
            break;
        case 110://异常响应client
            break;
        case 238://异常响应server
            break;
        case 144://安全响应
            break;
    }

    if (fifoBuf_getUsed(ctx->apdu) > 0)
    {
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512)
        {
            set_segment(ctx->ctrl);
            set_first_segment_no(ctx->segment);//第一帧
            mesg->len = 512;
        }

        fr_rx_698_end(instance, pipeline, mesg);
    }
    return 0;
#endif
}


int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx->step = 0;
    int r = ringbuffer_filter(fifo, matcher_698, ctx);
    if (r > 0)
    {
        assert(r == ctx->min_len + 2);
#ifdef WIN32
        log_hex(fifo->buf_ptr + fifo->rd, fifo->wr - fifo->rd);
#endif
        return 1;
    }
    return r;
}

int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo_out = (struct fifo_t*)pipeline->out_queue;
#if WIN32
    struct fifo_t* fifo_in = (struct fifo_t*)pipeline->in_queue;
#endif
    struct fifo_t* cache = ctx->apdu;


    //client  或者 主动上报 或者 延时读取发送
    if (recv_mesg_header(instance, pipeline, mesg))
    {
#if WIN32
        if ((TEST_CASE_Request == mesg->ctrl) && (TEST_CASE_ID == mesg->id))
        {
            cache->wr = 0;//临时清0
            cache->rd = 0;
            fifoBuf_getData(fifo_in, cache->buf_ptr + cache->wr, mesg->len);

            fifoBuf_putData(fifo_out, cache->buf_ptr + cache->wr, mesg->len);
            return 0;
        }
#endif
        mesg->len -= 4;
        ctx->ctrl = 0x43;
        ctx->SA = 0x05;
        ctx->addr[0] = 0x08;
        ctx->CA = 0x00;
        fifoBuf_putByte(cache, mesg->ctrl);//05
        fifoBuf_putByte(cache, 0x01);//01
        fifoBuf_putByte(cache, ctx->PIID); 

        ringbuffer_putU32(cache, mesg->id);
        mesg->len = 7;
        ringbuffer_putU32(cache, 0);
        mesg->len += 1;

        fifoBuf_getData(fifo_out, cache->buf_ptr + cache->wr, mesg->len);
        cache->wr += mesg->len;
        

        //区分  上报ID和上报内容
        fr_pack_698_head(instance, pipeline, mesg);
        fr_pack_698_apdu(instance, pipeline, mesg);
        fr_pack_698_tail(instance, pipeline, mesg);
#ifdef VERBOSE
        uint8_t buf[256];
        ctx->rx_count++;
        fifo = (struct fifo_t*)pipeline->out_queue;
        uint16_t len = fifoBuf_getDataPeek(fifo, buf, ctx->tx_len + 2);
        printf("                   {%s,send", get_pipeline_name(instance, pipeline->id));
        printf(",");
        for (size_t i = 0; i < len; i++)
        {
            printf("%02x", buf[i]);
        }
        printf(",apdu:");
        for (size_t i = 9 + SA_LEN(ctx->SA); i < len - 3; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("}\n");
#endif
    }

    return 0;
}

//内存消耗分析:  <3.5K
//串口接收FIFO 544  
//串口发送FIFO 544 
//接收别的pl的主动发送FIFO  200
//用来装apdu的cache fifo  2048
//ctx