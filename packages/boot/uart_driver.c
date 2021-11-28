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
#include "uart_driver.h"


#define MCU_UART_NUM    7


#define BAUD_CHG_FLAG    0x01
#define RST_PLC_FLAG     0x02

#define UART_DEVICE_TIME_SET      30000
#define UART_DEVICE_PLC_TIME_SET  1440000
#define UART_DEVICE_TIME_RESET    30
#define UART_DEVICE_TIME_OUT1     5
#define UART_DEVICE_TIME_OUT2	  1

#define UART_RX_TIME_OUT   5
#define UART_TX_TIME_OUT   3






static struct uart_chl_init_t uart_dev[MCU_UART_NUM];
static struct fifo_t uart_tx_fifo[MCU_UART_NUM];
static struct fifo_t uart_rx_fifo[MCU_UART_NUM];





struct uart_irq_list_t
{
    isr_handle tx_fun;
    isr_handle rx_fun;
};




void ndv_uart0_rx(void)
{
    uint8_t rcv_data = 0;
    if(true == mcu_uart_recive_byte(&rcv_data , 0))
    {
        fifoBuf_putByte(uart_dev[0].rx_fifo, rcv_data);
        uart_dev[0].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart0_tx(void)
{
    uint8_t tx_data = 0;
    if(fifoBuf_getUsed(uart_dev[0].tx_fifo) > 0 )
    {
        tx_data = fifoBuf_getByte(uart_dev[0].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[0].tx_time_out = UART_TX_TIME_OUT;
    }
}


void ndv_uart1_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 1))
    {
        fifoBuf_putByte(uart_dev[1].rx_fifo, rcv_data);
        uart_dev[1].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart1_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[1].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[1].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[1].tx_time_out = UART_TX_TIME_OUT;
    }
}


void ndv_uart2_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 2))
    {
        fifoBuf_putByte(uart_dev[2].rx_fifo, rcv_data);
        uart_dev[2].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart2_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[2].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[2].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[2].tx_time_out = UART_TX_TIME_OUT;
    }
}


void ndv_uart3_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 3))
    {
        fifoBuf_putByte(uart_dev[3].rx_fifo, rcv_data);
        uart_dev[3].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart3_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[3].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[3].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[3].tx_time_out = UART_TX_TIME_OUT;
    }
}

void ndv_uart4_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 4))
    {
        fifoBuf_putByte(uart_dev[4].rx_fifo, rcv_data);
        uart_dev[4].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart4_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[4].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[4].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[4].tx_time_out = UART_TX_TIME_OUT;
    }
}

void ndv_uart5_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 5))
    {
        fifoBuf_putByte(uart_dev[5].rx_fifo, rcv_data);
        uart_dev[5].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart5_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[5].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[5].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[5].tx_time_out = UART_TX_TIME_OUT;
    }
}

void ndv_uart6_rx(void)
{
    uint8_t rcv_data = 0;
    if (true == mcu_uart_recive_byte(&rcv_data, 6))
    {
        fifoBuf_putByte(uart_dev[6].rx_fifo, rcv_data);
        uart_dev[6].rx_time_out = UART_RX_TIME_OUT;
    }
}

void ndv_uart6_tx(void)
{
    uint8_t tx_data = 0;
    if (fifoBuf_getUsed(uart_dev[6].tx_fifo) > 0)
    {
        tx_data = fifoBuf_getByte(uart_dev[6].tx_fifo);
        mcu_uart_send_byte(tx_data, 0);
        uart_dev[6].tx_time_out = UART_TX_TIME_OUT;
    }
}


const struct uart_irq_list_t uart_irq_list[] =
{
    {ndv_uart0_tx,ndv_uart0_rx},
    {ndv_uart1_tx,ndv_uart1_rx},
    {ndv_uart2_tx,ndv_uart2_rx},
    {ndv_uart3_tx,ndv_uart3_rx},
    {ndv_uart4_tx,ndv_uart4_rx},
    {ndv_uart5_tx,ndv_uart5_rx},
    {ndv_uart6_tx,ndv_uart6_rx},
};



uint8 uart_time_process(void *tickpara)
{
    struct uart_chl_init_t *chl = (struct uart_chl_init_t *)tickpara;
    uint8_t send_data;
    
    if(chl->rx_time_out > 0)
    {
        chl->rx_time_out--;
    }
    
    if(chl->tx_time_out > 0)
    {
        chl->tx_time_out--;
    }
    
    if(chl->tx_time_out == 0)
    {
        mcu_uart_recive_enable(1 , chl->mcu_uart_index);
        mcu_uart_send_enable(0, chl->mcu_uart_index);
        if(chl->chl_type == UART_RS4851_CHL)
        {
            RS485_RCV_EN();
        }
    }
    
    if((fifoBuf_getUsed(chl->tx_fifo) > 0 ) && (chl->tx_time_out == 0))
    {
        if(chl->chl_type == UART_RS4851_CHL)
        {
            RS485_RCV_DIS();
        }
        chl->tx_time_out = UART_TX_TIME_OUT;
        send_data = fifoBuf_getByte(chl->tx_fifo);
        mcu_uart_send_enable(1, chl->mcu_uart_index);
        mcu_uart_send_byte(send_data , chl->mcu_uart_index);
        mcu_uart_recive_enable(0 , chl->mcu_uart_index);
    }

    return true;
}

int uart_chl_initialize(struct uart_chl_init_t *uart_chl)
{
    uint8_t index;
    uint8_t infrared_enable = 0;
    struct tm_list_init_t tm;
    
    if(uart_chl->mcu_uart_index < MCU_UART_NUM)
    {        
        index = uart_chl->mcu_uart_index;
        uart_dev[index].tx_fifo = uart_chl->tx_fifo;
        uart_dev[index].rx_fifo = uart_chl->rx_fifo;
        uart_dev[index].uart_cfg = uart_chl->uart_cfg;
        uart_dev[index].mcu_uart_index = uart_chl->mcu_uart_index;
        uart_dev[index].chl_type = uart_chl->chl_type;
        uart_dev[index].rx_time_out = 0;
        uart_dev[index].tx_time_out = 0;
        switch(uart_chl->chl_type)
        {
            case UART_RS4851_CHL:
                 RS485_RCV_EN();
                 RS485_RX_INPUT();
                 RS485_TX_OUTPUT();
                 RS485_TX_HIGH();
                 tm.tm_id = RS4851_TM;
                 break;
            case UART_IR_CHL:
                 infrared_enable = 1;
                 tm.tm_id = IR_TM;
                break;
            case UART_MODULE_CHL:
                tm.tm_id = MODULE_TM;
                break;
            default:
                break;
        }
        //tm.timeout = 1;
        //tm.tickpara = (void*)&uart_dev[index];
        //tm.tickfunc = uart_time_process;
        //tm.stoped = false;
        //ndv_timer_create(&tm);
        //ndv_isr_handle_regsit(uart_irq_list[index].tx_fun, IRQ_UART0_TX+(index<<1));
        //ndv_isr_handle_regsit(uart_irq_list[index].rx_fun, IRQ_UART0_RX+(index<<1));
        mcu_uart_init((void*)uart_chl->uart_cfg, index, infrared_enable);
        mcu_uart_recive_enable(1 , index);
    }
    return true;
}





/** @} */
