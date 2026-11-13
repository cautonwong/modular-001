#include "type_def.h"
#include <stdbool.h>

#ifndef n_assert
#define n_assert(x) ((void)0)
#endif
#include "application/adapters/dl645/fr_645_env.h"
#include "application/ports/fifo.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "gs_record.h"
#include "645id_table.h"
#include "fr_645_report.h"
#include <string.h>
#include "application/adapters/dl645/ctl_other.h"
#include "application/adapters/dl645/fr_645_matcher.h"
#include "application/adapters/dl645/fr_645_pack.h"
#include "application/adapters/dl645/fr_645_data.h"
#include "application/adapters/dl645/fr_645_addr.h"
#include "application/adapters/dl645/fr_645_password.h"
#include "application/ports/resource_access.h"
struct instance_t;
struct pipeline_t;
#define FR_645_PID_MSTA 12
#define FR_645_PID_RX_698 14
#define FR_645_PID_TX_698 15
#define FR_645_PID_PLC_RX 24
#define FR_645_PID_PLC_TX 25
#define FR_645_PID_IR_RX 26
#define FR_645_PID_4852_RX 28
#define FR_645_PID_RX_645 60
#define FR_645_CHL_IR 0
#define FR_645_CHL_RS485_1 1
#define FR_645_CHL_PLC 2
#define FR_645_CHL_RS485_2 3
#define FR_645_PASSWORD_EN 0x0002
#define eleof(x) (sizeof(x) / sizeof((x)[0]))


extern signed char ft_adj_set_meter(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern signed char Broadcast_timeadj(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char MulPort_set(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char meteraddr_write(struct instance_t* instance, uint8_t* p_rx);
extern signed char dl645_event_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern signed char dl645_meter_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern signed char Comm_Read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len,struct context_645_t* ctx);
extern signed char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len, struct context_645_t* ctx);
extern int8_t safe_Authen03_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern int8_t dlt645_image_transfer(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern int8_t dlt645_image_read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);

static uint8_t uint32_to_buf_lsb(uint32_t data, uint8_t *buf)
{
    buf[0] = (uint8_t)data;
    buf[1] = (uint8_t)(data >> 8);
    buf[2] = (uint8_t)(data >> 16);
    buf[3] = (uint8_t)(data >> 24);
    return 4;
}

static uint16_t fr645_fifo_used(const fr_645_fifo_t *fifo)
{
    uint16_t rd = fifo->rd;
    uint16_t wr = fifo->wr;
    uint16_t size = fifo->buf_size;

    return wr < rd ? (uint16_t)((size - rd) + wr) : (uint16_t)(wr - rd);
}

static void fr645_fifo_init(fr_645_fifo_t *fifo, const void *buffer,
                            uint16_t size)
{
    fifo->buf_ptr = (uint8_t *)buffer;
    fifo->rd = 0;
    fifo->wr = 0;
    fifo->buf_size = size;
}

static void fr645_fifo_update_rd(fr_645_fifo_t *fifo, uint16_t len,
                                 uint8_t forward)
{
    uint16_t rd = fifo->rd;

    if (forward)
    {
        rd = (uint16_t)(rd + len);
        if (rd >= fifo->buf_size)
        {
            rd = (uint16_t)(rd - fifo->buf_size);
        }
    }
    else if (rd >= len)
    {
        rd = (uint16_t)(rd - len);
    }
    else
    {
        rd = (uint16_t)(fifo->buf_size - len - rd);
    }
    fifo->rd = rd;
}










int fr_decode_645_11(struct instance_t*, struct pipeline_t*, nd_dl645_msg_t*)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    int r = 0;
    mesg->from = nd_fr645_pipeline_id(pipeline);
    mesg->to = sta_rx_from_msta;
    mesg->ctrl = 0x91;
    mesg->id = 0x00;//��ʱû��
    mesg->len = 0;//������ĳ���
    //memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    //uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
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

int fr_encode_645_11(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
//    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
//    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    //memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t* p= nd_fr645_apdu_cache(instance) + sizeof(nd_dl645_msg_t);//���������ʼ
    uint32_to_buf_lsb(mesg->id, p);
    if (mesg->len > 4)
    {
        fifoBuf_getData((struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline), p + 4, mesg->len - 4);
    }

    return 1;
}
int fr_decode_645_08(struct instance_t*, struct pipeline_t*, nd_dl645_msg_t*)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    uint8_t len = ctx->min_len - 12;
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
    fifoBuf_getData(fifo, p, len);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);

    if (nd_fr645_pipeline_id(pipeline) == 10)
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
        mesg->to = FR_645_PID_MSTA;
        mesg->len = len + 4;
        mesg->payload = p;
        if ((ctx != NULL) && (ctx->forward != NULL)) { ctx->forward(instance, pipeline, mesg); }
    }
#endif
    return 0;
}

int fr_encode_645_86(struct instance_t*, struct pipeline_t*, nd_dl645_msg_t*)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
    uint32_to_buf_lsb(mesg->id, p);
    if (mesg->len > 4)
    {
        fifoBuf_getData((struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline), p + 4, mesg->len - 4);
    }
    else
    {
        int r = __read(mesg->id, p + 4, 4);
        mesg->len += r;
    }
#endif
    return 1;
}

int fr_decode_645_86(struct instance_t*, struct pipeline_t*, nd_dl645_msg_t*)
{
#if 0
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    uint32_t id;
    mesg->from = nd_fr645_pipeline_id(pipeline);
    mesg->ctrl = 0x06;
    mesg->len = 0;//������ĳ���
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));

    uint8_t len = ctx->min_len - 12;
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
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
int fr_encode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
    mesg->len -= 4;//�㲥��ʱ  û�����ֽ�ID
    fifoBuf_getData((struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline), p, mesg->len);
    return 1;
}

int fr_encode_645_14(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
    uint32_to_buf_lsb(mesg->id, p);
    fifoBuf_getData((struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline), p + 4, mesg->len - 4);
    return 1;
}

int fr_decode_645_14(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    int r = 0;
    mesg->from = nd_fr645_pipeline_id(pipeline);
    mesg->to = sta_rx_from_msta;
    mesg->ctrl = 0x91;
    mesg->id = 0x00;
    mesg->len = 0;
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint32_t id;
    uint32_t seq;
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);

    if (id == 0x07fb0400)//�ļ�����
    {
        //4�ֽ�:FC0FC1FC2FC3 ����
        if (((instance->upgrade_info.info >> 7) == 1) &&
            instance->upgrade_info.pid != 0 &&
            instance->upgrade_info.pid != nd_fr645_pipeline_id(pipeline))
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
        instance->upgrade_info.pid = nd_fr645_pipeline_id(pipeline);
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

int fr_645_decode_91_from_meter(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    uint32_t id;

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);

    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);

    (void)id;
    fr_645_parse_91_data(p, len, mesg, FR_645_PID_MSTA);
    if (len == 5)
    {
        // return 1;//������ todo
    }
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    if ((ctx != NULL) && (ctx->forward != NULL)) { ctx->forward(instance, pipeline, mesg); }
    return 0;
}

int fr_encode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
    mesg->len = 0;
    p[mesg->len] = 0;
    return 1;
}

int fr_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    int r = 0;
    mesg->from = nd_fr645_pipeline_id(pipeline);
    mesg->to = msta_rx_from_meter;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = 0;//
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    n_assert(len == 0);
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);

    r = __read(mesg->id, p, 6);
    mesg->len += r;
    return 1;
}

int fr_msta_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    int r = 0;
    mesg->from = nd_fr645_pipeline_id(pipeline);
    mesg->to = msta_tx_to_sta;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = 0;//
    memcpy(instance->cache, mesg, sizeof(nd_dl645_msg_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    n_assert(len == 0);
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);
    extern struct rtc_context_t msta_context;
    memcpy(p, &msta_context.addr, 6); //msta_context.addr  ���ϵ�ʱ�����Ļ�����ַ
    r = 6;
    mesg->len += r;
    return 1;
}

int fr_645_decode_93_from_meter(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);
    mesg->to = FR_645_PID_MSTA;
    mesg->ctrl = 0x93;
    mesg->id = 0x04000401;
    mesg->len = len + 4;
    mesg->payload = p;
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    if ((ctx != NULL) && (ctx->forward != NULL)) { ctx->forward(instance, pipeline, mesg); }
    return 0;
}

int fr_645_decode_91(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    uint32_t id;

    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(nd_dl645_msg_t);//���������ʼ
    fifoBuf_getData(fifo, p, len);
    id = buf_to_uint32_lsb(p);
    mesg->len = 4;

    fifoBuf_getByte(fifo); fifoBuf_getByte(fifo);
    //__write(id, p + mesg->len, len - 4);

    return 0;
}
#endif









int fr_rx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{

    //uint8_t cs = fifoBuf_getByte(ififo);
    //n_assert(ctx->cs == cs);

    //uint8_t tail = fifoBuf_getByte(ififo);
    //n_assert(tail == 0x16);

    fr_pack_645_head(instance, pipeline, mesg);
    fr_pack_645_data(instance, pipeline, mesg);
    fr_pack_645_tail(instance, pipeline, mesg);
    return 0;
}

int fr_pack_645_check(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    (void)pipeline;
    (void)mesg;
    return nd_fr645_mbase_valid(instance) ? 1 : -1;
}



unsigned char Password_Check(struct instance_t* instance, unsigned char* Pass )
{
    uint32_t s_index = 0;
    uint8_t Buf[4]={0,};

    s_index = nd_fr645_password_index(instance, *Pass);

    nd_get_index(instance, s_index, (uint8_t*)&Buf[0], 4);
    return fr_645_password_matches(Pass, Buf);
}
const uint32_t CODE dl645_ir_readable_id[] =
{
    0x04000401,
    0x04000402,
    0x04800004,
    0x0400010C,
    0x04000101,
    0x04000102, /*时间*/
    0x00900200, /*当前剩余金额*/
};

//红外权限判断
unsigned char Dl645_IR_chl_Check(struct instance_t* instance, uint32_t id, uint8_t chl)
{
    uint8_t i;
    if(chl != FR_645_CHL_IR)
    {
        return true;
    }

    if(((*nd_fr645_metertype(instance) & FR_645_PASSWORD_EN) == 0) && (*nd_fr645_infrared_auth_timeout(instance) == 0) && (*nd_fr645_factor_mode(instance) == false))//红外通道必须在红外认证有效期间进行操作。
    {

        if (0x00000000 == (id & 0xFF0000FF))
        {
            return true;
        }
        if(id != 0xFFFFFFFF)
        {
            for (i = 0; i < eleof(dl645_ir_readable_id); i++)
            {
                if((id) == dl645_ir_readable_id[i])
                {
                    return true;
                }
            }
        }

        return false;
    }
    else
    {
        return true;
    }

}



unsigned char Cmp_Password(struct instance_t* instance, unsigned char* Addr )
{

    if((*nd_fr645_factor_mode(instance) == true ))
    {
        return 0;
    }

	if( (*Addr == 0x02) && (*nd_fr645_pass02_err_cnt(instance) < 3) )
	{
		if( Password_Check( instance, Addr ) == 1 )
		{
			(*nd_fr645_pass02_err_cnt(instance))++;
			return 1;
		}
		else
		{
			*nd_fr645_pass02_err_cnt(instance) = 0;
			return 0;
		}
	}
	else if( (*Addr == 0x04) && (*nd_fr645_pass04_err_cnt(instance) < 3) )
	{
		if( Password_Check( instance, Addr ) == 1 )
		{
			(*nd_fr645_pass04_err_cnt(instance))++;
			return 1;
		}
		else
		{
			*nd_fr645_pass04_err_cnt(instance) = 0;
			return 0;
		}
	}
	else
		return 1;
}
int __fr_segment_tx_645_handle(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t segment_no = 0;
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
            mesg->len = fr645_fifo_used(&ctx->apdu);
            mesg->ctrl =  ctx->ctrl | 0x80;
            if (mesg->len > ctx->follow_segment_MaxLen)//后续帧
            {
                mesg->len = ctx->follow_segment_MaxLen;
                mesg->ctrl |= FOLLOW_FRAM_FLAG;
            }
            else //结束
            {
                //ctx->segment_tx_no = 0;
                //ctx->segment_tx = 1;
            }

            fr645_fifo_update_rd(&ctx->apdu, 4, 0);
            mesg->len+=4;
            memcpy(ctx->apdu.buf_ptr + ctx->apdu.rd, data, 4);

            fr_rx_645_end(instance, pipeline, mesg);

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
    ctx->first_segment_MaxLen = MAX_READ_DATAFILED_LEN;
    ctx->follow_segment_MaxLen = MAX_READ_DATAFILED_LEN;

    return 0;
}

int fr_gatekeeper_645_server(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    int8_t ERRBYTE=0;//data[8]={0},
    uint16_t tx_len = 0, RemotCtrResult=0;
uint32_t ID=0;
    uint8_t buffer[10] = {0};
    int ret = 0;

//    int err_no = 0;

    if(0 == fr_pack_645_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }

    switch(nd_fr645_pipeline_id(pipeline))
    {
        case FR_645_PID_IR_RX:
            ctx->channel = FR_645_CHL_IR;
            break;
        case FR_645_PID_RX_698:
        case FR_645_PID_RX_645:
            ctx->channel = FR_645_CHL_RS485_1;
            break;
        case FR_645_PID_PLC_RX:
            ctx->channel = FR_645_CHL_PLC;
            break;
        case FR_645_PID_4852_RX:
            ctx->channel = FR_645_CHL_RS485_2;
            break;
        default:
            ctx->channel = FR_645_CHL_IR;
    }

    if((ctx->channel == FR_645_CHL_IR) || (ctx->channel == FR_645_CHL_RS485_1))
    {
        if(ctx->channel == FR_645_CHL_IR)  *nd_fr645_backlight_tmr(instance) = 2* (*nd_fr645_lcd_autodis_list_num(instance)) * (*nd_fr645_lcd_autodis_keep_sec(instance));
        *nd_fr645_commdisp_ir485(instance) = 2;
    }
    else if(ctx->channel == FR_645_CHL_PLC)
    {
        *nd_fr645_commdisp_plc(instance) = 2;
    }


    ctx->apdu_rx_offset = 0;
    ctx->apdu_rx_len = ctx->len;

    fifoBuf_getData(fifo, apdu_rx, ctx->min_len);


    ctx->first_segment_MaxLen = MAX_READ_DATAFILED_LEN;
    //ctx->follow_segment_MaxLen = MAX_READ_DATAFILED_LEN;

    if ((ctx->ctrl != 0x12) || (ctx->apdu.buf_ptr == NULL))
    {
        fr645_fifo_init(&ctx->apdu, ctx->apdu_tx, max_apdu_cache_645);
        ctx->segment_tx_no = 0;
        ctx->segment_tx = 0;
        ctx->report_stat_follow_flag = 0;

        ctx->first_segment_MaxLen = MAX_READ_DATAFILED_LEN;
        ctx->follow_segment_MaxLen = MAX_READ_DATAFILED_LEN;
    }

    for(int i = 0; i < ctx->apdu_rx_len; i++){
        apdu_rx[i+10] = apdu_rx[i+10] - 0x33;
    }

     memcpy(buffer, apdu_rx + 1, 6);
    //地址信息
    if(( fr_645_addr_match(buffer, nd_fr645_tsa(instance), 0) == 0 ))
    {
        ctx->SA = SA_TYPE_SINGLE645;  //实际地址
    }
    else if(( fr_645_addr_match(buffer, nd_fr645_tsa(instance), 1) == 0 ))
    {
        ctx->SA = SA_TYPE_GENERAL645;  //通配
    }
    else if((fr_645_addr_all(buffer, 0x99) == 0))
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
        case 0x01:  //97读 升级
            ERRBYTE = dlt645_image_read(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;
        case 0x04:  //97写 升级
            ERRBYTE = dlt645_image_transfer(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;

        case 0x03:  //安全认证
            ERRBYTE = safe_Authen03_645(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            break;
        case 0x08:  //广播校时
            ERRBYTE = Broadcast_timeadj(instance, ctx->channel, apdu_rx);//无需应答
            break;
        case 0x11:  //读数据
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 1) == 0 )
            {
                ERRBYTE = Comm_Read(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len, ctx);
            }
            else
                return -1;
            break;
        case 0x12: //后续帧
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 0) == 0 )
            {
                if(__fr_segment_tx_645_handle(instance, pipeline, mesg))
                {
                    return -1; //不应答
                }
            }

            return 0;
            break;
        case 0x13://读地址
            if((fr_645_addr_all( (unsigned char*)&apdu_rx[1], 0xAA ) == 0) && (apdu_rx[9] == 0))
            {
                memcpy(ctx->apdu_tx, nd_fr645_tsa(instance), 6);
                tx_len = 6;
                ERRBYTE = 0;
            }
            else
                return -1;
            break;
        case 0x14:  //写数据
            memcpy((unsigned char*)&ctx->password[0], apdu_rx + 10 + 4, 4);//拷贝密码
            memcpy((unsigned char*)nd_fr645_handler_code(instance), apdu_rx + 10 + 4 + 4, 4);
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 0) == 0 )
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
            else if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 1) == 0 )   //支持全A进厂内
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
            ERRBYTE = chg_baut(instance, apdu_rx,  ctx);
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
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 0) == 0 )
            {
                ERRBYTE = Comm_Other(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        case 0xfa:
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 0) == 0 )
            {
                ERRBYTE = upgrade_file_handle(instance, apdu_rx+11,  ctx->apdu_rx_len, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
        case 0x60:  //ft校表读命令
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 1) == 0 )
            {
                ERRBYTE = ft_adj_get_meter(instance, apdu_rx+10, ctx->apdu_tx, (uint8_t*)&tx_len);
            }
            else
                return -1;
            break;
            break;
        case 0x61:  //ft校表写命令
            if( fr_645_addr_match((unsigned char*)&apdu_rx[1], nd_fr645_tsa(instance), 1) == 0 )
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
    mesg->len = fr645_fifo_used(&ctx->apdu);
    if (mesg->len > ctx->first_segment_MaxLen){   //后续帧
        mesg->len = ctx->first_segment_MaxLen ;
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
    return 0;

}

int fr_rx_645_entry(struct instance_t*, struct pipeline_t* pipeline, nd_dl645_msg_t*)
{
    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
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



int fr_tx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg)
{

    //todo  ���Ƿ�Ҫ����
    fr_pack_645_head(instance, pipeline, mesg);
    fr_pack_645_data(instance, pipeline, mesg);
    fr_pack_645_tail(instance, pipeline, mesg);
    //ʹ�ܷ���
#ifdef VERBOSE
    if (ctx->tx_len > 0)
    {
        uint8_t buf[256];
        printf("                   {%s,send", get_pipeline_name(instance, nd_fr645_pipeline_id(pipeline)));
        uint16_t len = fifoBuf_getDataPeek((struct fifo_t*)nd_fr645_pipeline_out_queue(pipeline), buf, ctx->tx_len);
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

int fr_tx_645_entry(struct instance_t*, struct pipeline_t*, nd_dl645_msg_t* mesg)
{
//    struct context_645_t* ctx = (struct context_645_t*)nd_fr645_pipeline_ctx(pipeline);
//    struct fifo_t* ififo = (struct fifo_t*)(struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline);
//    uint16_t len = 0;

    if (mesg != NULL)
    {
        return 1;
    }

    return -1;
}

