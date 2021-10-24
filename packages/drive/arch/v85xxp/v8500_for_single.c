#include "target.h"
#include "core_cm0.h"
#include "lib_cortex.h"
#include "lib_uart.h"
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "lib_conf.h"

#define   IO_DAT(IOX, PINX)  *((volatile unsigned long *)((((uint32_t)IOX) & 0xF0000000)\
                              +0x2000000+((((uint32_t)IOX) &0xFFFFF)<<5)+(PINX<<2)))

#define WDTPASS_KEY 0xAA5555AA
#define WDTCLR_KEY  0x55AAAA55


const uint32 CODE freq[8] = { 26214400,13107200,6553600,3276800,1638400,819200,409600,204800, };
static uint32 get_system_clk(void)
{
    uint32 temp;

    /* Get PLLL Frequency */
    temp = ANA->REG9 & ANA_REG9_PLLLSEL;

    if (temp < 8)
    {
        temp = freq[temp];
    }
    else
    {
        temp = 3686400;
    }


    if (!(ANA->REG3 & ANA_REG3_PLLLPDN))
    {
        temp = 32768;// 调整
    }

    return temp;
}


void cm0_systick_init(uint8 cmd)
{
    uint32 cpu_clk;
    
    if(cmd)
    {
        cpu_clk = get_system_clk();
        SysTick->LOAD = cpu_clk / 100 - 1UL;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
        NVIC_EnableIRQ( SysTick_IRQn );
    }
    else
    {
        SysTick->CTRL = 0;
        NVIC_DisableIRQ( SysTick_IRQn );
    }
}

static void sysclk_at_3m2768(uint16 flag)
{

    
    CLK_InitTypeDef CLK_Struct;
    if (0x5aa5 == flag)
    {
      CLK_Struct.ClockType = CLK_TYPE_AHBSRC \
                            |CLK_TYPE_PLLL   \
                            |CLK_TYPE_HCLK   \
                            |CLK_TYPE_PCLK;
      CLK_Struct.AHBSource      = CLK_AHBSEL_LSPLL;
      CLK_Struct.PLLL.Frequency = CLK_PLLL_3_2768MHz;
      CLK_Struct.PLLL.Source    = CLK_PLLLSRC_XTALL;
      CLK_Struct.PLLL.State     = CLK_PLLL_ON;
      CLK_Struct.HCLK.Divider   = 1;
      CLK_Struct.PCLK.Divider   = 1;
      CLK_ClockConfig(&CLK_Struct);
      
    }
}

void  v85xxp_clear_watchdog(void)
{
  PMU->WDTCLR = WDTCLR_KEY;
}
/* 实测试10us到50ms误差在5%以内。@ 147435*/
/* 复位运行 3685875*/
/* 上电全速运行 14743500*/
/* 唤醒运行7371750*/
/* 休眠运行327687*/
/* 实测试10us到50ms误差在5%以内。@ 147435*/
uint8 v85xxp_delay_us(volatile uint16 count)
{
    uint32 timeout = 1000000;
    
    if(count == 0)
    {
        return true;
    }
    
    MISC2->PCLKEN |= (0x00000001 << 12);

    /* Disable timer */
    TMR1->CTRL = 0x00;
    /* clear interrupt status */
    TMR1->INT = TMR_INT_INT;
    TMR1->RELOAD = 0;
    TMR1->VALUE  =  count*40; //待调整   
    TMR1->CTRL |= TMR_CTRL_EN;

    while(0 == (TMR1->INT & 0x00000001))
    {
        timeout--;
        if(timeout < 1)
        {
            TMR1->CTRL = 0x00;
            return false;
        }
    } 
   
    TMR1->CTRL = 0x00;
    return true;
}

void MComp1Init(void)
{
    GPIO_InitType GPIO_InitStruct;

    /* Forbidden Comparator1 N input pin, IOE9 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
    //    GPIOE->OEN |= 0xFFFF;       	                    
    //	GPIOE->IE &=~0xFFFF;
    ANA->CTRL = (2 << ANA_CTRL_CMP1DEB_Pos) | (3 << ANA_CTRL_COMP1_SEL_Pos);  //3个RTCCLK时钟周期消抖  双边沿
    //ANA->REG2  = 1 << ANA_REG2_REFSEL_CMP1_Pos; //比较电压  BGPREF=1.2V

    ANA->REG2 |= 1 << ANA_REG2_CMP1_SEL_Pos;    //comp1_n与REF

    //ANA->REG3 &= ~ANA_REG3_BGPPD;               //上电BGP
    ANA->REG3 |= ANA_REG3_CMP1PDN;              //开启CMP1
    ANA->INTEN |= ANA_INTEN_INTEN2;//开启comp1中断

    //    KHandlerTabCfg(ANA_IRQn,ANA_COMP1_Interrupt,2);
}


/*全速运行26M */
void sys_clock_normal(void) 
{
    CLK_InitTypeDef CLK_Struct;
    uint32_t Periphral=0;

    CLK_Struct.ClockType = CLK_TYPE_AHBSRC \
                        |CLK_TYPE_PLLL   \
                        |CLK_TYPE_HCLK   \
                        |CLK_TYPE_PCLK;
    CLK_Struct.AHBSource      = CLK_AHBSEL_LSPLL;
    CLK_Struct.PLLL.Frequency = CLK_PLLL_26_2144MHz;
    CLK_Struct.PLLL.Source    = CLK_PLLLSRC_XTALL;
    CLK_Struct.PLLL.State     = CLK_PLLL_ON;
    CLK_Struct.HCLK.Divider   = 1;
    CLK_Struct.PCLK.Divider   = 1;
    CLK_ClockConfig(&CLK_Struct);
    
    Periphral = CLK_APBPERIPHRAL_PMU | CLK_APBPERIPHRAL_RTC | CLK_APBPERIPHRAL_ANA \
                | CLK_APBPERIPHRAL_TIMER | CLK_APBPERIPHRAL_UART3 \
                | CLK_APBPERIPHRAL_UART4 | CLK_APBPERIPHRAL_UART5 \
                | CLK_APBPERIPHRAL_UART0;
    CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_LCD | CLK_AHBPERIPHRAL_GPIO), ENABLE);
    CLK_APBPeriphralCmd(Periphral , ENABLE);
    
    // 初始化上电检测?
    GPIOE->OEN |= (0x00000001 << 5);// powercheck
    GPIOE->IE  |= (0x00000001 << 5);
}


/***********************************************************
 * @function_name: v85xxp_init
 * @function_file: cpu.c
 * @描述:mcu初始化函数
 *
 * @参数:
 * @返回:
 * @作者:C
 **********************************************************/
void mcu_init(void *init_type)
{
    uint16 flag = 0x5aa5;
    uint32 val = 0;

    // 看门狗初始化
    WDT_SetPeriod(WDT_2_SECS);
    WDT_Enable();

    v85xxp_clear_watchdog();
    __disable_irq();

    /**
    复位之后首先设置CMPLVD_EN 、RTC_SAR_EN 、WDT_EN 、LCD_EN、 GPIO_EN
    */

    CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_LCD | CLK_AHBPERIPHRAL_GPIO), ENABLE);
    CLK_APBPeriphralCmd((CLK_APBPERIPHRAL_PMU | CLK_APBPERIPHRAL_RTC | CLK_APBPERIPHRAL_ANA), ENABLE);

    // 初始化上电检测?
    GPIOE->OEN |= (0x00000001 << 5);// powercheck
    GPIOE->IE |= (0x00000001 << 5);

    /**
    MCU第一次上电时，需要等待约???的起震时间，才可以使用PLL//待调整
    */
    for (val = 0; val < 20000; val++)
    {
        v85xxp_clear_watchdog();
        __NOP();
    }
    sysclk_at_3m2768(flag);
    
    sys_clock_normal();
    
    v85xxp_clear_watchdog();
    
    __enable_irq();

}



UART_TypeDef* UART_list[] =
{
    UART0,
    UART1,
    UART2,
    UART3,
    UART4,
    UART5,
};

const IRQn_Type CODE UART_IRQ[] =
{
    UART0_IRQn,
    UART1_IRQn,
    UART2_IRQn,
    UART3_IRQn,
    UART4_IRQn,
    UART5_IRQn,
};

const uint32 CODE clock_en[] =
{
    MISC2_PCLKEN_UART0_Msk,
    MISC2_PCLKEN_UART1_Msk,
    MISC2_PCLKEN_UART2_Msk,
    MISC2_PCLKEN_UART3_Msk,
    MISC2_PCLKEN_UART4_Msk,
    MISC2_PCLKEN_UART5_Msk,
};

uint8 mcu_uart_init(void* init_config, uint8 uart_index, uint8 infrared_enable)
{
    UART_TypeDef* UARTx;
    UART_InitType* InitType = (UART_InitType*)init_config;

    if (uart_index < 6)
    {

        MISC2->PCLKEN |= clock_en[uart_index];

        __NOP();
        __NOP();

        UARTx = UART_list[uart_index];

    }
    else
    {
        return false;
    }

    UART_DeInit(UARTx);

    UART_Init(UARTx, InitType);

    if (infrared_enable)
    {
        MISC2->PCLKEN |= MISC2_PCLKEN_MISC;

        MISC_IRCmd(MISC_IREN_TX5, ENABLE);

        MISC_IRDutyConfig(344, 344);
    }

    UARTx->CTRL = 0;

    NVIC_EnableIRQ(UART_IRQ[uart_index]);
    return true;
}

uint8 mcu_uart_close(uint8 uart_index)
{
    UART_TypeDef* UARTx;

    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];
    }
    else
    {
        return false;
    }

    UARTx->CTRL &= ~(UART_CTRL_TXEN | UART_CTRL_RXEN);
    MISC2->PCLKEN &= ~clock_en[uart_index];

    NVIC_DisableIRQ(UART_IRQ[uart_index]);
    return true;
}

void mcu_uart_recive_enable(uint8 ops_enable, size_t uart_index)
{
    UART_TypeDef* UARTx;
    uint8 tmp = 0;

    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];
        UARTx->INTSTS = UART_INTSTS_RXIF | UART_INTSTS_RXOVIF | UART_INTSTS_RXPEIF;
        if (ops_enable == 1)
        {
            UARTx->CTRL |= UART_CTRL_RXEN + UART_CTRL_RXIE;
        }
        else if (ops_enable == 2)
        {
            UARTx->CTRL |= UART_CTRL_RXEN;
        }
        else
        {
            UARTx->CTRL &= ~(UART_CTRL_RXEN + UART_CTRL_RXIE);
        }
        UARTx->STATE = BIT1 | BIT3 | BIT4;
        
        tmp = UARTx->DATA;
    }
}

int mcu_uart_send_byte(uint8 send_data, size_t uart_index)
{
    UART_TypeDef* UARTx;

    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];

        UARTx->DATA = send_data;
    }
}

int mcu_uart_recive_byte(uint8* byte , size_t uart_index)
{
    UART_TypeDef* UARTx;


    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];
        *byte = UARTx->DATA;
        if (UARTx->STATE & 0x00000002)
        {
            UARTx->STATE |= 0x00000002;
            return false;
        }
        if (UARTx->STATE & 0x00000008)
        {
            UARTx->STATE |= 0x00000008;
            return false;
        }
        if (UARTx->STATE & 0x00000010)
        {
            UARTx->STATE |= 0x00000010;
            return false;
        }
        return true;
    }
    return false;
}

void mcu_uart_send_enable(uint8 ops_enable, size_t uart_index)
{
    UART_TypeDef* UARTx;

    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];
        UARTx->INTSTS = UART_INTSTS_TXDONEIF;
        if (ops_enable == 1)
        {
            UARTx->CTRL |= (UART_CTRL_TXEN + UART_CTRL_TXDONEIE);
        }
        else if (ops_enable == 2)
        {
            UARTx->CTRL |= UART_CTRL_TXEN;
        }
        else
        {
            UARTx->CTRL &= ~(UART_CTRL_TXEN + UART_CTRL_TXDONEIE);
        }
        UARTx->STATE = BIT2 | BIT5;
    }
}

uint8 mcu_uart_set_baud(uint16 baud, uint8 uart_index)
{
    uint32	ClkDiv, pclk;
    UART_TypeDef* UARTx;

    if (uart_index < 6)
    {
        UARTx = UART_list[uart_index];
    }
    else
    {
        return false;
    }

    MISC2->PCLKEN &= ~clock_en[uart_index];

    pclk = get_system_clk();
    ClkDiv = pclk / baud;
    if ((pclk % baud) > (baud / 2))
    {
        ClkDiv++;
    }
    UARTx->BAUDDIV = ClkDiv;

    MISC2->PCLKEN |= clock_en[uart_index];
    return true;
}

int mcu_uart_get_tx_status(size_t uart_index)
{
    UART_TypeDef* UARTx;

    UARTx = UART_list[uart_index];

    if (UARTx->STATE & 0x00000020)
    {
        //UARTx->STATE = 0x00000020;
        UARTx->INTSTS = 0x00000020;
        return true;
    }
    return false;
}

int mcu_uart_get_rx_status(size_t uart_index)
{
    UART_TypeDef* UARTx;

    UARTx = UART_list[uart_index];

    if(UARTx->STATE & 0x00000002)
    {
            //UARTx->STATE = 0x00000002;
            UARTx->INTSTS = 0x00000002;
            return true;
    }
    return false;
}

uint8 mcu_spi_bus_init(uint8 spi_index, uint8 mode)
{

    MISC2->PCLKEN |= MISC2_PCLKEN_SPI2;

    __NOP();
    __NOP();

    SPI2->CTRL = 0x00;

    v85xxp_delay_us(10);
    // CS软件控制 /  前边沿建立数据 / sck空闲状态为高 / MSB先传输   / 主机/ apbclk 4分频
    SPI2->CTRL = BIT10 | BIT5 | BIT4 | BIT0;

    SPI2->MISC_ |= BIT8;

    SPI2->CTRL |= (0x00000001 << 15);

    v85xxp_delay_us(10);

    return true;
}


uint8 mcu_spi_send_byte(uint8 byte, uint8 spi_index)
{
    uint16 timeout = 50000; // 待调整
    uint8  rxd;


    while ((0 == (SPI2->MISC_ & SPI_MISC_TFE)) && (timeout > 0))
    {
        timeout--;
    }

    SPI2->TXDAT = byte;
    timeout = 60000;

    while ((SPI2->MISC_ & SPI_MISC_BSY_Msk) && (timeout > 0))
    {
        timeout--;
    }
    rxd = SPI2->RXDAT;
    /////
    if (timeout > 0)
    {
        return true;
    }
    //////
    rxd -= rxd;

    return false;
}

uint8 mcu_spi_recive_byte(uint8* byte, uint8 spi_index)
{
    uint16 timeout = 50000;
    uint8  rxd = 0;



    while ((0 == (SPI2->MISC_ & SPI_MISC_TFE)) && (timeout > 0))
    {
        timeout--;
    }

    SPI2->TXDAT = 0x00;
    timeout = 50000;

    while ((SPI2->MISC_ & SPI_MISC_BSY_Msk) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout > 0)
    {
        rxd = SPI2->RXDAT;
    }

    *byte = rxd;

    return true;
}





void mcu_timer_ctrl(uint32 period_ms, uint8 index, uint8 stop_or_start)
{
    TMR_InitType    TMR_InitStruct;

    if (index == 0)
    {
        MISC2->PCLKEN |= (0x00000001 << 12);

        /* Disable timer */
        TMR0->CTRL = 0x00;
        /* clear interrupt status */
        TMR0->INT = TMR_INT_INT;
        TMR0->RELOAD = 4680;
        TMR0->VALUE = 4680;

        if (stop_or_start)
        {
            TMR0->CTRL |= (TMR_CTRL_EN + TMR_CTRL_INTEN);
            NVIC_EnableIRQ(TMR0_IRQn);
        }
        else
        {
            TMR0->CTRL &= ~(TMR_CTRL_EN + TMR_CTRL_INTEN);

            NVIC_DisableIRQ(TMR0_IRQn);
        }


    }

    if (index == 3)
    {
        TMR_DeInit(TMR3);
        TMR_InitStruct.ClockSource = TMR_CLKSRC_INTERNAL;
        TMR_InitStruct.EXTGT = TMR_EXTGT_DISABLE;
        TMR_InitStruct.Period = 52429;
        TMR_Init(TMR3, &TMR_InitStruct);

        if (stop_or_start)
        {
            TMR_INTConfig(TMR3, ENABLE);  //Enable Timer0 interrupt
            CORTEX_SetPriority_ClearPending_EnableIRQ(TMR3_IRQn, 2);
            TMR_Cmd(TMR3, ENABLE);  //Enable Timer0
        }
        else
        {
            TMR_INTConfig(TMR3, DISABLE);  //Enable Timer0 interrupt
            CORTEX_SetPriority_ClearPending_EnableIRQ(TMR3_IRQn, 2);
            TMR_Cmd(TMR3, DISABLE);  //Enable Timer0
        }

    }

}

void  mcu_clear_watchdog(void)
{
    PMU->WDTCLR = WDTCLR_KEY;
}

/* 实测试10us到50ms误差在5%以内。@ 147435*/
/* 复位运行 3685875*/
/* 上电全速运行 14743500*/
/* 唤醒运行7371750*/
/* 休眠运行327687*/
/* 实测试10us到50ms误差在5%以内。@ 147435*/
uint8 mcu_delay_us(volatile uint16 count)
{
    uint32 timeout = 1000000;

    if (count == 0)
    {
        return true;
    }

    MISC2->PCLKEN |= (0x00000001 << 12);

    /* Disable timer */
    TMR1->CTRL = 0x00;
    /* clear interrupt status */
    TMR1->INT = TMR_INT_INT;
    TMR1->RELOAD = 0;
    TMR1->VALUE = count * 40; //待调整   
    TMR1->CTRL |= TMR_CTRL_EN;

    while (0 == (TMR1->INT & 0x00000001))
    {
        timeout--;
        if (timeout < 1)
        {
            TMR1->CTRL = 0x00;
            return false;
        }
    }

    TMR1->CTRL = 0x00;
    return true;
}



uint8 poweroff_test = 1;//改回来
uint8 ac_power_off_monitor(void)
{
#if 0 
    if (poweroff_test) // 改回来
    {
        return true;
    }
    else
    {
        return false;
    }

    static uint8 test;
    if (test == 0x5a)
    {
        return false;
    }

    else
    {
        return true;
    }
#endif    

    if (ANA_GetStatus(ANA_STATUS_VDCINDROP))     //掉电
    {
        return true;
    }
    return false;




}

uint8 ac_power_on_monitor(void)
{
#if 0 
    if (poweroff_test) // 改回来
    {
        return false;
    }
    else
    {
        return true;
    }

    static uint8 test;

    if (test == 0x5a)
    {
        return false;
    }

    else
    {
        return true;
    }
#endif
    if (ANA_GetStatus(ANA_STATUS_VDCINDROP) == 0)
    {
        return true;
    }
    return false;


}

void mcu_main_frequency_mode(uint8 clk_mode)
{
    CLK_InitTypeDef CLK_Struct;
    uint32_t Periphral = 0;

    if (clk_mode)
    {
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

void mcu_enter_lowpower_then_sleep(void)
{
    GPIO_InitType GPIO_InitStruct;
    PMU_LowPWRTypeDef LowPower_InitStruct;
    uint32 lcd_tmp[20] = {0,};

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
    memcpy((uint8*)(&LCD->FB[0]), (uint8*)&lcd_tmp[0], sizeof(lcd_tmp));
    //lcd_init(__NULL, 0);

    /*------------------ Forbidden all GPIOs ------------------*/
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_All;
    //GPIOA_Init(GPIOA, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOB, &GPIO_InitStruct);
    /*if (hfp_system_state.power_off_sencond > 5)
    {
        GPIOBToF_Init(GPIOC, &GPIO_InitStruct);
    }*/
    GPIOBToF_Init(GPIOD, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
    GPIOBToF_Init(GPIOF, &GPIO_InitStruct);


    /*       GPIOA->DAT &= ~(BIT0|BIT1|BIT3|BIT4|BIT5|BIT6|BIT7|
                       BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);*/
    GPIOA->OEN |= (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 |
        BIT8 | BIT9 | BIT10 | BIT11 | BIT12 | BIT13 | BIT14 | BIT15);
    GPIOA->IE &= ~(BIT0 | BIT1 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 |
        BIT8 | BIT9 | BIT10 | BIT11 | BIT12 | BIT13 | BIT14 | BIT15);


    GPIOA->OEN |= BIT14;
    GPIOA->IE |= BIT14;
    GPIOA->DAT |= BIT2;
    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //使能外部IO口唤醒
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);
    CORTEX_SetPriority_ClearPending_EnableIRQ(PMU_IRQn, 0); //优先级高

    /*------------------ Low power configuration ------------------*/
/*        if((Flag.Disp & F_DispOff) == 0)
        {
                LowPower_InitStruct.AHBPeriphralDisable = (MISC2_HCLKEN_DMA     \
                                                        |MISC2_HCLKEN_GPIO    \
                                                        |MISC2_HCLKEN_CRYPT);
                LowPower_InitStruct.LCDPower            = PMU_LCDPWER_ON;
        }
        else*/
    {
        LowPower_InitStruct.AHBPeriphralDisable = PMU_AHB_ALL;
        LowPower_InitStruct.LCDPower = PMU_LCDPWER_OFF;
    }
    LowPower_InitStruct.APBPeriphralDisable = PMU_APB_ALL;
    LowPower_InitStruct.BGPPower = PMU_BGPPWR_OFF;
    LowPower_InitStruct.COMP1Power = PMU_COMP1PWR_ON;//PMU_COMP1PWR_OFF;
    LowPower_InitStruct.COMP2Power = PMU_COMP2PWR_OFF;
    LowPower_InitStruct.AVCCPower = PMU_AVCCPWR_OFF;
    LowPower_InitStruct.TADCPower = PMU_TADCPWR_OFF;
    LowPower_InitStruct.VDCINDetector = PMU_VDCINDET_ENABLE;
    LowPower_InitStruct.VDDDetector = PMU_VDDDET_DISABLE;
    RTC_PLLDIVOutputCmd(DISABLE);
    MComp1Init();//电池状态监测初始化
    WDT_Disable();
    PMU_EnterSleep_LowPower(&LowPower_InitStruct);
}



void mcu_exit_sleep_then_enter_normal(void)
{

}


void mcu_main_power_on_config(void)
{
    cm0_systick_init(1);

}

void mcu_main_power_off_config(void)
{
    cm0_systick_init(0);
    
    
}



int emu_uart_init(size_t index)
{
    UART_InitType* InitType = (UART_InitType*)index;
    
    mcu_uart_init(InitType, 3, 0);

}

const struct bus_ops_t mcu_uart_bus_ops = 
{
    BUS_UART,
    3,
    emu_uart_init,
    mcu_uart_send_enable,
    mcu_uart_send_byte,
    mcu_uart_recive_enable,
    mcu_uart_recive_byte,
    mcu_uart_get_tx_status,
    mcu_uart_get_rx_status,
    __NULL, 
};


#ifndef  ASSERT_NDEBUG
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_errhandler error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_errhandler error line source number
  * @retval None
  */
void assert_errhandler(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}











#endif