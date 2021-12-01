#include "boot.h"
#include "dlt698.h"
#include "fifo.h"
#include "lib_flash.h"  
#include "uart_driver.h"

#define  base_addr  0x00002000

UART_InitType  Uart_InitType_485 = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    9600,
};
UART_InitType  Uart_InitType_module = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    9600,
};
UART_InitType  Uart_InitType_ir = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    1200,
};

extern void memcopy(void *tb, void *sb, uint16_t length);
struct status_ota_t g_status_ota;
struct map_ota_t g_map_ota = {0};
struct download_ota_t  g_download_ota = {0};

//no_init_ram
struct start_ota_t g_start_ota;


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
    if((ctx->choice == 0x01) && ((GET_Request == ctx->service)||(ACTION_Request == ctx->service)))
    {
        ctx->PIID = fifoBuf_getByte(fifo);
        ctx->PIID |= (1 << 6);//PIID-ACD    

        fifoBuf_getData(fifo, &oad, 4);

        apdu = fifo->buf_ptr + fifo->rd;
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
                memcopy((uint8_t*)&g_start_ota, apdu, sizeof(struct start_ota_t) - 2);
                len= sizeof(struct start_ota_t) - 2;
                break;
                
            case download_ota_oad:
                memcopy((uint8_t*)&g_download_ota, apdu, 8);
                g_download_ota.block_data = apdu+8;
                len = g_download_ota.block_len + 8;
                
                if(g_download_ota.block_no == 0)
                {
                    
                }
                if(0x38de82ac == g_start_ota.type)//全包升级
                {
                    if(0 == is_bitmap(g_map_ota.bitmap, g_download_ota.block_no))
                    {
                        Addr = base_addr + g_download_ota.block_no*g_start_ota.block_size;
                        if(0 == (Addr % 4096))
                            FLASH_SectorErase(Addr);
                        
                        FLASH_ProgramWord(Addr, (uint32_t *)g_download_ota.block_data, g_download_ota.block_len);
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
    }
    for(uint16_t k=0; k<len ; k++)
        fifoBuf_getByte(fifo);
    
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);//16
    ctx->step = 0;
    //memset(ctx, 0, sizeof(struct context_698_t));
    return 0;
}
 
void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , uint8_t chl_type)
{
    struct uart_chl_init_t chl_init;

    switch(chl_type)
    {
        case UART_RS4851_AT_MCU_UART_INDEX:
             chl_init.uart_cfg = (const void*)&Uart_InitType_485;
             chl_init.mcu_uart_index = UART_RS4851_AT_MCU_UART_INDEX;
             break;
        case UART_IR_AT_MCU_UART_INDEX:
             chl_init.uart_cfg = (const void*)&Uart_InitType_ir;
             chl_init.mcu_uart_index = UART_IR_AT_MCU_UART_INDEX;
            break;
        case UART_MODULE_AT_MCU_UART_INDEX:
             chl_init.uart_cfg = (const void*)&Uart_InitType_module;
             chl_init.mcu_uart_index = UART_MODULE_AT_MCU_UART_INDEX;
            break;
        default:
            break;
    }

    chl_init.tx_fifo = txfifo;
    chl_init.rx_fifo = rxfifo;
    chl_init.chl_type = chl_type;    
    uart_chl_initialize(&chl_init);
}



void ota_mcu_init(void)
{
    mcu_init(0);
}

void ota_timer_init(void)
{
    extern int uart_time_process(void);
    ndv_isr_handle_regsit(uart_time_process , 0);
    
    cm0_systick_init(1);
}

#define FIFO_RX_BUF_SIZE_RS485 1024
#define FIFO_TX_BUF_SIZE_RS485 512
struct fifo_t fifo_rx_rs485={0};
uint8_t rs485_rx_time_out = 0;
uint8_t rx_rs485_buf[FIFO_RX_BUF_SIZE_RS485] = {0};
struct fifo_t fifo_tx_rs485={0};
uint8_t tx_rs485_buf[FIFO_TX_BUF_SIZE_RS485] = {0};

#define FIFO_RX_BUF_SIZE_IR 1024
#define FIFO_TX_BUF_SIZE_IR 512
struct fifo_t fifo_rx_ir={0};
uint8_t rx_ir_buf[FIFO_RX_BUF_SIZE_IR] = {0};
struct fifo_t fifo_tx_ir={0};
uint8_t tx_ir_buf[FIFO_TX_BUF_SIZE_IR] = {0};

#define FIFO_RX_BUF_SIZE_PLC 1024
#define FIFO_TX_BUF_SIZE_PLC 512
struct fifo_t fifo_rx_plc={0};
uint8_t rx_plc_buf[FIFO_RX_BUF_SIZE_PLC] = {0};
struct fifo_t fifo_tx_plc={0};
uint8_t tx_plc_buf[FIFO_TX_BUF_SIZE_PLC] = {0};

#define  UART_RS4851_AT_MCU_UART_INDEX   0
#define  UART_MODULE_AT_MCU_UART_INDEX   4
#define  UART_IR_AT_MCU_UART_INDEX       5


void ota_uart_init(void)
{
    fifoBuf_init(&fifo_rx_rs485, rx_rs485_buf, FIFO_RX_BUF_SIZE_RS485);
    fifoBuf_init(&fifo_tx_rs485, tx_rs485_buf, FIFO_TX_BUF_SIZE_RS485);
    communication_chlx_init(&fifo_tx_rs485, &fifo_rx_rs485, UART_RS4851_AT_MCU_UART_INDEX);
    
    fifoBuf_init(&fifo_rx_ir, rx_ir_buf, FIFO_RX_BUF_SIZE_IR);
    fifoBuf_init(&fifo_tx_ir, tx_ir_buf, FIFO_TX_BUF_SIZE_IR);
    communication_chlx_init(&fifo_tx_ir, &fifo_rx_ir, UART_IR_AT_MCU_UART_INDEX);
    
    fifoBuf_init(&fifo_rx_plc, rx_plc_buf, FIFO_RX_BUF_SIZE_PLC);
    fifoBuf_init(&fifo_tx_plc, tx_plc_buf, FIFO_TX_BUF_SIZE_PLC);
    communication_chlx_init(&fifo_tx_plc, &fifo_rx_plc, UART_MODULE_AT_MCU_UART_INDEX);
    
}

uint32_t is_update()
{
    return 0x8338ddff;
}


int main(int argc, char** argv)
{
    uint32_t time_out = 0;
    uint32_t i = 0, bit_num = 0;
    struct fifo_t *fifo = &fifo_rx_rs485;
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
            if ((rs485_rx_time_out)&&(fifoBuf_getUsed(fifo) >= 24))//数据帧大于最小长度
            {
                //if(fifoBuf_getUsed(fifo) == 0)
                    rs485_rx_time_out = 0;
                
                int r = ringbuffer_filter(fifo, matcher_698, &ctx);
                if (r > 0)
                {
                    ctx.fr_err = ota_server(fifo, &ctx);;
                }
            }
            bit_num = 0;
            for(i = 0 ; i< g_map_ota.bit_num; i++)
            {
                if(is_bitmap(g_map_ota.bitmap, i))
                   bit_num++;
            }
            
            if((bit_num == g_map_ota.bit_num) && (g_map_ota.bit_num != 0))
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