#include "boot.h"
#include "dlt698.h"
#include "fifo.h"
#include "lib_flash.h"  

struct status_ota_t g_status_ota;
struct map_ota_t g_map_ota = {0};
struct download_ota_t  g_download_ota = {0};

//no_init_ram
struct start_ota_t g_start_ota;


void memcopy(void *tb, void *sb, uint16_t length)
{
    uint8_t *tbuff = tb;
    uint8_t *sbuff = sb;

    // 先判断是否有正向移动重叠区域
    if ((sbuff < tbuff) && (sbuff + length > tbuff))
    {
        /* 有就从反向移动 */
        while (length != 0)                 //根据长度进行移动 
        {
            length--;
            tbuff[length] = sbuff[length];  //移动
        }
    }
    else                                    //其它情况进行正向移动
    {
        while (length != 0)
        {
            length--;
            *tbuff++ = *sbuff++;            //移动 
        }
    }
} 


void ota_mcu_init(void)
{
}

void ota_timer_init(void)
{
}

void ota_uart_init(void)
{
  
}

uint32_t is_update()
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


static int ota_server(struct fifo_t *fifo, void *pctx)
{
    struct context_698_t* ctx = (struct context_698_t*)pctx;
    uint32_t oad = 0;
    uint32_t Addr = 0;
    uint16_t len = 0;        
    uint8_t oad_buf[4];
    uint8_t *apdu;
    
    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    
    ctx->service = fifoBuf_getByte(fifo);
    ctx->choice  = fifoBuf_getByte(fifo);
    if((ctx->choice == 0xFF) && ((GET_Request == ctx->service)||(ACTION_Request == ctx->service)))
    {
        ctx->PIID = fifoBuf_getByte(fifo);
        ctx->PIID |= (1 << 6);//PIID-ACD    
    }

    fifoBuf_getData(fifo, oad_buf, 4);
    oad = ((uint32_t)oad_buf[0] << 24) + ((uint32_t)oad_buf[1] << 16) + ((uint32_t)oad_buf[2] << 8) + oad_buf[3];
     
    switch (oad)
    {
        case get_status_ota_oad:
            memcopy(apdu, (uint8_t*)&g_status_ota, sizeof(struct status_ota_t));
            len = sizeof(struct status_ota_t);
            break;
        case get_map_ota_oad:
            memcopy(apdu, (uint8_t*)&g_map_ota, 4);
            if(g_map_ota.bit_num > 4096)
                g_map_ota.bit_num = 4096;
            memcopy(apdu+4, (uint8_t*)&g_map_ota.bitmap, g_map_ota.bit_num);
            len = 4 + g_map_ota.bit_num;
            break;
        case start_ota_oad:
            memcopy((uint8_t*)&g_start_ota, apdu, sizeof(struct start_ota_t));
            break;
            
        case download_ota_oad:
            memcopy((uint8_t*)&g_download_ota, apdu, 8);
            g_download_ota.block_data = apdu+8;
            if(g_download_ota.block_no == 0)
            {
                
            }
            if(0x38de82ac == g_start_ota.type)//全包升级
            {
                if(is_bitmap(g_map_ota.bitmap, g_download_ota.block_no))
                {
                    Addr = g_download_ota.block_no*g_start_ota.block_size;
                    if(0 == (Addr % 4096))
                        FLASH_SectorErase(Addr);
                    
                    FLASH_ProgramWord(Addr, (uint32_t *)apdu, g_download_ota.block_len);
                    //位图标记
                    set_bitmap(g_map_ota.bitmap, g_download_ota.block_no);
                }
            }
            if(0x9267becc == g_start_ota.type)//定点升级
            {   
                Addr = g_download_ota.block_no;
                if(0 == (Addr % 4096))
                    FLASH_SectorErase(Addr);
                
                FLASH_ProgramWord(Addr, (uint32_t *)apdu, g_download_ota.block_len);
            }
            
            break;
        default:
            return -1;//回复异常，记录错误状态
            break;
    }


    return 0;
}
   
int main(int argc, char** argv)
{
    uint32_t time_out = 0;
    uint32_t i = 0, bit_num = 0;
    struct fifo_t *fifo;
    struct context_698_t ctx = {0};
    //1 配置mcu
    ota_mcu_init();
    //2 配置启动定时器
    ota_timer_init();
    //3 初始化串口
    ota_uart_init();
  
    //4 判断是否进入升级模式
    if(0x8338ddff == is_update())
    {
        //f13();
        for(;;)
        {
            //超时计时
            time_out++;
            if (fifoBuf_getUsed(fifo) >= 12)//数据帧大于最小长度
            {
                int r = ringbuffer_filter(fifo, matcher_698, &ctx);
                if (r > 0)
                {
                    ctx.fr_err = ota_server(fifo, &ctx);
                    return ctx.fr_err;
                }
            }
            bit_num = 0;
            for(i = 0 ; i< g_map_ota.bit_num; i++)
            {
                if(is_bitmap(g_map_ota.bitmap, i))
                   bit_num++;
            }
            
            if(bit_num == g_map_ota.bit_num)
            {
               //是否计算校验
               break;
            }
            if(time_out > 0xFFFFFFFE)//超时
            {
                break;
            }
        }
    }
    //jump

}