/**
 * @ingroup    drivers_uart_bus
 * @brief      uart 总线驱动
 *
 * @{
 * @file
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu_surport.h"
#include "components/core/fifo/fifo.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include "targets/stategrid_meter/board/board_transport.h"
#include "targets/stategrid_meter/board/uart_pins.h"
#include "uart_driver.h"
#include "board_state.h"


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














void UartReceiveHandle(void *para)
{
    struct uart_chl_init_t *chl = (struct uart_chl_init_t *)para;
    uint8_t rcv_data = 0;
    uint8_t uartIndex;

    if(chl == NULL)
    {
        return;
    }
    uartIndex = chl->mcu_uart_index;
    if(uartIndex < MCU_UART_NUM)
    {
        if(true == mcu_uart_recive_byte(&rcv_data , uartIndex))
        {
            fifoBuf_putByte(chl->rx_fifo, rcv_data);
            chl->rx_time_out = UART_RX_TIME_OUT;
        }
    }
}

void UartTransmissionHandle(void *para)
{
    struct uart_chl_init_t *chl = (struct uart_chl_init_t *)para;
    uint8_t tx_data = 0;
    uint8_t uartIndex;

    if(chl == NULL)
    {
        return;
    }
    uartIndex = chl->mcu_uart_index;
    if(uartIndex < MCU_UART_NUM)
    {
        if(fifoBuf_getUsed(chl->tx_fifo) > 0 )
        {
            tx_data = (uint8_t)fifoBuf_getByte(chl->tx_fifo);
            mcu_uart_send_byte(tx_data, uartIndex);
            chl->tx_time_out = UART_TX_TIME_OUT;
        }
        else
        {
            mcu_uart_send_enable(false, uartIndex);
            
            if(chl->CallBackSendTimeOut)
            {
                chl->CallBackSendTimeOut(chl->CallBackPara);
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
            chl->CallBackReceiveTimeOut(chl->CallBackPara);
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
        send_data = (uint8_t)fifoBuf_getByte(chl->tx_fifo);
        if(chl->chl_type == UART_IR_CHL) //禁止红外自发自收
        {
            mcu_uart_recive_enable(0 , chl->mcu_uart_index);
        }
        
        mcu_uart_send_enable(1, chl->mcu_uart_index);
        mcu_uart_send_byte(send_data , chl->mcu_uart_index);
    }

    return true;
}

int uart_chl_initialize(struct stategrid_board_state *state,
                        struct uart_chl_init_t *uart_chl)
{
    uint8_t index;
    uint8_t infrared_enable = 0;
    struct tm_list_init_t tm;
    struct isr_reg_ctx_t IsrReg;
    struct uart_chl_init_t *chl;

    if(state == NULL || uart_chl == NULL)
    {
        return 0;
    }
    if(uart_chl->mcu_uart_index < MCU_UART_NUM)
    {        
        index = uart_chl->mcu_uart_index;
        chl = &state->uart_dev[index];
        chl->tx_fifo = uart_chl->tx_fifo;
        chl->rx_fifo = uart_chl->rx_fifo;
        chl->uart_cfg = uart_chl->uart_cfg;
        chl->mcu_uart_index = uart_chl->mcu_uart_index;
        chl->chl_type = uart_chl->chl_type;
        chl->rx_time_out = 0;
        chl->tx_time_out = 0;
        chl->CallBackReceiveTimeOut = uart_chl->CallBackReceiveTimeOut;
        chl->CallBackSendTimeOut = uart_chl->CallBackSendTimeOut;
        chl->CallBackPara = uart_chl->CallBackPara;
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
        tm.tickpara = (void*)chl;
        tm.tickfunc = uart_time_process;
        tm.stoped = false;
        ndv_timer_create(&tm);
        
        IsrReg.IsrFunctionRegister = UartTransmissionHandle;
        IsrReg.para = (void*)chl;
        Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_UART0_TX+(index<<1));

        IsrReg.IsrFunctionRegister = UartReceiveHandle;
        IsrReg.para = (void*)chl;
        Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_UART0_RX+(index<<1));
        
        mcu_uart_init((void*)uart_chl->uart_cfg, index, infrared_enable);
        mcu_uart_recive_enable(1 , index);
    }
    return true;
}


int uart_chl_open(struct stategrid_board_state *state, uint8_t chl_type)
{
    uint8_t index;
    uint8_t infrared_enable = 0;
    struct tm_list_init_t tm;

    if(state == NULL)
    {
        return 0;
    }
    for(index = 0; index<MCU_UART_NUM ; index++)
    {
        if(state->uart_dev[index].chl_type == chl_type)
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
              mcu_uart_init((void*)state->uart_dev[index].uart_cfg, index, infrared_enable);
              mcu_uart_recive_enable(1 , index);
              return true;
        }
    }
    return false;
}



#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ

void nd_board_uart_init(struct stategrid_board_state *state, size_t index,
                        const void *config)
{
    nd_board_uart_irq_init(state, index, config);
}


void nd_board_uart_tx_enable(struct stategrid_board_state *state, size_t index,
                             uint8_t enable)
{
    nd_board_uart_irq_tx_enable(state, index, enable);
}


void nd_board_uart_rx_enable(struct stategrid_board_state *state, size_t index,
                             uint8_t enable)
{
    nd_board_uart_irq_rx_enable(state, index, enable);
}


int nd_board_uart_write(struct stategrid_board_state *state, size_t index,
                        uint8_t byte)
{
    return nd_board_uart_irq_write(state, index, byte);
}


int nd_board_uart_tx_done(struct stategrid_board_state *state, size_t index)
{
    return nd_board_uart_irq_tx_done(state, index);
}


int nd_board_uart_read(struct stategrid_board_state *state, size_t index,
                       uint8_t *byte)
{
    return nd_board_uart_irq_read(state, index, byte);
}


int nd_board_uart_rx_done(struct stategrid_board_state *state, size_t index)
{
    return nd_board_uart_irq_rx_done(state, index);
}

#else

void nd_board_uart_init(struct stategrid_board_state *state, size_t index,
                        const void *config)
{
    (void)state;
    (void)mcu_uart_init((void *)config, (uint8_t)index, 0);
}


void nd_board_uart_tx_enable(struct stategrid_board_state *state, size_t index,
                             uint8_t enable)
{
    (void)state;
    mcu_uart_send_enable(enable, index);
}


void nd_board_uart_rx_enable(struct stategrid_board_state *state, size_t index,
                             uint8_t enable)
{
    (void)state;
    mcu_uart_recive_enable(enable, index);
}


int nd_board_uart_write(struct stategrid_board_state *state, size_t index,
                        uint8_t byte)
{
    (void)state;
    return mcu_uart_send_byte(byte, index) ? 0 : -1;
}


int nd_board_uart_tx_done(struct stategrid_board_state *state, size_t index)
{
    (void)state;
    return mcu_uart_get_tx_status(index);
}


int nd_board_uart_read(struct stategrid_board_state *state, size_t index,
                       uint8_t *byte)
{
    (void)state;
    return mcu_uart_recive_byte(byte, index) ? 0 : -1;
}


int nd_board_uart_rx_done(struct stategrid_board_state *state, size_t index)
{
    (void)state;
    return mcu_uart_get_rx_status(index);
}

#endif


/** @} */
