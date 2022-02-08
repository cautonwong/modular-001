#include "cpu_surport.h"
#include "nidavellir.h"
#include "lib_conf.h"
#include "time_link.h"
#include "uart_driver.h"
#include "pin_function_def.h"

extern const struct driver_ops_t  CODE eeprom_driver;
extern const struct bus_ops_t iic_bus;
extern const struct bus_ops_t spi_mode3;
extern const struct driver_ops_t  CODE uart_driver;
extern const struct driver_ops_t rn8209_drv;
extern const struct driver_ops_t nor_flash_driver;
extern const struct bus_ops_t mcu_uart_bus_ops;
extern void esam_698_init(void);

static uint8_t fifo_buf[32];
static struct fifo_t irq_fifo;
static uint8_t meter_cover_cnt;

const struct dev_info_t eeprom_dev=
{
    DEV_EEPROM,
    1,
    0xFFFF,
    0,
    0,
    &iic_bus,
    &eeprom_driver,
    __NULL,
    __NULL,
    __NULL,
    __NULL,
};

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

const UART_InitType  Rn8209_Uart_InitType = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    4800,
};



const struct dev_info_t rn8209_dev=
{
    DEV_EMU,
    0,
    0x0000,
    0,
    0,
    &mcu_uart_bus_ops,
    &rn8209_drv,
    __NULL,
    __NULL,
    __NULL,
    &Rn8209_Uart_InitType,
};


const struct dev_info_t nor_flash_dev=
{
    DEV_FLASH,
    0,
    0x7FFFFF,
    0,
    0,
    &spi_mode3,
    &nor_flash_driver,
    __NULL,
    __NULL,
    __NULL,
    __NULL,
};


void RTC_Second(void)
{
    fifoBuf_putByte(&irq_fifo, IRQ_RTC);
}

static uint8 wpulse_light_flag = 0;
static uint8 wpulse_light_open = 0;
static uint8 wpulse_light_open_flag = 0;
static uint8 wplse_light_width = 0;

static uint8 key_keep_flag = 0;
static uint8 key_keep_width = 0;
void IO_Interrupt(void)
{
   uint16 cnt;
    
    if( PMU_GetIOAINTStatus(GPIO_Pin_8))   //有功脉冲
    {
        PMU_ClearIOAINTStatus(GPIO_Pin_8);
        GPIOF->OEN &=~BIT1;
        GPIOF->IE &=~BIT1; 
        GPIOF->ATT &=~BIT1; 
        IO_DAT( GPIOF_DAT, 1) = 1;
        wpulse_light_open = 0x5a;
        wpulse_light_flag = 0;
    }

    if(GPIOA->INT & BIT2)  // 按键
    {
        
        key_keep_flag = 0x5a;
        key_keep_width = 0;
        
        //// 停电下按键消抖
        if(ANA->COMPOUT & ANA_STATUS_VDCINDROP)
        {
             key_keep_flag = 0; // 避免停电下 tick被打开 发两次 按键消息
             if(ANA->COMPOUT & BIT2)//有电池
             {
                 for(cnt = 0; cnt< 10; cnt++)  
                 {
                    if(((GPIOA->STS & BIT2) != 0))
                    {
                        break;
                    }
                 }
                 if (cnt >= 10)
                 {
                    fifoBuf_putByte(&irq_fifo, IRQ_PMU);
                 }
             }
        }
        fifoBuf_putByte(&irq_fifo, IRQ_PMU);
        PMU->IOAINTSTS = BIT2;
    }
}

extern  uint32 over_Imax;
extern void Energy_Pluse_Add(void);

void timing_ftr(void)
{
      uint8 dly1,dly2=0;
      
      if(over_Imax == 0x55)
      {
              dly1 = 6;
              dly2 = 14;
      }
      else
      {
              dly1 = 10;
              dly2 = 30;
      }
      /******************** 消抖 ********************/ 
      if(wpulse_light_open == 0x5a)
      {
              if((GPIOA->STS & BIT8) == BIT8)//消抖    
              {
                      wpulse_light_open_flag++;
              }
              else
              {
                      wpulse_light_open_flag=0;
              }
              if(wpulse_light_open_flag >= dly1)//20ms
              {

                      wpulse_light_open = 0;
                      wpulse_light_open_flag = 0;
                      wpulse_light_flag = 0x5A;
                      wplse_light_width = 0;
                      
              }
      }
      
      /******************** 脉宽80ms ********************/ 
      if(wpulse_light_flag == 0x5a)
      {
          wplse_light_width++;
          if(wplse_light_width > dly2)
          {
               GPIOF->OEN &=~BIT1;
               GPIOF->IE &=~BIT1;
               GPIOF->ATT &=~BIT1;
               IO_DAT( GPIOF_DAT, 1) = 0;
               wplse_light_width = 0;
               wpulse_light_flag = 0;
               Energy_Pluse_Add();
          }
          else
          {
              GPIOF->OEN &=~BIT1;
              GPIOF->IE &=~BIT1;
              GPIOF->ATT &=~BIT1; 
              IO_DAT( GPIOF_DAT, 1) = 1;
          }
      }
}

   

void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , uint8_t chl_type)
{
    struct uart_chl_init_t chl_init;
    
    switch(chl_type)
    {
        case UART_RS4851_CHL:
             chl_init.uart_cfg = (const void*)&Uart_InitType_485;
             chl_init.mcu_uart_index = UART_RS4851_AT_MCU_UART_INDEX;
             break;
        case UART_IR_CHL:
             chl_init.uart_cfg = (const void*)&Uart_InitType_ir;
             chl_init.mcu_uart_index = UART_IR_AT_MCU_UART_INDEX;
            break;
        case UART_MODULE_CHL:
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

void irq_system_init(void)
{
    fifoBuf_init(&irq_fifo, fifo_buf, 32);
    
    ndv_isr_handle_regsit(RTC_Second , IRQ_RTC );
    ndv_isr_handle_regsit(IO_Interrupt , IRQ_PMU );
    ndv_isr_handle_regsit(timing_ftr , IRQ_TMR3 );
    
    __enable_irq();
    
}

void irq_transform_to_instance(struct instance_t* instance)
{
    uint8_t fifo_irq = 0;

#ifdef DEBUG
    if(fifoBuf_getUsed(&irq_fifo) >0)
#else
    while(fifoBuf_getUsed(&irq_fifo) >0)
#endif
    {
       fifo_irq = fifoBuf_getByte(&irq_fifo);
       
       switch(fifo_irq)
       {
           case IRQ_RTC:
                fifoBuf_putByte(&instance->pipe_rtc, IRQ_RTC);
                break;
             
           case IRQ_PMU:
                fifoBuf_putByte(&instance->pipe_key, IRQ_PMU);
                break;                 
           default:
                break;
       }
    }
}

void IOStateCheckInit(void);

void dev_init(void)
{
    mcu_main_power_on_config();
    
    ndv_timer_init();
	
    IOStateCheckInit();
    
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
    
    DISPLAY_KEY_INPUT();
    
    rn8209_dev.dev_driver_ops->dev_initialize(&rn8209_dev);
    
    nor_flash_dev.dev_driver_ops->dev_initialize(&nor_flash_dev);
    
    rn8209_dev.dev_driver_ops->dev_control(&rn8209_dev , AC_POWER_ON , 0);
    
    esam_698_init();

}

//关闭用电大户外设的电源，比如背光，LED，串口，高速定时器，tick
 void device_power_close(struct instance_t* instance)
 {
 
   
   
 
 }


//打开串口，高速定时器，tick，初始化IO上电配置
 void device_power_open(struct instance_t* instance)
 {
 
 
 
 
 }

//初始化IO低功耗配置，初始化系统低功耗配置，准备进入sleep模式。
void cpu_io_lowpower_cfg_chk(struct instance_t* instance)
{







}

//此处放sleep函数，以及唤醒后的系统基本处理
void cpu_sleep_manage(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    PMU_LowPWRTypeDef LowPower_InitStruct;
    GPIO_InitType GPIO_InitStruct;
    
    if((mbase_control_block->Key & F_KeyAwake) == 0)
    {
        //LCD_InitType lcd_init_para;
        //LCD_IOInitType lcd_io_para;
        uint32 lcd_tmp[20] = {0,};

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
        MISC2->HCLKEN |= (uint32)BIT6;
        memcpy((uint8*)(&LCD->FB[0]), (uint8 *)&lcd_tmp[0], sizeof(lcd_tmp));
    //    lcd_init(__NULL , POWER_OFF_MODE);

        /*------------------ Forbidden all GPIOs ------------------*/
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
        GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_All;
        //GPIOA_Init(GPIOA, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOB, &GPIO_InitStruct);
    //    if (hfp_system_state.power_off_sencond > 5)
        {
            GPIOBToF_Init(GPIOC, &GPIO_InitStruct);
        }
        GPIOBToF_Init(GPIOD, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
        GPIOBToF_Init(GPIOF, &GPIO_InitStruct);
        
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

        LowPower_InitStruct.AHBPeriphralDisable = PMU_AHB_ALL;
        LowPower_InitStruct.LCDPower            = PMU_LCDPWER_OFF;
        LowPower_InitStruct.APBPeriphralDisable = PMU_APB_ALL;
        LowPower_InitStruct.BGPPower            = PMU_BGPPWR_OFF;
        LowPower_InitStruct.COMP1Power          = PMU_COMP1PWR_ON;//PMU_COMP1PWR_OFF;
        LowPower_InitStruct.COMP2Power          = PMU_COMP2PWR_OFF;
        LowPower_InitStruct.AVCCPower           = PMU_AVCCPWR_OFF;
        LowPower_InitStruct.TADCPower           = PMU_TADCPWR_OFF;
        LowPower_InitStruct.VDCINDetector       = PMU_VDCINDET_ENABLE;
        LowPower_InitStruct.VDDDetector         = PMU_VDDDET_DISABLE;
        RTC_PLLDIVOutputCmd(DISABLE);
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
        LowPower_InitStruct.AVCCPower           = PMU_AVCCPWR_OFF;
        LowPower_InitStruct.TADCPower           = PMU_TADCPWR_OFF;
        LowPower_InitStruct.VDCINDetector       = PMU_VDCINDET_ENABLE;
        LowPower_InitStruct.VDDDetector         = PMU_VDDDET_DISABLE;

        RTC_PLLDIVOutputCmd(DISABLE);
        MComp1Init();//电池状态监测初始化
    }
    WDT_Disable();
    PMU_EnterSleep_LowPower(&LowPower_InitStruct);
}



static uint8 IOState(void *ptr)
{
    if(METER_COVER_GET_DIN() == METER_COVER_OPEN_LEVE)
    {
        if(meter_cover_cnt < 50)
        {
            meter_cover_cnt++;
        }
    }
    else
    {
        if(meter_cover_cnt > 0)
        {
            meter_cover_cnt--;
        }
    }
    
    return 1;
}


void IOStateCheckInit(void)
{
   struct tm_list_init_t tm;

   tm.timeout = 2;
   tm.tickpara = 0;
   tm.tickfunc = IOState;
   tm.tm_id = METER_COVER_TM;
   tm.stoped = false;
   
   ndv_timer_create(&tm);

   meter_cover_cnt = 0;

}



void IOStateCheck(struct instance_t* instance, struct pipeline_t* pipeline)
{
    
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    METER_COVER_INPUT();
    if(meter_cover_cnt==50)
    {
        mbase_control_block->meter_cover_sta = EVENT_SET;
    }
    else if(meter_cover_cnt==0)
    {
        mbase_control_block->meter_cover_sta = EVENT_RESET;
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
