/**
 * @ingroup    drivers_uart_bus
 * @brief      uart 总线驱动
 *
 * @{
 * @file
 *
 */
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "time_link.h"
#include "pin_function_def.h"


#define MCU_UART_NUM    6


#define BAUD_CHG_FLAG    0x01
#define RST_PLC_FLAG     0x02

#define UART_DEVICE_TIME_SET      30000
#define UART_DEVICE_PLC_TIME_SET  1440000
#define UART_DEVICE_TIME_RESET    30
#define UART_DEVICE_TIME_OUT1     5
#define UART_DEVICE_TIME_OUT2	  1

;

/*关于串口及其外置设备的运行可靠性，主要考虑不同硬件体系下串口接收错误引起的UART锁死或者外置模块死机的情况*/
/*综合当前设计资源以及可靠性等级，在此处我们依赖于用户RAM不被意外篡改... shuk*/
struct uart_contex_t
{
    uint32 reset_tmr;
    uint32 rx_time_out;
    uint32 module_reset_tmr;//通讯模块复位计数器
};


static struct dev_info_t uart_dev[MCU_UART_NUM];
static struct fifo_t uart_tx_fifo[MCU_UART_NUM];
static struct fifo_t uart_rx_fifo[MCU_UART_NUM];
static struct uart_contex_t uart_contex[MCU_UART_NUM];

static uint8 rxbuffer[512];
static uint8 txbuffer[512];

void ndv_uart0_rx(void)
{
    uint8_t rcv_data = 0;
    if(true == mcu_uart_recive_byte(&rcv_data , 0))
    {
        fifoBuf_putByte(uart_dev[0].dev_output_fifo, rcv_data);
        uart_contex[0].rx_time_out = 5;
    }
}

void ndv_uart0_tx(void)
{
    uint8_t tx_data = 0;
    if(fifoBuf_getUsed(uart_dev[0].dev_input_fifo) > 0 )
    {
        tx_data = fifoBuf_getByte(uart_dev[0].dev_input_fifo);
        mcu_uart_send_byte(tx_data, 0);
    }
}

uint8 uart_time_process(void *tickpara)
{
    struct dev_info_t *dev = (struct dev_info_t *)tickpara;
    if(uart_contex[dev->dev_index].rx_time_out > 0)
    {
        uart_contex[dev->dev_index].rx_time_out--;
    }
    
    if(uart_contex[dev->dev_index].rx_time_out == 1)
    {
        mcu_uart_recive_enable(0 , dev->dev_index);
    }
    return true;
}

int uart_dev_initialize(struct dev_info_t *dev)
{
    uint8_t index = dev->dev_index;
    uint8_t *txbuf;
    uint8_t *rxbuf;
    uint8_t infrared_enable = 0;
    
    if(index < MCU_UART_NUM)
    {
        uart_dev[index].dev_type = dev->dev_type;
        uart_dev[index].data_size = dev->data_size;
        uart_dev[index].dev_timer_type = dev->dev_timer_type;
        uart_dev[index].dev_timer_id = ndv_timer_create(uart_time_process, dev, 1, false);
        uart_dev[index].dev_bus_ops = dev->dev_bus_ops;
        uart_dev[index].dev_driver_ops = dev->dev_driver_ops;
        uart_dev[index].dev_sys_ops = dev->dev_sys_ops;
        uart_dev[index].dev_input_fifo = &uart_tx_fifo[index];
        uart_dev[index].dev_output_fifo = &uart_rx_fifo[index];
        txbuf = txbuffer;
        rxbuf = rxbuffer;
        fifoBuf_init(uart_dev[index].dev_input_fifo, txbuf, uart_dev[index].data_size);
        fifoBuf_init(uart_dev[index].dev_output_fifo, rxbuf, uart_dev[index].data_size);
        if(dev->dev_type == DEV_IR)
        {
            infrared_enable = 1;
        }
        if(dev->dev_type == DEV_RS4851)
        {
            RS485_RCV_EN();
            RS485_RX_INPUT();
            RS485_TX_OUTPUT();
            RS485_TX_HIGH();
        }
        ndv_isr_handle_regsit(ndv_uart0_tx , IRQ_UART0_TX+(index<<1));
        ndv_isr_handle_regsit(ndv_uart0_rx , IRQ_UART0_RX+(index<<1));
        mcu_uart_init((void*)dev->cfg, index, infrared_enable);
        mcu_uart_recive_enable(1 , index);
    }
    return true;
}


int uart_dev_control(struct dev_info_t* dev, uint8 ctrl , void *ctrl_parameter)
{
     
    return true;
}


int uart_dev_write(struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{


     return true;
}

const struct driver_ops_t  CODE uart_driver =
{
    uart_dev_initialize,
    uart_dev_control,
    __NULL,
    uart_dev_write,
};



/** @} */
