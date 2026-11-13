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
#include "infra/boards/stategrid_single_phase/uart_pins.h"
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
#define UART_TX_TIME_OUT   3 //300波特率 发不完？？？   待调整






static struct uart_chl_init_t uart_dev[MCU_UART_NUM];
static struct fifo_t uart_tx_fifo[MCU_UART_NUM];
static struct fifo_t uart_rx_fifo[MCU_UART_NUM];








void UartReceiveHandle(void *para)
{
    uint8_t rcv_data = 0;
    
    uint8_t uartIndex = (uint8_t)para;
    if(uartIndex < MCU_UART_NUM)
    {
        if(true == mcu_uart_recive_byte(&rcv_data , uartIndex))
        {
            fifoBuf_putByte(uart_dev[uartIndex].rx_fifo, rcv_data);
            uart_dev[uartIndex].rx_time_out = UART_RX_TIME_OUT;
        }
    }
}

void UartTransmissionHandle(void *para)
{
    uint8_t tx_data = 0;
    uint8_t uartIndex = (uint8_t)para;
    if(uartIndex < MCU_UART_NUM)
    {
        if(fifoBuf_getUsed(uart_dev[uartIndex].tx_fifo) > 0 )
        {
            tx_data = fifoBuf_getByte(uart_dev[uartIndex].tx_fifo);
            mcu_uart_send_byte(tx_data, uartIndex);
            uart_dev[uartIndex].tx_time_out = UART_TX_TIME_OUT;
        }
        else
        {
            mcu_uart_send_enable(false, uartIndex);
            
            if(uart_dev[uartIndex].CallBackSendTimeOut)
            {
                uart_dev[uartIndex].CallBackSendTimeOut(uart_dev[uartIndex].CallBackSendPara);
            }
        }
    }
}


uint8_t uart_time_process(void *tickpara)
{
    struct uart_chl_init_t *chl = (struct uart_chl_init_t *)tickpara;
    uint8_t send_data;
    
    //if(chl->rx_time_out != 0xFFFFFFFF){
        if(chl->rx_time_out > 0)
        {
            chl->rx_time_out--;
        }
        
        if(chl->rx_time_out == 1)
        {
            chl->CallBackReceiveTimeOut();
            //chl->rx_time_out = 0xFFFFFFFF;
        }  
    //}
    if(chl->tx_time_out > 0)
    {
        chl->tx_time_out--;
    }
    
    if(chl->tx_time_out == 1)
    {
        mcu_uart_recive_enable(1 , chl->mcu_uart_index);
        if(chl->chl_type == UART_RS4851_CHL)
        {
            nd_uart_rs485_receiver_enable();
        }
    }
    
    //if((chl->tx_time_out == 0) && (chl->rx_time_out == 0))
    //{

    //}
    
    if((fifoBuf_getUsed(chl->tx_fifo) > 0 ) && (chl->tx_time_out == 0))
    {
        if(chl->chl_type == UART_RS4851_CHL)
        {
            nd_uart_rs485_receiver_disable();
        }
        chl->tx_time_out = UART_TX_TIME_OUT;
        send_data = fifoBuf_getByte(chl->tx_fifo);
        if(chl->chl_type == UART_IR_CHL) //禁止红外自发自收
        {
            mcu_uart_recive_enable(0 , chl->mcu_uart_index);
        }
        
        mcu_uart_send_enable(1, chl->mcu_uart_index);
        mcu_uart_send_byte(send_data , chl->mcu_uart_index);
    }

    return true;
}

int uart_chl_initialize(struct uart_chl_init_t *uart_chl)
{
    uint8_t index;
    uint8_t infrared_enable = 0;
    struct tm_list_init_t tm;
    struct isr_reg_ctx_t IsrReg;
    
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
        uart_dev[index].CallBackReceiveTimeOut = uart_chl->CallBackReceiveTimeOut;
        uart_dev[index].CallBackSendTimeOut = uart_chl->CallBackSendTimeOut;
        uart_dev[index].CallBackSendPara = uart_chl->CallBackSendPara;
        switch(uart_chl->chl_type)
        {
            case UART_RS4851_CHL:
                 nd_uart_rs485_receiver_enable();
                 nd_uart_rs485_rx_input();
                 nd_uart_rs485_tx_output();
                 nd_uart_rs485_tx_set(1);
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
        tm.timeout = 1;
        tm.tickpara = (void*)&uart_dev[index];
        tm.tickfunc = uart_time_process;
        tm.stoped = false;
        ndv_timer_create(&tm);
        
        IsrReg.IsrFunctionRegister = UartTransmissionHandle;
        IsrReg.para = (void*)(index);
        Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_UART0_TX+(index<<1));

        IsrReg.IsrFunctionRegister = UartReceiveHandle;
        IsrReg.para = (void*)(index);
        Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_UART0_RX+(index<<1));
        
        mcu_uart_init((void*)uart_chl->uart_cfg, index, infrared_enable);
        mcu_uart_recive_enable(1 , index);
    }
    return true;
}


int uart_chl_open(uint8_t chl_type)
{
    uint8_t index;
    uint8_t infrared_enable = 0;
    struct tm_list_init_t tm;
    
    for(index = 0; index<MCU_UART_NUM ; index++)
    {
        if(uart_dev[index].chl_type == chl_type)
        {
              switch(chl_type)
              {
                  case UART_RS4851_CHL:
                       nd_uart_rs485_receiver_enable();
                       nd_uart_rs485_rx_input();
                       nd_uart_rs485_tx_output();
                       nd_uart_rs485_tx_set(1);
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
              mcu_uart_init((void*)uart_dev[index].uart_cfg, index, infrared_enable);
              mcu_uart_recive_enable(1 , index);
              return true;
        }
    }
    return false;
}



/** @} */
