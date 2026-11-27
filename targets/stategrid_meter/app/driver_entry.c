#include "cpu_surport.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "lib_conf.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include "uart_driver.h"
#include "targets/stategrid_meter/board/board_io.h"
#include "targets/stategrid_meter/board/uart_pins.h"
#include "targets/stategrid_meter/app/app_config.h"
#include "targets/stategrid_meter/app/display_applications.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/board/board_eeprom.h"
#include "targets/stategrid_meter/board/board_flash.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/app/driver_entry.h"
#include "targets/stategrid_meter/app/stategrid_context.h"

const UART_InitType  Uart_InitType_485 =
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    9600,
};
const UART_InitType  Uart_InitType_module = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    9600,
};
const UART_InitType  Uart_InitType_ir = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    1200,
};


void RTC_Second(void *para)
{
    stategrid_board_state_t *state = para;

    if (state != NULL) {
        fifoBuf_putByte(&state->irq_fifo, IRQ_RTC);
    }
}

//static uint8_t key_keep_flag = 0;
//static uint8_t key_keep_width = 0;
void IO_Interrupt(void *para)
{
   uint16_t cnt;
   stategrid_board_state_t *state = para;

    if (state == NULL) {
        return;
    }

    if( PMU_GetIOAINTStatus(GPIO_Pin_8))   //有功脉冲
    {
        PMU_ClearIOAINTStatus(GPIO_Pin_8);
        //GPIOF->OEN &=~BIT1;
        //GPIOF->IE &=~BIT1;
        //GPIOF->ATT &=~BIT1;
        //IO_DAT( GPIOF_DAT, 1) = 1;
        state->wpulse_light_open = 0x5a;
        //wpulse_light_flag = 0;
        fnINTTC3_Init(1);
    }

    if(GPIOA->INT & BIT2)  // 按键
    {
        
//        key_keep_flag = 0x5a;
//        key_keep_width = 0;
        
        //// 停电下按键消抖
        if(ANA->COMPOUT & ANA_STATUS_VDCINDROP)
        {
//             key_keep_flag = 0; // 避免停电下 tick被打开 发两次 按键消息
             if(ANA->COMPOUT & BIT2)//有电池
             {
                 for(cnt = 0; cnt< 3; cnt++)  
                 {
                    if(((GPIOA->STS & BIT2) != 0))
                    {
                        break;
                    }
                 }
                 if (cnt >= 3)
                 {
                    fifoBuf_putByte(&state->irq_fifo, IRQ_PMU);
                 }
             }
        }
        else
        {
            fifoBuf_putByte(&state->irq_fifo, IRQ_PMU);
        }
        PMU->IOAINTSTS = BIT2;
    }
}


void timing_ftr(void *para)
{
      uint8_t dly1,dly2=0;
      stategrid_board_state_t *state = para;

      if (state == NULL) {
          return;
      }

      if(over_Imax == 0x55)
      {
          dly1 = 5;
          dly2 = 16;    //32ms
      }
      else
      {
          dly1 = 10;
          dly2 = 30;
      }
      /******************** 消抖 ********************/
      if(state->wpulse_light_open == 0x5a)
      {
          if((GPIOA->STS & BIT8) == BIT8)//消抖
          {
                state->wpulse_light_open_flag++;
          }
          else
          {
                state->wpulse_light_open_flag=0;
          }
          if(state->wpulse_light_open_flag >= dly1)//20ms
          {

                state->wpulse_light_open = 0;
                state->wpulse_light_open_flag = 0;
                state->wpulse_light_flag = 0x5A;
                state->wplse_light_width = 0;
                GPIOF->OEN &=~BIT1;
                GPIOF->IE &=~BIT1;
                GPIOF->ATT &=~BIT1;
                IO_DAT( GPIOF_DAT, 1) = 1;
          }
      }

      /******************** 脉宽80ms ********************/
      if(state->wpulse_light_flag == 0x5a)
      {
          state->wplse_light_width++;
          if(state->wplse_light_width > dly2)
          {
               GPIOF->OEN &=~BIT1;
               GPIOF->IE &=~BIT1;
               GPIOF->ATT &=~BIT1;
               IO_DAT( GPIOF_DAT, 1) = 0;
               state->wplse_light_width = 0;
               state->wpulse_light_flag = 0;
               Energy_Pluse_Add();
               TMR_INTConfig(TMR3, DISABLE);  //Enable Timer0 interrupt
               TMR_Cmd(TMR3, DISABLE);  //Enable Timer0
          }
/*          else
          {
              GPIOF->OEN &=~BIT1;
              GPIOF->IE &=~BIT1;
              GPIOF->ATT &=~BIT1; 
              IO_DAT( GPIOF_DAT, 1) = 1;
          }*/
      }

}

static void irq_fifo_push(void *instance, uint8_t irq)
{
    stategrid_board_state_t *state = stategrid_context_board(instance);

    if (state != NULL) {
        fifoBuf_putByte(&state->irq_fifo, irq);
    }
}

void rs485_receive_timeout(void *CallBackPara)
{
    irq_fifo_push(CallBackPara, IRQ_RS485_RX_TM_OUT);
}

void plc_receive_timeout(void *CallBackPara)
{
    irq_fifo_push(CallBackPara, IRQ_PLC_RX_TM_OUT);
}

void ir_receive_timeout(void *CallBackPara)
{
    irq_fifo_push(CallBackPara, IRQ_IR_RX_TM_OUT);
}

const uint32_t baud_table[11]={300,600,1200,2400,4800,7200,9600,19200,38400,57600,115200};

void baud_update(void *CallBackSendPara)
{
    uint32_t baud=115200,i; 
    stategrid_context_t * instance = (stategrid_context_t *)CallBackSendPara;
    struct mbase_control_block_t* mbase_control_block = ((stategrid_context_t *)instance)->mbase;

    if(nd_mbase_baud_chg(mbase_control_block) & 0x01)    //485
    {
        for(i=0;i<11;i++)
        {
            if(nd_mbase_Baut_rs485(mbase_control_block) == i)
            {
                baud = baud_table[i];
                break;
            }
        }
        

        mcu_uart_set_baud(baud, 0);
    }
    if(nd_mbase_baud_chg(mbase_control_block) & 0x02)    //plc
    {
        for(i=0;i<11;i++)
        {
            if(nd_mbase_Baut_plc(mbase_control_block) == i)
            {
                baud = baud_table[i];
                break;
            }
        }

        mcu_uart_set_baud(baud, 4);
    }
    if(nd_mbase_baud_chg(mbase_control_block) & (0x04 + 0x02))    //plc协商    只要协商存在，始终以协商为准
    {
        for(i=0;i<11;i++)
        {
            if(nd_mbase_Baut_plc_neg(mbase_control_block) == i)
            {
                baud = baud_table[i];
                break;
            }
        }

        mcu_uart_set_baud(baud, 4);
    }
    nd_mbase_baud_chg(mbase_control_block) = 0; 
}

void rs485_send_timeout(void *CallBackSendPara)
{
    nd_uart_rs485_receiver_enable();
    //这里是发送结束最后一个字节后调用的函数，可以在这里判断是否有波特率修改，并调用修改波特率函数，注意该函数是在中断调用，因此要简洁
    baud_update(CallBackSendPara);
}

void plc_send_timeout(void *CallBackSendPara)
{   
    //这里是发送结束最后一个字节后调用的函数，可以在这里判断是否有波特率修改，并调用修改波特率函数，注意该函数是在中断
    baud_update(CallBackSendPara);
}

void ir_send_timeout(void *CallBackSendPara)
{
    //这里是发送结束最后一个字节后调用的函数，可以在这里判断是否有波特率修改，并调用修改波特率函数，注意该函数是在中断
    baud_update(CallBackSendPara);
}   


void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , uint8_t chl_type , stategrid_context_t * p_instance)
{
    struct uart_chl_init_t chl_init;
    
    switch(chl_type)
    {
        case UART_RS4851_CHL:
             chl_init.uart_cfg = (const void*)&Uart_InitType_485;
             chl_init.mcu_uart_index = ND_UART_RS4851;
             chl_init.CallBackReceiveTimeOut = rs485_receive_timeout;
             chl_init.CallBackSendTimeOut = rs485_send_timeout; 
             break;
        case UART_IR_CHL:
             chl_init.uart_cfg = (const void*)&Uart_InitType_ir;
             chl_init.mcu_uart_index = ND_UART_IR;
             chl_init.CallBackReceiveTimeOut = ir_receive_timeout;
             chl_init.CallBackSendTimeOut = ir_send_timeout;
            break;
        case UART_MODULE_CHL:
             chl_init.uart_cfg = (const void*)&Uart_InitType_module;
             chl_init.mcu_uart_index = ND_UART_MODULE;
             chl_init.CallBackReceiveTimeOut = plc_receive_timeout;
             chl_init.CallBackSendTimeOut = plc_send_timeout;
            break;
        default:
            break;
    }
    chl_init.CallBackPara = p_instance;
    chl_init.tx_fifo = txfifo;
    chl_init.rx_fifo = rxfifo;
    chl_init.chl_type = chl_type;    
    uart_chl_initialize(p_instance->board, &chl_init);
}

void board_irq_bind(stategrid_board_state_t *state)
{
    struct isr_reg_ctx_t IsrReg;

    if (state == NULL) {
        return;
    }

    fifoBuf_init(&state->irq_fifo, state->irq_fifo_buf, 32);

    IsrReg.IsrFunctionRegister = RTC_Second;
    IsrReg.para = state;
    Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_RTC);

    IsrReg.IsrFunctionRegister = IO_Interrupt;
    IsrReg.para = state;
    Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_PMU);

    IsrReg.IsrFunctionRegister = timing_ftr;
    IsrReg.para = state;
    Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_TMR3);

    __enable_irq();

}

void irq_transform_to_instance(stategrid_context_t * instance)
{
    uint8_t fifo_irq = 0;
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);
    struct fifo_t *irq_fifo = &ctx->board->irq_fifo;

#ifdef DEBUG
    if(fifoBuf_getUsed(irq_fifo) >0)
#else
    while(fifoBuf_getUsed(irq_fifo) >0)
#endif
    {
       fifo_irq = (uint8_t)fifoBuf_getByte(irq_fifo);
       
       switch(fifo_irq)
       {
           case IRQ_RTC:
                fifoBuf_putByte(&ctx->pipe_rtc, IRQ_RTC);
                break;
             
           case IRQ_PMU:
                fifoBuf_putByte(&ctx->pipe_key, IRQ_PMU);
                break;
           case IRQ_RS485_RX_TM_OUT:
                fifoBuf_putByte(&ctx->pipe_uart[UART_RS4851_CHL], IRQ_UART_RX_TM_OUT);
                break;
           case IRQ_PLC_RX_TM_OUT:
                fifoBuf_putByte(&ctx->pipe_uart[UART_MODULE_CHL], IRQ_UART_RX_TM_OUT);
                break;
           case IRQ_IR_RX_TM_OUT:
                fifoBuf_putByte(&ctx->pipe_uart[UART_IR_CHL], IRQ_UART_RX_TM_OUT);
                break;
           default:
                break;
       }
    }
}

void IOStateCheckInit(stategrid_board_state_t *state);

void mcu_main_frequency_mode(uint8_t clk_mode)
{
    CLK_InitTypeDef CLK_Struct;
    uint32_t Periphral = 0;

    if (clk_mode)
    {
        CLK_Struct.ClockType = CLK_TYPE_AHBSRC \
            | CLK_TYPE_PLLL   \
            | CLK_TYPE_HCLK   \
            | CLK_TYPE_PCLK \
            |CLK_TYPE_RTCCLK;
        CLK_Struct.AHBSource = CLK_AHBSEL_LSPLL;
        CLK_Struct.PLLL.Frequency = CLK_PLLL_26_2144MHz;
        CLK_Struct.PLLL.Source = CLK_PLLLSRC_XTALL;
        CLK_Struct.PLLL.State = CLK_PLLL_ON;
        CLK_Struct.HCLK.Divider = 1;
        CLK_Struct.PCLK.Divider = 1;
        CLK_Struct.RTCCLK.Source = 0;
        CLK_Struct.RTCCLK.Divider = 0;
        CLK_ClockConfig(&CLK_Struct);

        Periphral = CLK_APBPERIPHRAL_PMU | CLK_APBPERIPHRAL_RTC | CLK_APBPERIPHRAL_ANA \
            | CLK_APBPERIPHRAL_TIMER | CLK_APBPERIPHRAL_UART3 \
            | CLK_APBPERIPHRAL_UART4 | CLK_APBPERIPHRAL_UART5 \
            | CLK_APBPERIPHRAL_UART0;
        CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_LCD | CLK_AHBPERIPHRAL_GPIO), ENABLE);
        CLK_APBPeriphralCmd(Periphral, ENABLE);
    }
    else
    {

    }
}


void dev_init(stategrid_context_t * instance, uint8_t power_mode)
{
    mcu_main_power_on_config(power_mode);
    
    //ndv_timer_init();
	
    IOStateCheckInit(instance->board);
    
    (void)board_eeprom(instance->board);

    nd_board_key_input_enable();

    (void)board_flash(instance->board);

    rn8209_target_power(instance, power_mode == AC_POWER_ON);
    
    esam_698_init(instance, power_mode);
}

void report_port_open(stategrid_context_t * instance)
{
    struct mbase_control_block_t* mbase_cb = ((stategrid_context_t *)instance)->mbase;
    uint8_t Baut = DEF_BAUT_PLC;
    
    CLK_InitTypeDef CLK_Struct;
    uint32_t Periphral = 0;

    CLK_Struct.ClockType = CLK_TYPE_AHBSRC \
        | CLK_TYPE_PLLL   \
        | CLK_TYPE_HCLK   \
        | CLK_TYPE_PCLK;
    CLK_Struct.AHBSource = CLK_AHBSEL_LSPLL;
    CLK_Struct.PLLL.Frequency = CLK_PLLL_26_2144MHz;
    CLK_Struct.PLLL.Source = CLK_PLLLSRC_XTALL;
    CLK_Struct.PLLL.State = CLK_PLLL_ON;
    CLK_Struct.HCLK.Divider = 1;
    CLK_Struct.PCLK.Divider = 1;
    CLK_ClockConfig(&CLK_Struct);
    Periphral = CLK_APBPERIPHRAL_UART4;
    CLK_AHBPeriphralCmd(( CLK_AHBPERIPHRAL_GPIO), ENABLE);
    CLK_APBPeriphralCmd(Periphral, ENABLE);

    GPIOB->ATT |= BIT0;//plcrx
    GPIOB->ATT |= BIT6;//plctx
    cm0_systick_init(1);
    uart_chl_open(instance->board, UART_MODULE_CHL);

    if(nd_mbase_Baut_plc_neg(mbase_cb) <= 10)
    {
        Baut = nd_mbase_Baut_plc_neg(mbase_cb);    //载波波特率
    }
    else 
    {   
        if(nd_mbase_Baut_plc(mbase_cb) <= 10)
            Baut = nd_mbase_Baut_plc(mbase_cb);    //载波波特率
    }    
    
    mcu_uart_set_baud(baud_table[Baut], 4);
}

//关闭用电大户外设的电源，比如背光，LED，串口，高速定时器，tick
 void device_power_close(stategrid_context_t * instance)
 {
    uint32_t lcd_tmp[20] = {0,};
    
    GPIO_InitType GPIO_InitStruct;
    PMU_LowPWRTypeDef LowPower_InitStruct;

    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE); //先关闭脉冲输出 中断
    cm0_systick_init(0);

    MISC_IRCmd(MISC_IREN_TX5, DISABLE);
    TMR_INTConfig(TMR0, DISABLE);
    TMR_INTConfig(TMR3, DISABLE);
    TMR_Cmd(TMR0, DISABLE);
    TMR_Cmd(TMR3, DISABLE);
    UART_INTConfig(UART0, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART3, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART4, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART5, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    
    /*------------------ Forbidden all GPIOs ------------------*/
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_All;
    //GPIOA_Init(GPIOA, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOB, &GPIO_InitStruct);
/*  if (system_state.power_off_sencond > 5)
    {
        GPIOBToF_Init(GPIOC, &GPIO_InitStruct);
    }*/
    GPIOBToF_Init(GPIOD, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOF, &GPIO_InitStruct);
    
/*  GPIOA->DAT &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);*/
    GPIOA->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOA->IE &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
  
  
    GPIOA->OEN |=BIT14; 
    GPIOA->IE |=BIT14;
    GPIOA->DAT |= BIT2;
    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //使能外部IO口唤醒
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);
    CORTEX_SetPriority_ClearPending_EnableIRQ(PMU_IRQn, 0); //优先级高
    
    LCD->CTRL |= LCD_CTRL_EN;
    MISC2->HCLKEN |= (uint32_t)BIT6;
    memcpy((uint8_t*)(&LCD->FB[0]), (uint8_t *)&lcd_tmp[0], sizeof(lcd_tmp));
    lcd_init(AC_POWER_OFF);

    RTC_PLLDIVOutputCmd(DISABLE);
    MComp1Init();//电池状态监测初始化   
}

//上报完之后调用，关闭除液晶以外的 用电大户外设的电源，比如背光，LED，串口，高速定时器，tick
 void device_power_close_after_report(stategrid_context_t * instance)
 {
    uint32_t lcd_tmp[20] = {0,};
    
    GPIO_InitType GPIO_InitStruct;
    PMU_LowPWRTypeDef LowPower_InitStruct;
    
    cm0_systick_init(0);

    MISC_IRCmd(MISC_IREN_TX5, DISABLE);
    TMR_INTConfig(TMR0, DISABLE);
    TMR_INTConfig(TMR3, DISABLE);
    TMR_Cmd(TMR0, DISABLE);
    TMR_Cmd(TMR3, DISABLE);
    UART_INTConfig(UART0, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART3, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART4, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    UART_INTConfig(UART5, UART_INT_RX | UART_INT_TXDONE, DISABLE);
    
    //LCD->CTRL |= LCD_CTRL_EN;
    //MISC2->HCLKEN |= (uint32_t)BIT6;
    //memcpy((uint8_t*)(&LCD->FB[0]), (uint8_t *)&lcd_tmp[0], sizeof(lcd_tmp));
    //lcd_init(AC_POWER_OFF);

    /*------------------ Forbidden all GPIOs ------------------*/
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_All;
    //GPIOA_Init(GPIOA, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOB, &GPIO_InitStruct);
/*    if (system_state.power_off_sencond > 5)
    {
        GPIOBToF_Init(GPIOC, &GPIO_InitStruct);
    }*/
    GPIOBToF_Init(GPIOD, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOF, &GPIO_InitStruct);
    
/*       GPIOA->DAT &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);*/
    GPIOA->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOA->IE &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
  
  
    GPIOA->OEN |=BIT14; 
    GPIOA->IE |=BIT14;
    GPIOA->DAT |= BIT2;
    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //使能外部IO口唤醒
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);
    CORTEX_SetPriority_ClearPending_EnableIRQ(PMU_IRQn, 0); //优先级高

    /*------------------ Low power configuration ------------------*/
    LowPower_InitStruct.AHBPeriphralDisable = PMU_AHB_ALL;
    LowPower_InitStruct.LCDPower            = PMU_LCDPWER_OFF;
    
    LowPower_InitStruct.APBPeriphralDisable = PMU_APB_ALL;
    LowPower_InitStruct.BGPPower            = PMU_BGPPWR_OFF;
    LowPower_InitStruct.COMP1Power          = PMU_COMP1PWR_ON;//PMU_COMP1PWR_OFF;
    LowPower_InitStruct.COMP2Power          = PMU_COMP2PWR_OFF;
    LowPower_InitStruct.AVCCPower           = PMU_AVCCPWR_ON;
    LowPower_InitStruct.TADCPower           = PMU_TADCPWR_OFF;
    LowPower_InitStruct.VDCINDetector       = PMU_VDCINDET_ENABLE;
    LowPower_InitStruct.VDDDetector         = PMU_VDDDET_DISABLE;
    RTC_PLLDIVOutputCmd(DISABLE);
    MComp1Init();//电池状态监测初始化
    WDT_Disable();
    PMU_EnterSleep_LowPower(&LowPower_InitStruct);
 }
uint8_t shut_multifunction_pluse(void *ptr)
{
    opreate_second_pluse(3);
    return 0;
}

uint8_t multifunction_port_Init(void)
{
   struct tm_list_init_t tm;

   tm.timeout = 8;
   tm.tickpara = 0;
   tm.tickfunc = shut_multifunction_pluse;
   tm.tm_id = MULTIFUNCTION_PORT;
   tm.stoped = true;
   
   ndv_timer_create(&tm);
   
   return true;
}

void rs485_init(stategrid_context_t * instance, void * pipeline)
{
    uint32_t baud = 115200;
    uint8_t i;
    struct mbase_control_block_t* mbase_cb = ((stategrid_context_t *)instance)->mbase;
    
    uart_chl_open(instance->board, UART_RS4851_CHL);

    for(i=0;i<11;i++)
    {
        if(nd_mbase_Baut_rs485(mbase_cb) == i)
        {
            baud = baud_table[i];
            break;
        }
    }

    mcu_uart_set_baud(baud, 0);     //485波特率切换至EEPROM设置要求
    
}

//刷新所有通道   波特率配置
void reset_all_channel_baud(stategrid_context_t * instance)
{
    struct mbase_control_block_t* mbase_cb = ((stategrid_context_t *)instance)->mbase;

    nd_mbase_baud_chg(mbase_cb) = 0xFF; 
    baud_update((void *)instance);
}

//打开串口，高速定时器，tick，初始化IO上电配置
 void device_power_open(stategrid_context_t * instance, void * pipeline)
 {
    uint32_t baud = 115200;
    uint8_t i;
    struct mbase_control_block_t* mbase_cb = ((stategrid_context_t *)instance)->mbase;
    
    mcu_main_frequency_mode(1);
    gpio_configuration_AC();
    
    nd_board_relay_led_set(1);                   //拉闸灯
    lcd_init(AC_POWER_ON);
    AllDisplayLCD(instance, pipeline);
    
    dev_init(instance, AC_POWER_ON);
    
    rs485_init(instance, pipeline);

    uart_chl_open(instance->board, UART_IR_CHL);
    uart_chl_open(instance->board, UART_MODULE_CHL);

    opreate_second_pluse(1);
    
    nd_mbase_MulPortMode(mbase_cb) = 0;  //上电默认为秒信号

    multifunction_port_Init();
 }

void init_rs485_baud()
{
    mcu_uart_set_baud(115200, 0);
}

//初始化IO低功耗配置，初始化系统低功耗配置，准备进入sleep模式。
void cpu_io_lowpower_cfg_chk(void *instance)
{







}

//此处放sleep函数，以及唤醒后的系统基本处理
void cpu_sleep_manage(stategrid_context_t * instance, void * pipeline)
{
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);
    struct mbase_control_block_t* mbase_cb = ctx->mbase;

    (void)pipeline;
    PMU_LowPWRTypeDef LowPower_InitStruct;
    GPIO_InitType GPIO_InitStruct;
    
    if(((nd_mbase_Key(mbase_cb) & F_KeyAwake) == 0))
    {
        //LCD_InitType lcd_init_para;
        //LCD_IOInitType lcd_io_para;
        uint32_t lcd_tmp[20] = {0,};

        cm0_systick_init(0);
        MISC_IRCmd(MISC_IREN_TX5, DISABLE);
        TMR_INTConfig(TMR0, DISABLE);
        TMR_INTConfig(TMR3, DISABLE);
        TMR_Cmd(TMR0, DISABLE);
        TMR_Cmd(TMR3, DISABLE);
        UART_INTConfig(UART0, UART_INT_RX | UART_INT_TXDONE, DISABLE);
        UART_INTConfig(UART3, UART_INT_RX | UART_INT_TXDONE, DISABLE);
        UART_INTConfig(UART4, UART_INT_RX | UART_INT_TXDONE, DISABLE);
        UART_INTConfig(UART5, UART_INT_RX | UART_INT_TXDONE, DISABLE);
        
        LCD->CTRL |= LCD_CTRL_EN;
        MISC2->HCLKEN |= (uint32_t)BIT6;
        memcpy((uint8_t *)(&LCD->FB[0]), (uint8_t *)&lcd_tmp[0], sizeof(lcd_tmp));
        lcd_init(AC_POWER_OFF);

        /*------------------ Forbidden all GPIOs ------------------*/
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
        GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_All;
        //GPIOA_Init(GPIOA, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOB, &GPIO_InitStruct);

        GPIOBToF_Init(GPIOC, &GPIO_InitStruct);

        if(( ctx->ac_power_off_sec_cnt <= 5 )) // 电源异常检测、上报等。   e2p电源打开，电源上升完整大约需要50ms
        {
            GPIOC->OEN &= ~(BIT4|BIT5|BIT6);  //e2p 电源打开
            GPIOC->IE &= ~(BIT4|BIT5|BIT6);
            GPIOC->DAT |= (BIT6);
        }
        
        GPIOBToF_Init(GPIOD, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOF, &GPIO_InitStruct);
        
        GPIOA->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
        GPIOA->IE &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
 
        lcd_port_config(AC_POWER_OFF);// 液晶置低   mcu芯片要求   避免电荷残存

        //卡表功能引脚不启用，做输出低状态
        GPIOA->OEN &= ~BIT13; 
        GPIOA->IE &= ~BIT13;
        GPIOA->DAT &= ~BIT13;
        
        GPIOB->OEN &= ~BIT3; 
        GPIOB->IE &= ~BIT3;
        GPIOB->DAT &= ~BIT3;
        
        GPIOF->OEN &= ~BIT0; 
        GPIOF->IE &= ~BIT0;
        GPIOF->DAT &= ~BIT0;
        
        GPIOE->OEN &= ~(BIT4 | BIT8); 
        GPIOE->IE &= ~(BIT4 | BIT8);
        GPIOE->DAT &= ~(BIT4 | BIT8);
      
        GPIOA->OEN |=BIT14; 
        GPIOA->IE |=BIT14;
        GPIOA->DAT |= BIT2;
        PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //使能外部IO口唤醒
        PMU_ClearIOAINTStatus(GPIO_Pin_2);
        PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
        PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);
        CORTEX_SetPriority_ClearPending_EnableIRQ(PMU_IRQn, 0); //优先级高

        LowPower_InitStruct.AHBPeriphralDisable = PMU_AHB_ALL;
        LowPower_InitStruct.LCDPower            = PMU_LCDPWER_OFF;
        LowPower_InitStruct.APBPeriphralDisable = PMU_APB_ALL;
        LowPower_InitStruct.BGPPower            = PMU_BGPPWR_OFF;
        LowPower_InitStruct.COMP1Power          = PMU_COMP1PWR_ON;//PMU_COMP1PWR_OFF;
        LowPower_InitStruct.COMP2Power          = PMU_COMP2PWR_OFF;
        LowPower_InitStruct.AVCCPower           = PMU_AVCCPWR_ON;
        LowPower_InitStruct.TADCPower           = PMU_TADCPWR_OFF;
        LowPower_InitStruct.VDCINDetector       = PMU_VDCINDET_ENABLE;
        LowPower_InitStruct.VDDDetector         = PMU_VDDDET_DISABLE;
    }
    else 
    {
        LowPower_InitStruct.AHBPeriphralDisable = (MISC2_HCLKEN_DMA     \
                                                |MISC2_HCLKEN_GPIO    \
                                                |MISC2_HCLKEN_CRYPT);
        LowPower_InitStruct.LCDPower            = PMU_LCDPWER_ON;

        LowPower_InitStruct.APBPeriphralDisable = PMU_APB_ALL;
        LowPower_InitStruct.BGPPower            = PMU_BGPPWR_OFF;
        LowPower_InitStruct.COMP1Power          = PMU_COMP1PWR_ON;//PMU_COMP1PWR_OFF;
        LowPower_InitStruct.COMP2Power          = PMU_COMP2PWR_OFF;
        LowPower_InitStruct.AVCCPower           = PMU_AVCCPWR_ON;
        LowPower_InitStruct.TADCPower           = PMU_TADCPWR_OFF;
        LowPower_InitStruct.VDCINDetector       = PMU_VDCINDET_ENABLE;
        LowPower_InitStruct.VDDDetector         = PMU_VDDDET_DISABLE;
    }
    RTC_PLLDIVOutputCmd(DISABLE);
    MComp1Init();//电池状态监测初始化
    WDT_Disable();
    PMU_EnterSleep_LowPower(&LowPower_InitStruct);

 
        //待调整 开E2P
    CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
    GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
    GPIOC->IE &= ~(BIT4|BIT5|BIT6);
    GPIOC->DAT |= (BIT6);
    
}

void gpio_configuration_batter(void)
{   
    GPIOA->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT15);
    GPIOA->IE &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|/*BIT7||BIT2*/
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT15);

    GPIOB->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOB->IE &= ~(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);

    GPIOC->OEN |= (BIT0|BIT1|BIT2|BIT3| /*BIT4|BIT5|BIT6|*/ BIT7| // bit6 e2p电源 不关闭
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOC->IE &= ~(BIT0|BIT1|BIT2|BIT3| /*BIT4|BIT5|BIT6|*/ BIT7|
                    BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);  

    GPIOD->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOD->IE &= ~(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);

    GPIOE->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOE->IE &= ~(BIT0|BIT1|BIT2|BIT3|BIT4|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);

    GPIOF->OEN |= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
    GPIOF->IE &= ~(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|
                    BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);

    GPIOE->OEN |= (0x00000001 << 5);// powercheck
    GPIOE->IE  |= (0x00000001 << 5);
    
    GPIOE->OEN |= BIT5 | BIT9;  //电源检测和电池检测口禁止输出
    GPIOE->IE &= ~(BIT5 | BIT9);//电源检测和电池检测口禁止输入
    
    //卡表功能引脚不启用，做输出低状态
    GPIOA->OEN &= ~BIT13; 
    GPIOA->IE &= ~BIT13;
    GPIOA->DAT &= ~BIT13;
    
    GPIOB->OEN &= ~BIT3; 
    GPIOB->IE &= ~BIT3;
    GPIOB->DAT &= ~BIT3;
    
    GPIOF->OEN &= ~BIT0; 
    GPIOF->IE &= ~BIT0;
    GPIOF->DAT &= ~BIT0;
    
    GPIOE->OEN &= ~(BIT4 | BIT8); 
    GPIOE->IE &= ~(BIT4 | BIT8);
    GPIOE->DAT &= ~(BIT4 | BIT8);

}

void gpio_configuration_AC(void)
{
    //PMU_INTConfig(PMU_INT_IOAEN, DISABLE);
    
    GPIOC->OEN &= ~BIT6; // V_EEP 
    GPIOC->IE &= ~BIT6;
    GPIOC->ATT &= ~BIT6;
    GPIOC->DAT |= BIT6;
    
    GPIOB->OEN &= ~BIT8; // V_FLASH
    GPIOB->IE &= ~BIT8;
    GPIOB->ATT &= ~BIT8;
    GPIOB->DAT &= ~BIT8;

    GPIOA->OEN |= BIT15; // 计量串口输入
    GPIOA->IE |= BIT15;
  
    GPIOA->OEN |= BIT14; // 开盖输入
    GPIOA->IE |= BIT14;
   
    GPIOC->OEN &= ~BIT9;//plc_rst
    GPIOC->IE &= ~BIT9;
    GPIOC->ATT |= (BIT9 | BIT10);
    GPIOC->DAT |= BIT9;//RST 高
    GPIOC->OEN |= BIT10;//plc_event，高阻态
    GPIOC->IE |= BIT10;
    
    GPIOF->OEN |= BIT1;//plc_STA，高组态
    GPIOF->IE |= BIT1;
    GPIOF->ATT |= BIT1;
    
    GPIOB->ATT |= BIT0;//plcrx
    GPIOB->ATT |= BIT6;//plctx
    
    
    GPIOB->ATT |= BIT9 | BIT10 | BIT11 | BIT12;//FLASH开漏
    

    
    GPIOA->OEN |=BIT2; 
    GPIOA->IE |=BIT2;
    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //使能外部IO口唤醒
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    
    
}

static uint8_t IOState(void *ptr)
{
    stategrid_board_state_t *state = ptr;

    if (state == NULL) {
        return 1;
    }

    if(nd_board_cover_closed())
    {
        if(state->meter_cover_cnt < 50)
        {
            state->meter_cover_cnt++;
        }
    }
    else
    {
        if(state->meter_cover_cnt > 0)
        {
            state->meter_cover_cnt--;
        }
    }
/*
    if(POWER_ABNORMAL_GET_DIN() == POWER_ABNORMAL_LEVE)
    {
        if(power_abnormal_cnt < 50)
        {
            power_abnormal_cnt++;
        }
    }
    else
    {
        if(power_abnormal_cnt > 0)
        {
            power_abnormal_cnt--;
        }
    }
*/    
    return 1;
}


void IOStateCheckInit(stategrid_board_state_t *state)
{
   struct tm_list_init_t tm;

   if (state == NULL) {
       return;
   }

   tm.timeout = 2;
   tm.tickpara = state;
   tm.tickfunc = IOState;
   tm.tm_id = METER_COVER_TM;
   tm.stoped = false;

   ndv_timer_create(&tm);

   state->meter_cover_cnt = 0;

}

void IOStateCheck(stategrid_context_t * instance, void * pipeline)
{
    struct mbase_control_block_t* mbase_control_block;
    mbase_control_block = ((stategrid_context_t *)instance)->mbase;
    GPIO_InitType GPIO_InitStruct;
    
    
    nd_board_cover_input_enable();


    if( ((stategrid_context_t *)instance)->ac_power_status == AC_POWER_OFF )
    {
        //开盖
        if(nd_board_cover_closed())
        {
            nd_mbase_CoverOpenS_status(mbase_control_block) = EVENT_RESET;
        }
        else
        {
            nd_mbase_CoverOpenS_status(mbase_control_block) = EVENT_SET;
        }
        
        if(nd_mbase_power_abnormal_tick(mbase_control_block) <= 5)
        {
            nd_mbase_power_abnormal_tick(mbase_control_block)++;
            
            //开启电源异常检测
            CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
            GPIOC->OEN &= ~BIT6; 
            GPIOC->IE &= ~BIT6;
            GPIOC->DAT |= BIT6;
            
            GPIOC->OEN |= BIT8; 
            GPIOC->IE |= BIT8;
            
            //mcu_delay_us(TEST_tmpU32);
            
            if((GPIOC->STS & BIT8) == 0)    //检测到信号
            {
                if(nd_mbase_power_abnormal_tick1(mbase_control_block) < 3)   
                {            
                    nd_mbase_power_abnormal_tick1(mbase_control_block)++;
                    if(nd_mbase_power_abnormal_tick1(mbase_control_block) >= 3)
                    {                   
                        nd_mbase_PowerAbnormalS_status(mbase_control_block) = EVENT_SET;  //电源异常发生
                    }
                }
            }
            else
            {
                nd_mbase_power_abnormal_tick1(mbase_control_block) = 0;
                nd_mbase_PowerAbnormalS_status(mbase_control_block) = EVENT_RESET;
            }
            
            if(nd_mbase_power_abnormal_tick(mbase_control_block) >= 5)
            {
//                GPIOC->DAT &= ~BIT6;
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
                GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_All;
                GPIOBToF_Init(GPIOC, &GPIO_InitStruct);
                CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), DISABLE);
            }
        }
    }
    else
    {
        if(instance->board->meter_cover_cnt==50)
        {
            nd_mbase_CoverOpenS_status(mbase_control_block) = EVENT_RESET;
        }
        else if(instance->board->meter_cover_cnt==0)
        {
            nd_mbase_CoverOpenS_status(mbase_control_block) = EVENT_SET;
        }
    }
    
}



/*
void main(void)
{    
    mcu_init(__NULL);
    
    dev_init();
        
    
    while(1)
    {
        mcu_clear_watchdog();
        
        if(sec_flag == 0x55)
        {
             Electrical_measurement();
             
             sec_flag = 0;
        }
    }
}
*/
