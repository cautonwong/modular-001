#include "nidavellir.h"
#include "fr_698.h"

extern int fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_get_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_set_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_action_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);



static const uint16_t crc16_tab[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t crc16(uint16_t init, uint8_t* buf, size_t len)
{
    uint16_t cs = init;/**<  0xffff; */

    while (len--)
    {
        cs = (cs >> 8) ^ crc16_tab[(cs ^ *buf++) & 0xff];
    }
    cs ^= 0xffff;
    return cs;
}

uint16_t nd_get_crc16(uint8_t* buf, size_t len)
{
    uint16_t cs = 0xffff;

    while (len--)
    {
        cs = (cs >> 8) ^ crc16_tab[(cs ^ *buf++) & 0xff];
    }
    cs ^= 0xffff;
    return cs;
}

uint16_t update_crc16(uint16_t init, uint8_t byte)
{
    uint16_t cs = init;/**<  0xffff; */

    cs = (cs >> 8) ^ crc16_tab[(cs ^ byte) & 0xff];
    //cs ^= 0xffff;

    return cs;
}

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
            return -1;
        }
        ctx->step = 11;
        return FRM_OK;
    case 11:
        if (byte != ((ctx->fcs >> 8) & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 12;
        return FRM_OK;
    case 12:
        if (byte != 0x16)
        {
            ctx->step = 0;
            return -1;
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
#ifdef WIN32
    if(mesg->err_no == FRM_HEADER_ERR)
        fifoBuf_putByte(fifo, 0x66);
    else
#endif

    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
#ifdef WIN32
    if (mesg->err_no == FRM_LEN_ERR1)
        append_byte(fifo, ((6) & 0xff), update_hcs_fcs, ctx);
    else if (mesg->err_no == FRM_LEN_ERR2)
        append_byte(fifo, ((len + 2) & 0xff), update_hcs_fcs, ctx);
    else
#endif
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
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

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
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    struct message_t* n = (struct message_t*)instance->cache;

    fifoBuf_getByte(ififo);//fcs-0
    fifoBuf_getByte(ififo);//fcs-1
    //assert(fifoBuf_getByte(ififo) == 0x16);//0x16
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

    //合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 1)
    {
        //自己是服务器  收到来自服务器的报文  丢掉
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }
    //控制域判断
    if (!((0 == DIR(ctx->ctrl)) && (1 == PRM(ctx->ctrl))))
    {
        return 0;
    }
    if(is_cmd(ctx->ctrl))
    { 
        return 0;
    }
    //地址判断
    if (0 != SA_LOGIC(ctx->SA))
    {
        return 0;
    }
    uint8_t addr[6] = {0x01,0,0,0,0,0};
    if (0 != mem_compare(ctx->addr, addr, SA_LEN(ctx->SA) + 1))
    {
        return 0;
    }
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
    struct fifo_t* ofifo = (struct fifo_t*)pipeline->out_queue;
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

    //数据解密
    fr_security_decode(instance, pipeline, mesg);

    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));

    ctx->service = fifoBuf_getByte(fifo);
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

    mesg->oad_n = 0;
    if (ctx->choice > 1)
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
        log_hex(fifo->buf_ptr + fifo->rd, fifo->wr - fifo->rd);
        return 1;
    }
    return r;
}

int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo_out = (struct fifo_t*)pipeline->out_queue;
    struct fifo_t* fifo_in = (struct fifo_t*)pipeline->in_queue;
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