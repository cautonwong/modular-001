#include "application/adapters/dl698/fr_698_matcher.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "fr_698.h"

static uint16_t update_crc16(uint16_t init, uint8_t byte)
{
    uint16_t crc = (uint16_t)(init ^ byte);
    uint8_t i;

    for (i = 0; i < 8; i++) {
        crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0x8408u)
                         : (uint16_t)(crc >> 1);
    }
    return crc;
}

static uint16_t get_crc16(uint16_t cs)
{
    return (uint16_t)(cs ^ 0xFFFFu);
}

int __matcher_698(void* pctx, uint8_t* b)
{
    uint8_t byte = *b;
    struct context_698_t* ctx = pctx;

    switch (ctx->step)
    {
    case 0://帧头68判断
        if (byte != 0x68){
            ctx->err_n++;
            ctx->err_type = 1;
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
        if (ctx->min_len < 11){
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 2;
            return FRM_LEN_ERR1;
        }
        //if ((len < 11) || (len + 2 > *bytes) || (0x16 != p[len + 1]))
        ctx->step = 3;
        return FRM_OK;
    case 3:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if ( (FC_CODE(byte) != 1) && (FC_CODE(byte) != 3)){
            ctx->step = 0;
            ctx->err_type = 3;
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
        if(SA_LOGIC_TYPE(ctx->SA)){
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
        if (ctx->index == SA_LEN(ctx->SA) + 1){
            ctx->CA = byte;
            ctx->step = 7;
            //uint8_t tmpbuf[32] = {0};
            //mem_swap_cpy(tmpbuf, ctx->addr, ctx->index);
            //memcpy(ctx->addr, tmpbuf, ctx->index);
        }else{
            ctx->addr[ctx->index++] = byte;
        }
        return FRM_OK;
    case 7:
        ctx->hcs = get_crc16(ctx->hcs);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != (ctx->hcs & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 4;
            return -1;
        }
        ctx->step = 8;
        return FRM_OK;
    case 8:
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != ((ctx->hcs >> 8) & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 5;
            return -1;
        }
        ctx->index = 0;
        ctx->step = 9;
        return FRM_OK;
    case 9://用户链路数据
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (ctx->min_len > 2048)// 长度大于2048 暂时删除一个字节  为了台体能跑下去。  注意： 还需要超时删除报文！
        {
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 6;
            return -1;
        }
        
        if (ctx->index == ctx->min_len - 11 - SA_LEN(ctx->SA))//超过最大长度未测试？？
        {
            ctx->step = 10;
            return FRM_OK;
        }
        ctx->index++;
        return FRM_OK;
    case 10:
        ctx->fcs = get_crc16(ctx->fcs);
        if (byte != (ctx->fcs & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 7;
            return -1;
        }
        ctx->step = 11;
        return FRM_OK;
     
    case 11:
        if (byte != ((ctx->fcs >> 8) & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            ctx->err_type = 8;
            return -1;
        }
        ctx->step = 12;
        return FRM_OK;
    case 12:
        if (byte != 0x16){
            ctx->step = 0;
            ctx->err_type = 9;
            return -1;
        }
        return 1;//接收到完整的一帧
    }
    ctx->err_type = 10;
    //assert(0);
    return -1;
}
