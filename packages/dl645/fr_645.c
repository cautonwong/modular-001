#include "nidavellir.h"
#include "gs_record.h"
#include "645id_table.h"
#include "fr_645_report.h"



extern signed char ft_adj_set_meter(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern signed char Broadcast_timeadj(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char MulPort_set(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char meteraddr_write(struct instance_t* instance, uint8_t* p_rx);
extern signed char dl645_event_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern signed char dl645_meter_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern signed char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len, struct context_645_t* ctx);
extern int8_t safe_Authen03_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
int __update_cs(void* pctx, uint8_t* byte)
{
    struct context_645_t* ctx = pctx;
    ctx->tx_len++;
    ctx->cs += *byte;
    return 0;
}

int __update_scrambler_and_cs(void* pctx, uint8_t* byte)
{
    struct context_645_t* ctx = pctx;
    ctx->tx_len++;
    *byte += 0x33;
    ctx->cs += *byte;
    return 0;
}
int __pack_apdu_byte_645(void* pctx, uint8_t* byte)
{
    struct pipeline_t* pipeline = pctx;
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

    append_byte(fifo, *byte, __update_scrambler_and_cs, ctx);
    return 0;
}

int fr_pack_645_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    fifo_popData(&ctx->apdu, mesg->len, __pack_apdu_byte_645, pipeline);
    return 1;
}

int __matcher_645(void* pctx, uint8_t* b)
{
    struct context_645_t* ctx = pctx;
    uint8_t byte = *b;

    switch (ctx->step)
    {
    case 0:
        if (byte != 0x68)
        {
            ctx->err_n++;
            return -1;
        }
        ctx->step = 1;
        ctx->addr_n = 0;
        ctx->min_len = 12;
        ctx->cs = byte;
        return 0;
    case 1:
        ctx->cs += byte;
        if (ctx->addr_n == 6)
        {
            if (byte != 0x68)
            {
                ctx->step = 0;
                return -1;
            }
            ctx->step = 2;
            return 0;
        }
        ctx->addr[ctx->addr_n++] = byte;//�յ��ĵ�ַ
        return 0;
    case 2:
        ctx->ctrl = byte;
        ctx->cs += byte;
        if(ctx->ctrl == 0xfa){
            ctx->step = 3;
            ctx->ver = 1;
        }
        else {
            ctx->step = 5;    
            ctx->ver = 0;
        }
        return 0;
    case 3:
        ctx->min_len = 13;
        ctx->len = byte;
        ctx->cs += byte;
        ctx->step = 4;
        return 0;
    case 4:
        if(ctx->ctrl == 0xfa){
            ctx->len += (byte<<8);
            ctx->min_len = ctx->len + 13;          
            if (ctx->len > 1024)
            {
                ctx->step = 0;
                return -1;
            }
        }
        ctx->cs += byte;
        ctx->addr_n = 0;
        ctx->step = 6;
        return 0;
    case 5:
        if (byte > data_max_len)
        {
            ctx->step = 0;
            return -1;
        }
        ctx->len = byte;
        ctx->min_len = byte + 12;
        //if (ctx->min_len > fifoBuf_getUsed(pipeline->in_queue))
        {
    //            return 2;//���Ȳ���  �´�����
        }
        ctx->cs += byte;
        ctx->addr_n = 0;
        ctx->step = 6;
        return 0;
    case 6:
        if (ctx->addr_n == ctx->len)
        {
            if (ctx->cs != byte)
            {
                ctx->step = 0;
                return -1;
            }
            ctx->step = 7;
            return 0;
        }
        ctx->cs += byte;
        
        ctx->addr_n++;
        return 0;
    case 7:
        if (0x16 != byte)
        {
            ctx->step = 0;
            return -1;
        }
        return 1;
    }

    return -1;
}

int fr_decode_645_11(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    mesg->from = pipeline->id;
    mesg->to = sta_rx_from_msta;
    mesg->ctrl = 0x91;
    mesg->id = 0x00;//��ʱû��
    mesg->len = 0;//������ĳ���
    //memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    //uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    //fifoBuf_getData(fifo, p + mesg->len, 4);
    uint32_t id = buf_to_uint32_lsb(p + mesg->len);
    mesg->len += 4;
    uint8_t byte;

    if (len == 0x04)
    {
        r = __read(id, p + mesg->len, 4);//����ID  todo
        if (r < 0)
        {
            r = 1;
            mesg->ctrl = 0xb1;
            p[mesg->len++] = -r;//todo ������ӳ��
        }
        else
        {
            mesg->len += r;
        }
    }
    else if (len == 0x09)//��ȡλͼ
    {
        if (id == 0x07fb0303)
        {
            fifoBuf_getData(fifo, p + mesg->len, 5);
            //SA = p[mesg->len];
            uint16_t n = (p[mesg->len + 1] | (p[mesg->len + 2] << 8));
            uint16_t m = (p[mesg->len + 3] | (p[mesg->len + 4] << 8));
            //todo �ж���Ϸ���
            r = (m + 7) / 8 - n / 8;//Ҫ��ȡ���ܰ���
            mesg->len += 5;
            memcpy(&p[mesg->len], instance->upgrade_info.bits + n / 8, r);
            mesg->len += r;
        }
    }
#endif
    return 1;
}

int fr_encode_645_11(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
//    struct fifo_t* fifo = pipeline->in_queue;
    //memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p= instance->apdu_cache + sizeof(struct message_t);//���������ʼ
    uint32_to_buf_lsb(mesg->id, p);
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, p + 4, mesg->len - 4);
    }

    return 1;
}
int fr_decode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t len = ctx->min_len - 12;
    uint8_t* p = instance->cache + sizeof(struct message_t);
    fifoBuf_getData(fifo, p, len);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);

    if (pipeline->id == 10)
    {
        struct tm t;
        array_0_to_time(&t, p, len);

        if (tm_diff(&instance->meter_rtc, &t) <= 5 * 60)
        {
            array_0_to_time(&instance->meter_rtc, p, len);
        }
    }
    else
    {
        mesg->ctrl = 0x08;
        mesg->id = 0x00000001;
        mesg->to = pid_msta;
        mesg->len = len + 4;
        mesg->payload = p;
        send_mesg(instance, pipeline, mesg);
    }
#endif
    return 0;
}

int fr_encode_645_86(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    uint32_to_buf_lsb(mesg->id, p);
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, p + 4, mesg->len - 4);
    }
    else
    {
        int r = __read(mesg->id, p + 4, 4);
        mesg->len += r;
    }
#endif
    return 1;
}

int fr_decode_645_86(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint32_t id;
    mesg->from = pipeline->id;
    mesg->ctrl = 0x06;
    mesg->len = 0;//������ĳ���
    memcpy(instance->cache, mesg, sizeof(struct message_t));

    uint8_t len = ctx->min_len - 12;
    uint8_t* p = instance->cache + sizeof(struct message_t);
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);
    mesg->len = 4;
    mesg->id = id;

    //__write(id, p + mesg->len, len - 4);
    p[mesg->len++] = 0x00;
#endif
    return 1;
}

#if 0
int fr_encode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p = instance->cache + sizeof(struct message_t);
    mesg->len -= 4;//�㲥��ʱ  û�����ֽ�ID
    fifoBuf_getData(pipeline->in_queue, p, mesg->len);
    return 1;
}

int fr_encode_645_14(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p = instance->cache + sizeof(struct message_t);
    uint32_to_buf_lsb(mesg->id, p);
    fifoBuf_getData(pipeline->in_queue, p + 4, mesg->len - 4);
    return 1;
}

int fr_decode_645_14(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    mesg->from = pipeline->id;
    mesg->to = sta_rx_from_msta;
    mesg->ctrl = 0x91;
    mesg->id = 0x00;
    mesg->len = 0;
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint32_t id;
    uint32_t seq;
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);

    if (id == 0x07fb0400)//�ļ�����
    {
        //4�ֽ�:FC0FC1FC2FC3 ����
        if (((instance->upgrade_info.info >> 7) == 1) &&
            instance->upgrade_info.pid != 0 &&
            instance->upgrade_info.pid != pipeline->id)
        {
            return 0;//���ͨ����������
        }

        //4�ֽ�:DA
        //4�ֽ�:PAP0P1P2  ����Ȩ�޼�������
        //4�ֽ�:C0C1C2C3  �����ߴ���
        //4�ֽ�:FC
        seq = buf_to_uint32_lsb(p + 12);
        if (seq > upgrade_setsize * 8)
        {
            return 0;
        }

        if (bit_isset(seq, instance->upgrade_info.bits))
        {
            //�ж��Ƿ�д����  �����漰������
            //����д����  �Ͳ�д��
            return 0;
        }

        //__write(id, p + 12, len - 12);//���δ�����ļ�����

        bit_set(seq, instance->upgrade_info.bits);
        instance->upgrade_info.info |= 1 << 7;//������
        instance->upgrade_info.pid = pipeline->id;
    }
    else if (id == 0x07fb0201)//�·�֤��
    {
        //__write(id, p + 12, len - 12);
    }
    else if (id == 0x07fb0202)//�·��������򼤻�ʱ��
    {
        //__write(id, p + mesg->len, len - 4);
    }
    else
    {
        //__write(id, p + mesg->len, len - 4);
    }

    mesg->ctrl = 0x94;
    p[mesg->len] = 0x00;
    return 1;
}

int fr_645_decode_91_from_meter(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint32_t id;

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);

    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);

    mesg->to = pid_msta;
    mesg->ctrl = 0x91;
    mesg->id = id;
    mesg->len = len + 4;
    mesg->payload = p+4;
    if (len == 5)
    {
        // return 1;//������ todo
    }
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    send_mesg(instance, pipeline, mesg);
    return 0;
}

int fr_encode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p = instance->cache + sizeof(struct message_t);
    mesg->len = 0;
    p[mesg->len] = 0;
    return 1;
}

int fr_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    mesg->from = pipeline->id;
    mesg->to = msta_rx_from_meter;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = 0;//
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    n_assert(len == 0);
    uint8_t* p = instance->cache + sizeof(struct message_t);

    r = __read(mesg->id, p, 6);
    mesg->len += r;
    return 1;
}

int fr_msta_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    mesg->from = pipeline->id;
    mesg->to = msta_tx_to_sta;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = 0;//
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    n_assert(len == 0);
    uint8_t* p = instance->cache + sizeof(struct message_t);
    extern struct rtc_context_t msta_context;
    memcpy(p, &msta_context.addr, 6); //msta_context.addr  ���ϵ�ʱ�����Ļ�����ַ
    r = 6;
    mesg->len += r;
    return 1;
}

int fr_645_decode_93_from_meter(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);
    mesg->to = pid_msta;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = len + 4;
    mesg->payload = p;
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    send_mesg(instance, pipeline, mesg);
    return 0;
}

int fr_645_decode_91(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint32_t id;

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);
    mesg->len = 4;

    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);
    //__write(id, p + mesg->len, len - 4);

    return 0;
}
#endif


int fr_pack_645_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    ctx->cs = 0;
    ctx->tx_len = 0;
    uint8_t buf[6];
    

    for(int i = 0; i < 4; i ++){
        fifoBuf_putByte(fifo, 0xfe);
    }
    append_byte(fifo, 0x68, __update_cs, ctx);
    //todo ��ַת��
    if (mesg->ctrl == 0x08)
    {
        memset(buf, 0x99, 6);
    }
    else if (mesg->ctrl == 0x13)
    {
        memset(buf, 0xAA, 6);
    }
    else
    {
        memcpy(buf, mbase_cb->tsa, 6);
        //memcpy(buf, &msta_context.addr, 6);
    }
    append_data(fifo, buf, 6, __update_cs, ctx);
    append_byte(fifo, 0x68, __update_cs, ctx);
    return 1;
}

int fr_pack_645_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint8_t len = mesg->len; 

    if ((ctx->segment_tx) && (ctx->segment_tx_no != 0)) //后续帧情况下 发帧序号
    {
        len = mesg->len + 1;
    }


    append_byte(fifo, mesg->ctrl, __update_cs, ctx);
    append_byte(fifo, len, __update_cs, ctx);

    if(mesg->len)
        fr_pack_645_apdu(instance, pipeline, mesg);
    
    //append_data(fifo, ctx->apdu_tx, mesg->len, __update_scrambler_and_cs, ctx);
    
    if ((ctx->segment_tx)&&(ctx->segment_tx_no != 0)) //后续帧情况下 发帧序号
    {
        append_data(fifo, &ctx->segment_tx_no, 1, __update_scrambler_and_cs, ctx);
    }

    return 1;
}

int fr_pack_645_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;
    fifoBuf_putByte(fifo, ctx->cs);
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len += 2;
    return 1;
}


int fr_rx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;

    uint8_t cs = fifoBuf_getByte(ififo);
    n_assert(ctx->cs == cs);
    
    uint8_t tail = fifoBuf_getByte(ififo);
    n_assert(tail == 0x16);

    fr_pack_645_head(instance, pipeline, mesg);
    fr_pack_645_data(instance, pipeline, mesg);
    fr_pack_645_tail(instance, pipeline, mesg);
    return 0;
}

int fr_pack_645_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
//    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    return 1;
}

unsigned char Addr_9_9( unsigned char* Addr )
{
	unsigned char i;

	for( i=0;i<6;i++ )
	{
		if( *(Addr+i) != 0x99 ) return 1;
	}
	
	return 0;
}

unsigned char Addr_A_A( unsigned char* Addr ) 
{
	unsigned char i;
    
	for( i=0;i<6;i++ )
	{
		if( *(Addr+i) != 0xAA ) return 1;
	}
	return 0;
}

unsigned char Cmp_MeterAddr( struct instance_t* instance, unsigned char* Addr, unsigned char mode )
{
	unsigned char i;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    if( mode == 1 )
	{
		for( i=0;i<6;i++ )
		{
	    	if( *(Addr+5-i) == 0xAA )
	    		*(Addr+5-i) = mbase_cb->tsa[5-i];
			else
				break;
		}
	}
	if( memcmp( Addr, mbase_cb->tsa, 6 ) == 0 )
	{
		return 0;
	}
	else
		return 1;
}


unsigned char Password_Check(struct instance_t* instance, unsigned char* Pass )
{
    uint32_t s_index = 0;
    uint8_t Buf[4]={0,};
	
    if(*Pass == 0x02)
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_02Password);
    else if(*Pass == 0x04)
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_04Password);
    
    nd_get_index(instance, s_index, (uint8_t*)&Buf[0], 4);
    if( memcmp( Pass+1, Buf+1, 3 ) == 0 )
        return 0;
    else
        return 1;
}

unsigned char Cmp_Password(struct instance_t* instance, unsigned char* Addr )
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    if((mbase_cb->factor_mode == true ))
    {
        return 0; 
    }
    
	if( (*Addr == 0x02) && (mbase_cb->Pass02Err_Cnt < 3) )
	{
		if( Password_Check( instance, Addr ) == 1 )
		{
			mbase_cb->Pass02Err_Cnt++;
			return 1;
		}
		else
		{
			mbase_cb->Pass02Err_Cnt = 0;
			return 0;
		}
	}
	else if( (*Addr == 0x04) && (mbase_cb->Pass04Err_Cnt < 3) )
	{
		if( Password_Check( instance, Addr ) == 1 )
		{
			mbase_cb->Pass04Err_Cnt++;
			return 1;
		}
		else
		{
			mbase_cb->Pass04Err_Cnt = 0;
			return 0;
		}
	}
	else
		return 1;
}
int __fr_segment_tx_645_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t type = 0;
    uint8_t segment_no = 0;
    uint8_t segment = 0;
    uint8_t data[10] ={0};
    uint8_t data_id[4] = {0x01, 0x15, 0x00, 0x04}; 
    
    if((ctx->ctrl == 0x12) && (ctx->segment_tx)){
        //fifoBuf_getData(fifo, apdu_rx,  2);
        //handle_scrambler(instance, pipeline, mesg);

        if (ctx->report_stat_follow_flag)
        {
            memcpy((unsigned char*)&data[0], data_id, 4);
        }
        else
        {
            memcpy((unsigned char*)&data[0], apdu_rx+10, 4);
        }
        segment_no = apdu_rx[10 + 4];
        
        
        if(segment_no == (ctx->segment_tx_no + 1))
        {
            ctx->segment_tx_no++;
            ctx->segment_tx = 1;
            if (fifoBuf_getUsed(&ctx->apdu)  >= 0)
            {
                mesg->len = fifoBuf_getUsed(&ctx->apdu);
                if (mesg->len > (MAX_READ_DATAFILED_LEN - 1))//后续帧  // 1 字节帧序号
                {   
                    mesg->len = (MAX_READ_DATAFILED_LEN - 1);
                    mesg->ctrl = ctx->ctrl | FOLLOW_FRAM_FLAG;
                }
                else //结束
                {
                    //ctx->segment_tx_no = 0;
                    //ctx->segment_tx = 1;
                }

                mesg->ctrl = ctx->ctrl | 0x80; 
                
                fifoBuf_update_rd(&ctx->apdu, 4, 0);          
                mesg->len+=4;
                memcpy(ctx->apdu.buf_ptr + ctx->apdu.rd, data, 4);
                
                fr_rx_645_end(instance, pipeline, mesg);
            }

            return 1;
        }
        else
        {
            ctx->segment_tx_no = 0; //异常应答
            ctx->segment_tx = 0;
            ctx->report_stat_follow_flag = 0;
            mesg->ctrl = ctx->ctrl | 0xC0;
            mesg->len = 1;
            *ctx->apdu_tx = 1+0x33;
            fr_rx_645_end(instance, pipeline, mesg);
        }
        
    }
    ctx->segment_tx = 0;
    ctx->segment_tx_no = 0;
    ctx->report_stat_follow_flag = 0;
    return 0;
}

int fr_gatekeeper_645_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    int8_t ERRBYTE=0;//data[8]={0}, 
    uint16_t tx_len = 0, RemotCtrResult=0;
    uint32_t ID=0,s_index=0;
    uint8_t buffer[10] = {0};
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;

//    int err_no = 0;

    if(0 == fr_pack_645_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    
    switch(pipeline->id)
    {
        case pid_ir_rx_698: 
            ctx->channel = chl_ir_698;
            break;
        case pid_rx_698: 
            ctx->channel = chl_RS485_1_698;
            break;
        case pid_plc_rx_698: 
            ctx->channel = chl_PLC_698;
            break;
        case pid_4852_rx_698: 
            ctx->channel = chl_RS485_2_698;
            break;
        default:
            ctx->channel = chl_ir_698;
    }
    
    if((ctx->channel == chl_ir_698) || (ctx->channel == chl_RS485_1_698))
    {
        if(ctx->channel == chl_ir_698)  mbase_cb->BackLightTmr = 2* (mbase_cb->lcd_autodis_list_num) * (mbase_cb->lcd_autodis_keep_sec);
        mbase_cb->commdisp_ir485 = 2;
    }
    else if(ctx->channel == chl_PLC_698)
    {
        mbase_cb->commdisp_plc = 2;
    }


    ctx->apdu_rx_offset = 0;
    ctx->apdu_rx_len = ctx->len;
    if(ctx->ver)    
        fifoBuf_getData(fifo, apdu_rx,  ctx->apdu_rx_len+11);
    else
        fifoBuf_getData(fifo, apdu_rx,  ctx->apdu_rx_len+10);


    if ((ctx->ctrl != 0x12) || (ctx->apdu.buf_ptr == NULL))// 待调整 ？？ fifo初始化为何放此处？  暂时改为分帧情况下 不初始化
    {
        fifoBuf_init(&ctx->apdu, ctx->apdu_tx, max_apdu_cache_645);
        ctx->segment_tx_no = 0;
        ctx->segment_tx = 0;
        ctx->report_stat_follow_flag = 0;
    }
       
    for(int i = 0; i < ctx->apdu_rx_len; i++){
        apdu_rx[i+10] = apdu_rx[i+10] - 0x33;
    }

     ndv_memcopy(buffer, apdu_rx+1 , 6); 
    //地址信息
    if(( Cmp_MeterAddr( instance, buffer , 0 ) == 0 ))
    {
        ctx->SA = SA_TYPE_SINGLE645;  //实际地址
    }
    else if(( Cmp_MeterAddr( instance, buffer , 1 ) == 0 ))
    {
        ctx->SA = SA_TYPE_GENERAL645;  //通配 
    }
    else if((Addr_9_9( buffer ) == 0))
    {
        ctx->SA = SA_TYPE_BROADECAST645;  //  99 广播
    }
    else
    {
        return -1; //不响应
    }


    memcpy((unsigned char*)&ID, apdu_rx+10, 4);
    
    switch (ctx->ctrl)
    {
        case 0x03:  //安全认证
            ERRBYTE = safe_Authen03_645(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;
        case 0x08:  //广播校时
            ERRBYTE = Broadcast_timeadj(instance, ctx->channel, apdu_rx);//无需应答
            break;
        case 0x11:  //读数据
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 1 ) == 0 )
            {
                ERRBYTE = Comm_Read(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        case 0x12: //后续帧
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 0 ) == 0 )
            {
                if(__fr_segment_tx_645_handle(instance, pipeline, mesg))
                {
                    return -1; //不应答
                }
            }

            return 0;
            break;
        case 0x13://读地址
            if((Addr_A_A( (unsigned char*)&apdu_rx[1] ) == 0) && (apdu_rx[9] == 0))
            {
                memcpy(ctx->apdu_tx, mbase_cb->tsa, 6);
                tx_len = 6;
                ERRBYTE = 0;
            }
            else
                return -1;
            break;
        case 0x14:  //写数据
            memcpy((unsigned char*)&ctx->password[0], apdu_rx + 10 + 4, 4);//拷贝密码
            memcpy((unsigned char*)mbase_cb->handler_code, apdu_rx + 10 + 4 + 4, 4); 
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 0 ) == 0 )
            {
                ERRBYTE = Comm_Write(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
                if(ERRBYTE == 0)
                {
                    trigger_MeterWr645_record(instance,   ID, 0);
                    tx_len = 0;
                }
                else
                {
                    tx_len = 1;
                    *ctx->apdu_tx = ERRBYTE;
                }
            }
            else if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 1 ) == 0 )   //支持全A进厂内
            {
                
                if(ID == 0xA8000100)
                {
                    ERRBYTE = Comm_Write(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
                    if(ERRBYTE == 0)
                    {
                        trigger_MeterWr645_record(instance,   ID, 0);
                        tx_len = 0;
                    }
                    else
                    {
                        tx_len = 1;
                        *ctx->apdu_tx = ERRBYTE;
                    }
                }
                else
                    return -1;
            }
            else
                return -1;
            break;
        case 0x15:  //写通信地址
            ERRBYTE = meteraddr_write(instance, apdu_rx);
            break;
        case 0x16:  //冻结
            ERRBYTE = ctl_freeze(instance, ctx->channel, apdu_rx);
            break;
        case 0x17:  //修改波特率
            ERRBYTE = chg_baut(instance, apdu_rx);
            break;
        case 0x1A:  //电表清零
            memcpy((unsigned char*)&ctx->password[0], apdu_rx + 10, 4);//拷贝密码
            ERRBYTE = dl645_meter_clear(instance, apdu_rx, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;
        case 0x1B:  //事件清零
            memcpy((unsigned char*)&ctx->password[0], apdu_rx + 10, 4);//拷贝密码
            ERRBYTE = dl645_event_clear(instance, apdu_rx, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;
        case 0x1C:  //远程控制
            memcpy((unsigned char*)&ctx->password[0], apdu_rx + 10, 4);//拷贝密码

            ERRBYTE = CommRemoteCtl_Process(instance,apdu_rx, ctx->apdu_tx, (uint8_t*)&tx_len, ctx,  RemotCtrResult);
            break;
        case 0x1D:  //多功能口控制
            ERRBYTE = MulPort_set(instance, ctx->channel, apdu_rx);
            break;
        case 0x1F:  //飞腾扩展
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 0 ) == 0 )
            {
                ERRBYTE = Comm_Other(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        case 0xfa:
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 0 ) == 0 )
            {
                ERRBYTE = upgrade_file_handle(instance, apdu_rx+11,  ctx->apdu_rx_len, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        case 0x60:  //ft校表读命令
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 1 ) == 0 )
            {
                ERRBYTE = ft_adj_get_meter(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
            break;
        case 0x61:  //ft校表写命令
            if( Cmp_MeterAddr( instance, (unsigned char*)&apdu_rx[1], 1 ) == 0 )
            {
                ERRBYTE = ft_adj_set_meter(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        default:
            ERRBYTE = RS_ERR_ID;
            break;
    }
    
    if(ERRBYTE == 0)
    {
        mesg->ctrl = ctx->ctrl | 0x80;
    }
    else if(ERRBYTE == -1)  //不应答
    {
        return -1;
    }
    else
    {
        mesg->ctrl = ctx->ctrl | 0xC0;
        if((ctx->ctrl == 0x1C) && (*(apdu_rx+10) == 0x01) && (*(apdu_rx+10) == 0x98))   //增加新标准应答方式
        {
            tx_len = 2;
            *ctx->apdu_tx = (uint8_t)RemotCtrResult;
            *(ctx->apdu_tx+1) = (uint8_t)((RemotCtrResult & 0xFF00)>>8);
        }
        else if ((ctx->ctrl == 0x03)) //长度及错误字内部已处理
        {
                
        }
        else 
        {
            tx_len = 1;
            *ctx->apdu_tx = ERRBYTE;
        }
    }
            
    tx_len = tx_len;
    ctx->apdu.wr = tx_len;
    ctx->ctrl = mesg->ctrl;
    if (fifoBuf_getUsed(&ctx->apdu)  >= 0){
        mesg->len = fifoBuf_getUsed(&ctx->apdu);
        if (mesg->len > MAX_READ_DATAFILED_LEN){   //后续帧
            mesg->len = MAX_READ_DATAFILED_LEN;
            mesg->ctrl |= FOLLOW_FRAM_FLAG;
            ctx->segment_tx_no = 0;
            ctx->segment_tx = 1;
        }
        else
        {
            ctx->segment_tx_no = 0;
            ctx->segment_tx = 0;

            ret = report_push_data_judge(instance, &ctx->apdu_tx[tx_len],  ctx);
            if(ret > 0)
            {
                ctx->apdu.wr += ret;
                ctx->segment_tx = 1;
                ctx->report_stat_follow_flag = 1;
                mesg->ctrl |= FOLLOW_FRAM_FLAG;
            }
        }
        
        fr_rx_645_end(instance, pipeline, mesg);
    }
    return 0;
    
}

int fr_rx_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx->step = 0;
    uint16_t rd = fifo->rd;
    int r = ringbuffer_filter(fifo, __matcher_645, ctx);
    if (r > 0){
        n_assert(r == ctx->min_len);
#ifdef WIN32
        log_hex(fifo->buf_ptr + fifo->rd, fifo->wr - fifo->rd);
#endif
        return 1;
    }else{
        fifo->rd = rd;
    }
    return r;
    
}



int fr_tx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;

    //todo  ���Ƿ�Ҫ����
    fr_pack_645_head(instance, pipeline, mesg);
    fr_pack_645_data(instance, pipeline, mesg);
    fr_pack_645_tail(instance, pipeline, mesg);
    //ʹ�ܷ���
#ifdef VERBOSE
    if (ctx->tx_len > 0)
    {
        uint8_t buf[256];
        printf("                   {%s,send", get_pipeline_name(instance, pipeline->id));
        uint16_t len = fifoBuf_getDataPeek(pipeline->out_queue, buf, ctx->tx_len);
        printf(",");
        for (size_t i = 0; i < len; i++)
        {
            printf("%02x", buf[i]);
        }

        printf("}\n");
    }
#endif

    return 0;
}

int fr_tx_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
//    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
//    uint16_t len = 0;

    if (recv_mesg_v2(instance, pipeline, mesg, NULL, 0))
    {
        return 1;
    }

    return -1;
}

