#include "application/adapters/dl645/fr_645_matcher.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "645id_table.h"

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
        //if (ctx->min_len > fifoBuf_getUsed((struct fifo_t*)nd_fr645_pipeline_in_queue(pipeline)))
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
