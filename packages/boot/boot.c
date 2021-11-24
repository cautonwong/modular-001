#include "boot.h"
#include "dlt698.h"
#include "fifo.h"


struct status_ota_t g_status_ota;

struct map_ota_t g_map_ota = {0};
//no_init_ram
struct start_ota_t g_start_ota;


void ota_mcu_init(void)
{
}

void ota_timer_init(void)
{
}

void ota_uart_init(void)
{
  
}

uint32_t is_ota()
{
    return 0x8338ddff;
}





static int ringbuffer_filter(struct fifo_t *buf, int (*callback)(void* ctx, uint8_t *b),void *ctx)
{       
    uint16_t i = 0;
    uint16_t rd = buf->rd;
    uint16_t wr = buf->wr;
    uint16_t r = 0;
    int ret = 0;
    uint16_t buf_size = buf->buf_size;
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    if (num_bytes < 1)
        return -1;                        
    
    while(i <= num_bytes)
    {
        r = rd + i;
        if (r >= buf_size)
            r -= buf_size;
        ret = callback(ctx, buf->buf_ptr+r);
        if(ret < 0)
        {
            //返回-1  
            num_bytes = wr - rd;
            if (wr < rd)
            {
                num_bytes = (buf_size - rd) + wr;
            }
            if (num_bytes < 1)
            {
                return ret;//-1迭代结束  没找到
            }

            if (++rd >= buf_size)//删掉一字节
                rd = 0;
            buf->rd = rd;
            i = 0;
            continue;
        }
        else if(ret == 1)
        {
            return ++i;//解析成功
        }
        else if (ret  == 2)
        {
            return 0;//字节不够  下次再来
        }
        else//  ret == 0  keep going
        {
            ++i;//已符合迭代器要求的字节数
        }
    }

    //rd += num_bytes;
    //if (rd >= buf_size)
        //rd -= buf_size;
    //buf->rd = rd;

    return -1;            
}

static int ota_server(struct fifo_t *fifo, void *ctx)
{
#if 0
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;

    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    
    //数据解密
    fr_security_decode(instance, pipeline, mesg);



    ctx->service = fifoBuf_getByte(fifo);
    ctx->choice = fifoBuf_getByte(fifo);
    if((ctx->choice == 0x01) && ((GET_Request == ctx->service)||(ACTION_Request == ctx->service)))
    {
        ctx->PIID = fifoBuf_getByte(fifo);
        ctx->PIID |= (1 << 6);//PIID-ACD    
        
    }

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
    uint8_t oad_buf[4];
    fifoBuf_getData(fifo, oad_buf, 4);
    oad = ((uint32_t)oad_buf[0] << 24) + ((uint32_t)oad_buf[1] << 16) + ((uint32_t)oad_buf[2] << 8) + oad_buf[3];
    uint16_t len = 1;         
    switch (oad)
    {
        case status_ota_oad:
            memcpy(p, (uint8_t*)&g_status_ota, sizeof(struct status_ota_t));
            len = sizeof(struct status_ota_t);
            break;
        case map_ota_oad:
            memcpy(p, (uint8_t*)&g_map_ota, 4);
            if(g_map_ota.bit_num > 512)
                g_map_ota.bit_num = 512;
            memcpy(p+4, (uint8_t*)&g_map_ota.bitmap, g_map_ota.bit_num);
            len = 4 + g_map_ota.bit_num;
            break;
        case start_ota_oad:
            memcpy((uint8_t*)&g_start_ota, p, sizeof(struct start_ota_t));
            break;
        case download_ota_oad:
            memcpy((uint8_t*)&g_download_ota, p, 8);
            g_download_ota.block_data = p+8;
            //位图检查
            if(is_bitmap(g_map_ota.bitmap, g_download_ota.block_no))
            {
                uint32_t Addr = g_download_ota.block_no*g_start_ota.block_size;
                if(is_sectorerase(g_download_ota.block_no))
                    FLASH_SectorErase(Addr);
                
                FLASH_ProgramWord(Addr, uint32_t *WordBuffer, g_download_ota.block_len);
                //位图标记
                set_bitmap(g_map_ota.bitmap, g_download_ota.block_no);
            }
            break;
        default:
            return -1;//回复异常，记录错误状态
            break;
    }

    //数据加密
    fr_security_encode(instance, pipeline, mesg);

    fr_encode_optional(instance, pipeline, mesg);

    if (fifoBuf_getUsed(ctx->apdu)  > 0)
    {
        fr_rx_698_end(instance, pipeline, mesg);
    }
#endif
    return 0;
}
   
int main(int argc, char** argv)
{
    //1 配置mcu
    ota_mcu_init();
    //2 配置启动定时器
    ota_timer_init();
    //3 初始化串口
    ota_uart_init();
  
    //4 判断是否进入升级模式
    if(0x8338ddff == is_ota())
    {
      f13();
        //5 升级模式数据帧处理
        for(;;)
        {
#if 0
            if (fifoBuf_getUsed(fifo) >= 12)//数据帧大于最小长度
            {
                int r = ringbuffer_filter(fifo, matcher_698, ctx);
                if (r > 0)
                {
                    ctx->fr_err = ota_server(fifo, ctx);
                    return ctx->fr_err;
                }
            }
            //超时计时
            time_out++;
#endif
        }
    }

}