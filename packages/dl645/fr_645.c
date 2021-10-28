#include "nidavellir.h"
#define data_max_len 222

int __update_cs(struct context_645_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    ctx->cs += *byte;
    return 0;
}

int __update_scrambler_and_cs(struct context_645_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    *byte += 0x33;
    ctx->cs += *byte;
    return 0;
}

int __matcher_645(struct pipeline_t* pipeline, uint8_t* b)
{
    uint8_t byte = *b;
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;

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
        //todo  ���˿�����
        ctx->step = 3;
        return 0;
    case 3:
        if (byte > data_max_len)
        {
            ctx->step = 0;
            return -1;
        }
        ctx->len = byte;
        ctx->min_len = byte + 12;
        if (ctx->min_len > fifoBuf_getUsed(pipeline->in_queue))
        {
            return 2;//���Ȳ���  �´�����
        }
        ctx->cs += byte;
        ctx->addr_n = 0;
        ctx->step = 4;
        return 0;
    case 4:
        if (ctx->addr_n == ctx->min_len - 12)
        {
            if (ctx->cs != byte)
            {
                ctx->step = 0;
                return -1;
            }
            ctx->step = 6;
            return 0;
        }
        ctx->cs += byte;
        *b = byte - 0x33;
        ctx->addr_n++;
        return 0;
    case 6:
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
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    mesg->from = pipeline->id;
    mesg->to = sta_rx_from_msta;
    mesg->ctrl = 0x91;
    mesg->id = 0x00;//��ʱû��
    mesg->len = 0;//������ĳ���
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t len = ctx->min_len - 12;//�յ���������ĳ���
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    fifoBuf_getData(fifo, p + mesg->len, 4);
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

    return 1;
}

int fr_encode_645_11(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    memcpy(instance->cache, mesg, sizeof(struct message_t));
    uint8_t* p = instance->cache + sizeof(struct message_t);//���������ʼ
    uint32_to_buf_lsb(mesg->id, p);
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, p + 4, mesg->len - 4);
    }

    return 1;
}
int fr_decode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
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
        mesg->len = len + 4;
        mesg->to = pid_msta;
        send_mesg(instance, pipeline, mesg, p);
    }

    return 0;
}

int fr_encode_645_86(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
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
    return 1;
}

int fr_decode_645_86(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
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

    __write(id, p + mesg->len, len - 4);
    p[mesg->len++] = 0x00;
    return 1;
}


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

        __write(id, p + 12, len - 12);//���δ�����ļ�����

        bit_set(seq, instance->upgrade_info.bits);
        instance->upgrade_info.info |= 1 << 7;//������
        instance->upgrade_info.pid = pipeline->id;
    }
    else if (id == 0x07fb0201)//�·�֤��
    {
        __write(id, p + 12, len - 12);
    }
    else if (id == 0x07fb0202)//�·��������򼤻�ʱ��
    {
        __write(id, p + mesg->len, len - 4);
    }
    else
    {
        __write(id, p + mesg->len, len - 4);
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
    mesg->len = len;
    if (len == 5)
    {
        // return 1;//������ todo
    }
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    send_mesg(instance, pipeline, mesg, p + 4);
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
    assert(len == 0);
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
    assert(len == 0);
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
    //����ȫ���ض�����pl_mstaȥ����
    //ҪŪ���ض������  todo
    send_mesg(instance, pipeline, mesg, p);
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
    __write(id, p + mesg->len, len - 4);

    return 0;
}

int fr_pack_645_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->out_queue;
    ctx->cs = 0;
    ctx->tx_len = 0;
    uint8_t buf[6];
    fifoBuf_putByte(fifo, 0xfe);
    fifoBuf_putByte(fifo, 0xfe);
    fifoBuf_putByte(fifo, 0xfe);
    fifoBuf_putByte(fifo, 0xfe);
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
        memcpy(buf, ctx->addr, 6);
        //memcpy(buf, &msta_context.addr, 6);
    }
    append_data(fifo, buf, 6, __update_cs, ctx);
    append_byte(fifo, 0x68, __update_cs, ctx);
    return 1;
}

int fr_pack_645_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

    append_byte(fifo, mesg->ctrl, __update_cs, ctx);
    append_byte(fifo, mesg->len, __update_cs, ctx);
    append_data(fifo, instance->cache + sizeof(struct message_t),
        mesg->len, __update_scrambler_and_cs, ctx);

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

int fr_rx_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx->step = 0;

    int r = ringbuffer_filter(fifo, __matcher_645, pipeline);
    if (r > 0)
    {
#ifdef VERBOSE
        uint8_t buf[256];
        printf("                   {%s,recv", get_pipeline_name(instance, pipeline->id));
        uint16_t len = fifoBuf_getDataPeek(pipeline->in_queue, buf, ctx->min_len);
        printf(",");
        for (size_t i = 0; i < len; i++)
        {
            printf("%02x", buf[i]);
        }

        printf("}\n");
#endif
        return 1;
    }
    return 0;
}

int fr_rx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    struct message_t* n = (struct message_t*)instance->cache;

    fifoBuf_getByte(ififo);//fcs-0
    assert(fifoBuf_getByte(ififo) == 0x16);//0x16
    n->len = mesg->len;
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
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    uint16_t len = 0;

    if (recv_mesg_header(instance, pipeline, mesg))
    {
        return 1;
    }

    return -1;
}

