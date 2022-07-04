#include "target.h"
#include "core_cm0.h"
#include "lib_cortex.h"
#include "lib_uart.h"
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "lib_conf.h"



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
        temp = 32768;// Ë∞ÉÊï¥
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


void MComp1Init(void)
{
    GPIO_InitType GPIO_InitStruct;

    /* Forbidden Comparator1 N input pin, IOE9 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIOBToF_Init(GPIOE, &GPIO_InitStruct);
    //    GPIOE->OEN |= 0xFFFF;       	                    
    //	GPIOE->IE &=~0xFFFF;
    ANA->CTRL = (2 << ANA_CTRL_CMP1DEB_Pos) | (3 << ANA_CTRL_COMP1_SEL_Pos);  //3‰∏™RTCCLKÊó∂ÈíüÂë®ÊúüÊ∂àÊäñ  ÂèåËæπÊ≤ø
    //ANA->REG2  = 1 << ANA_REG2_REFSEL_CMP1_Pos; //ÊØîËæÉÁîµÂéã  BGPREF=1.2V

    ANA->REG2 |= 1 << ANA_REG2_CMP1_SEL_Pos;    //comp1_n‰∏éREF

    //ANA->REG3 &= ~ANA_REG3_BGPPD;               //‰∏äÁîµBGP
    ANA->REG3 |= ANA_REG3_CMP1PDN;              //ÂºÄÂêØCMP1
    ANA->INTEN |= ANA_INTEN_INTEN2;//ÂºÄÂêØcomp1‰∏≠Êñ≠

    //    KHandlerTabCfg(ANA_IRQn,ANA_COMP1_Interrupt,2);
}


/*ÂÖ®ÈÄüËøêË°å26M */
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
    
    // ÂàùÂßãÂåñ‰∏äÁîµÊ£ÄÊµã?
    GPIOE->OEN |= (0x00000001 << 5);// powercheck
    GPIOE->IE  |= (0x00000001 << 5);
}



/* ÂÆûÊµãËØ?0usÂà?0msËØØÂ∑ÆÂú?%‰ª•ÂÜÖ„ÄÇ@ 147435*/
/* Â§ç‰ΩçËøêË°å 3685875*/
/* ‰∏äÁîµÂÖ®ÈÄüËøêË°?14743500*/
/* Âî§ÈÜíËøêË°å7371750*/
/* ‰ºëÁú†ËøêË°å327687*/
/* ÂÆûÊµãËØ?0usÂà?0msËØØÂ∑ÆÂú?%‰ª•ÂÜÖ„ÄÇ@ 147435*/
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
    TMR1->VALUE = count * 40; //ÂæÖË∞ÉÊï?  
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

static void RTCWriteEnable(void)
{
    while(RTC->CE & RTC_CE_BSY)            /* µ±wr_busy=1 ±£¨RTCºƒ¥Ê∆˜–¥≤Ÿ◊˜√¶ */
    {
    }
    
    RTC->PWD = RTCPWD_KEY;
    RTC->CE = RTCCE_SETKEY;       
	     
    mcu_delay_us(500);                  /* delay needed! */
    while(RTC->CE & RTC_CE_BSY)            /* µ±wr_busy=1 ±£¨RTCºƒ¥Ê∆˜–¥≤Ÿ◊˜√¶ */
    {
    }
}

static void RTCWriteDisable(void)
{

    RTC->PWD = RTCPWD_KEY;
    RTC->CE = RTCCE_CLRKEY;

  	/* RTC Write disable */
    mcu_delay_us(500);
    while(RTC->CE & RTC_CE_BSY)            /* µ±wr_busy=1 ±£¨RTCºƒ¥Ê∆˜–¥≤Ÿ◊˜√¶ */
    {
    }
}

uint8 init_v85xxp_rtc( uint8 power_mode )
{
    
    MISC2->PCLKEN |= MISC2_PCLKEN_RTC;
    
    RTCWriteEnable();
    
    mcu_delay_us(500);

    RTC->INTSTS = 0xFF; 
    
    mcu_delay_us(500);

    RTC->WKUSEC = 0;// ≈‰÷√√ø√Î÷–∂œ
    RTC->INTEN = BIT0;    
    
    mcu_delay_us(500);

    NVIC_EnableIRQ ( RTC_IRQn );
    
    
    //switch(power_mode)
    //{
    //    case POWER_ON_MODE:
    //          rtc_out(1 , rtc_hal->multifunction_port);
    //          break;
    //    case RESET_MODE:
    //          rtc_multifunction_port.tmr_id = tick_timer.create(end_multifunction_port_driver, &rtc_multifunction_port.operate_pin, 8, true);
              /*◊¢“‚£¨’‚¿Ô√ª”–break*/
    //    default:
    //          rtc_out(0 , rtc_hal->multifunction_port);
    //          break;
    
    
    
    RTCWriteDisable();
    return true;
}

uint8_t read_mcu_rtc(uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t time_data[7];
    uint8_t time_check[7];
    uint32_t dummy_data = 0;

    /* Wait until the RTC registers be synchronized */
    RTC_WaitForSynchro();
    
    /* Dummy read-operation to RTC->LOAD register */
    dummy_data = RTC->LOAD;
    dummy_data += 1;
    /* Wait until the RTC registers be synchronized */
    RTC_WaitForSynchro();
    
    
    do
    {
        time_data[0] = RTC->SEC;
        time_data[1] = RTC->MIN;
        time_data[2] = RTC->HOUR;
        time_data[3] = RTC->DAY;
        time_data[4] = RTC->MON;
        time_data[5] = RTC->YEAR;
        time_data[6] = RTC->WEEK;
        
        mcu_delay_us(10);
        
        time_check[0] = RTC->SEC;
        time_check[1] = RTC->MIN;
        time_check[2] = RTC->HOUR;
        time_check[3] = RTC->DAY;
        time_check[4] = RTC->MON;
        time_check[5] = RTC->YEAR;
        time_check[6] = RTC->WEEK;
        
    }while(mem_compare(time_data, time_check, 7));
       
    memcpy(buf , &time_data[0] , len);
    return true;
}

uint8_t write_mcu_rtc(uint8_t addr, uint8_t *buf, uint16 len)
{
    uint8 time_data[7];
    
    RTCWriteEnable();
    
    memcpy(time_data , buf , 7);

    RTC->WEEK = time_data[6];
    mcu_delay_us(1000);
    RTC->YEAR = time_data[5];
    mcu_delay_us(1000);
    RTC->MON = time_data[4];
    mcu_delay_us(1000);
    RTC->DAY = time_data[3];
    mcu_delay_us(1000);
    RTC->HOUR = time_data[2];
    mcu_delay_us(1000);
    RTC->MIN = time_data[1];
    mcu_delay_us(1000);
    RTC->SEC = time_data[0];
    mcu_delay_us(1000);
    
    RTCWriteDisable();
    return true;
}
/***********************************************************
 * @function_name: v85xxp_init
 * @function_file: cpu.c
 * @ÊèèËø∞:mcuÂàùÂßãÂåñÂáΩÊï∞
 *
 * @ÂèÇÊï∞:
 * @ËøîÂõû:
 * @‰ΩúËÄÖ:C
 **********************************************************/
void mcu_init(void *init_type)
{
    uint16 flag = 0x5aa5;
    uint32 val = 0;

    // ÁúãÈó®ÁãóÂàùÂßãÂåñ
    WDT_SetPeriod(WDT_2_SECS);
    WDT_Enable();

    v85xxp_clear_watchdog();
    __disable_irq();

    /**
    Â§ç‰Ωç‰πãÂêéÈ¶ñÂÖàËÆæÁΩÆCMPLVD_EN „ÄÅRTC_SAR_EN „ÄÅWDT_EN „ÄÅLCD_EN„ÄÅ GPIO_EN
    */

    CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_LCD | CLK_AHBPERIPHRAL_GPIO), ENABLE);
    CLK_APBPeriphralCmd((CLK_APBPERIPHRAL_PMU | CLK_APBPERIPHRAL_RTC | CLK_APBPERIPHRAL_ANA), ENABLE);

    // ÂàùÂßãÂåñ‰∏äÁîµÊ£ÄÊµã?
    GPIOE->OEN |= (0x00000001 << 5);// powercheck
    GPIOE->IE |= (0x00000001 << 5);

    /**
    MCUÁ¨¨‰∏ÄÊ¨°‰∏äÁîµÊó∂ÔºåÈúÄË¶ÅÁ≠âÂæÖÁ∫¶???ÁöÑËµ∑ÈúáÊó∂Èó¥ÔºåÊâçÂèØ‰ª•‰ΩøÁî®PLL//ÂæÖË∞ÉÊï¥
    */
    for (val = 0; val < 20000; val++)
    {
        v85xxp_clear_watchdog();
        __NOP();
    }
#ifndef _BOOT_
    sysclk_at_3m2768(flag);
#endif
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

        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;

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
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;
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
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;
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
        
        tmp = UARTx->DATA;  //≥ı ºªØ∫Û»°“ª¥Œ ˝æ›.
    }
}

int mcu_uart_send_byte(uint8 send_data, size_t uart_index)
{
    UART_TypeDef* UARTx;

    if (uart_index < 6)
    {
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;

        UARTx->DATA = send_data;
        
        return true;
    }
    
    return false;
}

int mcu_uart_recive_byte(uint8* byte , size_t uart_index)
{
    UART_TypeDef* UARTx;


    if (uart_index < 6)
    {
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;
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
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;
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
        //UARTx = UART_list[uart_index];
        UARTx = UART0 + uart_index*0x20;
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

    //UARTx = UART_list[uart_index];
    UARTx = UART0 + uart_index*0x20;

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
    
    //UARTx = UART_list[uart_index];
    UARTx = UART0 + uart_index*0x20;

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

    mcu_delay_us(10);
    // CSËΩØ‰ª∂ÊéßÂà∂ /  ÂâçËæπÊ≤øÂª∫Á´ãÊï∞ÊçÆ / sckÁ©∫Èó≤Áä∂ÊÄÅ‰∏∫È´ò / MSBÂÖà‰º†Ëæì   / ‰∏ªÊú∫/ apbclk 4ÂàÜÈ¢ë
    SPI2->CTRL = BIT10 | BIT5 | BIT4 | BIT0;

    SPI2->MISC_ |= BIT8;

    SPI2->CTRL |= (0x00000001 << 15);

    mcu_delay_us(10);

    return true;
}


uint8 mcu_spi_send_byte(uint8 byte, uint8 spi_index)
{
    uint16 timeout = 50000; // ÂæÖË∞ÉÊï¥
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




uint8 poweroff_test = 1;//ÊîπÂõûÊù•
uint8 ac_power_off_monitor(void)
{
#if 0 
    if (poweroff_test) // ÊîπÂõûÊù•
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

    if (ANA_GetStatus(ANA_STATUS_VDCINDROP))     //ÊéâÁîµ
    {
        return true;
    }
    return false;




}

uint8 ac_power_on_monitor(void)
{
#if 0 
    if (poweroff_test) // ÊîπÂõûÊù•
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

void fnINTTC3_Init(uint8 cmd)//2ms
{
        TMR_InitType    TMR_InitStruct;

        TMR_DeInit(TMR3);
        TMR_InitStruct.ClockSource = TMR_CLKSRC_INTERNAL;
        TMR_InitStruct.EXTGT = TMR_EXTGT_DISABLE;
        TMR_InitStruct.Period = 52429;
        TMR_Init(TMR3, &TMR_InitStruct);
        
        TMR_INTConfig(TMR3, ENABLE);  //Enable Timer0 interrupt
        CORTEX_SetPriority_ClearPending_EnableIRQ(TMR3_IRQn, 2);

        TMR_Cmd(TMR3, ENABLE);  //Enable Timer0
}

void start_emu_signal_pro(void)
{
    PMU_ClearIOAINTStatus(GPIO_Pin_8);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_RISING);
    GPIOA->OEN |=BIT8;
    GPIOA->IE |=BIT8;
    fnINTTC3_Init(1);
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
    /*if (system_state.power_off_sencond > 5)
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
    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);   //‰ΩøËÉΩÂ§ñÈÉ®IOÂè£Âî§ÈÜí
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);
    CORTEX_SetPriority_ClearPending_EnableIRQ(PMU_IRQn, 0); //‰ºòÂÖàÁ∫ßÈ´ò

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
    MComp1Init();//ÁîµÊ±†Áä∂ÊÄÅÁõëÊµãÂàùÂßãÂåñ
    WDT_Disable();
    PMU_EnterSleep_LowPower(&LowPower_InitStruct);
}


int v85xxp_spi_bus_init(size_t index)
{

    MISC2->PCLKEN |= MISC2_PCLKEN_SPI2;
    
    __NOP();
    __NOP();
    
    SPI2->CTRL = 0x00;
    
    mcu_delay_us(10);
    // CS»Ìº˛øÿ÷∆ /  «∞±ﬂ—ÿΩ®¡¢ ˝æ› / sckø’œ–◊¥Ã¨Œ™∏ﬂ / MSBœ»¥´ ‰   / ÷˜ª˙/ apbclk 4∑÷∆µ
    SPI2->CTRL = BIT10 | BIT5 | BIT4 | BIT0;

    SPI2->MISC_ |= BIT8;
    
    SPI2->CTRL |= (0x00000001 << 15);
    
    mcu_delay_us(10);
    
    return true;
}


int v85xxp_spi_send_byte(uint8 byte , size_t index)
{
   uint16 timeout = 50000; // ¥˝µ˜’˚
   uint8  rxd;
    

   while((0 == (SPI2->MISC_ & SPI_MISC_TFE))&&(timeout>0))
   {
        timeout--;
   }
   
   SPI2->TXDAT = byte;
   timeout = 60000;
   
   while((SPI2->MISC_ & SPI_MISC_BSY_Msk)&&(timeout>0))
   {
        timeout--;
   }
   rxd = SPI2->RXDAT;
   /////
   if(timeout > 0 )
   {
        return true;
   }
   //////
   rxd -= rxd;
   
   return false;
}

int v85xxp_spi_recive_byte(uint8 *byte , size_t index)
{
   uint16 timeout = 50000;
   uint8  rxd = 0;
   
   while((0 == (SPI2->MISC_ & SPI_MISC_TFE))&&(timeout>0))
   {
        timeout--;
   }

   SPI2->TXDAT = 0x00;
   timeout = 50000;

   while((SPI2->MISC_ & SPI_MISC_BSY_Msk)&&(timeout>0))
   {
        timeout--;
   }
   
   if(timeout > 0 )
   {
        rxd = SPI2->RXDAT;
        *byte = rxd;
        return true;
   }
   return false;
}

void v85xxp_spi_ctrl(uint8 ctrl , size_t index)
{
    switch(ctrl)
    {
        case START:
             SPI2->CTRL |= SPI_CTRL_SPIEN;
             GPIOC->OEN &= ~(BIT0 + BIT1 + BIT3);
             GPIOC->IE &= ~(BIT0 + BIT1 + BIT3);
             GPIOC->OEN |= BIT2;
             GPIOC->IE |= BIT2;
              break;
        case STOP:
            SPI2->CTRL &= ~SPI_CTRL_SPIEN;
            GPIOC->OEN &= ~(BIT0 + BIT1 + BIT2 + BIT3);
            GPIOC->IE &= ~(BIT0 + BIT1 + BIT2 + BIT3 );
            GPIOC->DAT &= ~(BIT0 + BIT1 + BIT2 + BIT3);
              break;
        default:
          break;
    }
}


/* lcd ioø⁄ ‰≥ˆƒ£ Ω≈‰÷√ */
uint8 lcd_port_config(uint8 mode)
{
#if 1
    //MISC2->HCLKEN &= (~(uint32)BIT6);
    if(mode == AC_POWER_ON)
    {
            GPIOD->OEN   |= (BIT0| BIT1| BIT2| BIT3| BIT4| BIT5| BIT6| BIT7| BIT12| BIT13| BIT14| BIT15);// πƒ‹ ‰≥ˆ

            GPIOE->OEN   |= (BIT0| BIT1| BIT6| BIT7| BIT13| BIT14| BIT15);

            GPIOA->OEN   |= (BIT0| BIT1| BIT4| BIT5| BIT6| BIT9| BIT10| BIT11);

            GPIOB->OEN   |= (BIT13| BIT14| BIT15);
    }
    else if(mode == AC_POWER_OFF)// ‰≥ˆµÕµÁ∆Ω
    {
            GPIOD->OEN   &= ~(BIT0| BIT1| BIT2| BIT3| BIT4| BIT5| BIT6| BIT7| BIT12| BIT13| BIT14| BIT15);// πƒ‹ ‰≥ˆ

            GPIOE->OEN   &= ~(BIT0| BIT1| BIT6| BIT7| BIT13| BIT14| BIT15);

            GPIOA->OEN   &= ~(BIT0| BIT1| BIT4| BIT5| BIT6| BIT9| BIT10| BIT11);

            GPIOB->OEN   &= ~(BIT13| BIT14| BIT15);
    }
    
    GPIOD->IE    &= ~(BIT0| BIT1| BIT2| BIT3| BIT4| BIT5| BIT6| BIT7| BIT12| BIT13| BIT14| BIT15);//Ω˚÷π ‰»Î
    GPIOD->DAT   &= ~(BIT0| BIT1| BIT2| BIT3| BIT4| BIT5| BIT6| BIT7| BIT12| BIT13| BIT14| BIT15);

    GPIOE->IE    &= ~(BIT0| BIT1| BIT6| BIT7| BIT13| BIT14| BIT15);
    GPIOE->DAT   &= ~(BIT0| BIT1| BIT6| BIT6| BIT13| BIT14| BIT15);

    GPIOA->IE    &= ~(BIT0| BIT1| BIT4| BIT5| BIT6| BIT9| BIT10| BIT11);
    GPIOA->DAT   &= ~(BIT0| BIT1| BIT4| BIT5| BIT6| BIT9| BIT10| BIT11);

    GPIOB->IE    &= ~(BIT13| BIT14| BIT15);
    GPIOB->DAT   &= ~(BIT13| BIT14| BIT15);
#endif
    return 0;
}


uint8 lcd_init(uint8 power_mode)
{   
    LCD_InitType lcd_init_para;
    LCD_IOInitType lcd_io_para;
    switch(power_mode)
    {
        case AC_POWER_ON:
          //memset((uint8 *)&lcd_init_para, 0x00, sizeof(lcd_init_para));
          //memset((uint8 *)&lcd_io_para,   0x00, sizeof(lcd_init_para));
          MISC2->HCLKEN |= BIT6;//¥Úø™lcd ±÷”
          lcd_port_config(AC_POWER_ON);

          lcd_init_para.Type   = LCD_TYPE_6COM;
          lcd_init_para.Drv    = LCD_DRV_600;
          lcd_init_para.FRQ    = LCD_FRQ_256H;
          lcd_init_para.SWPR   = 0x00;
          lcd_init_para.FBMODE = LCD_FBMODE_BUFA;
          lcd_init_para.BKFILL = LCD_BKFILL_0;

          lcd_io_para.COMMode  = LCD_TYPE_6COM;
          lcd_io_para.SegCtrl0 = (BIT2| BIT3| BIT8| BIT9| BIT10| BIT11 | BIT0 | BIT1);
          lcd_io_para.SegCtrl1 = (BIT29| BIT28| BIT25| BIT24| BIT23| BIT22| BIT21| BIT17| BIT16| BIT15| BIT10| BIT9| BIT8);
          lcd_io_para.SegCtrl2 = (BIT2| BIT3| BIT4| BIT10| BIT11);
          
          LCD_Init(&lcd_init_para);
          LCD_BiasModeConfig(LCD_BMODE_DIV4);
          LCD_Cmd(&lcd_io_para, ENABLE);
            break;
        default:
          LCD->SEGCTRL0 = 0;
          LCD->SEGCTRL1 = 0;
          LCD->SEGCTRL2 = 0;
          lcd_port_config(AC_POWER_OFF);
          lcd_io_para.COMMode  = LCD_TYPE_6COM;
          lcd_io_para.SegCtrl0 = 0x00;
          lcd_io_para.SegCtrl1 = 0x00;
          lcd_io_para.SegCtrl2 = 0x00;
          LCD->CTRL &=~LCD_CTRL_EN;
          MISC2->HCLKEN &= (~(uint32)BIT6);
          LCD_Cmd(&lcd_io_para, DISABLE);
            break;
    }
    return true;
}


#define MAX_LCD_BUF  24
#define MCU_BUF_MAX  20
const uint8 CODE mcu_seg_map [] =
{
   2, 3, 8, 9, 10, 11, 75, 74, 68, 67, 66, 61, 60, 57, 56, 55, 54, 53, 49, 48, 47, 42, 41, 40,
};
uint8 lcd_mapping(uint32 addr, uint8 *buf, uint16 len)
{ 
    uint8 i = 0x00, index = 0x00, offset = 0x00;
    uint32 lcd_tmp[MCU_BUF_MAX] = {0x00};
	
    if((addr+len) > MAX_LCD_BUF)
    {
        return false;
    }

    for(i = 0; i < MAX_LCD_BUF; i++)
    {
        index  = mcu_seg_map[i] / 4;
        offset = mcu_seg_map[i] % 4;

        lcd_tmp[index] |= ((uint32)buf[i] << offset * 8);
    }

    memcpy((uint8*)(&LCD->FB[addr]), (uint8 *)&lcd_tmp[0], sizeof(lcd_tmp));
	
    return true;
}

void mcu_exit_sleep_then_enter_normal(void)
{

}


void mcu_main_power_on_config(void)
{
    cm0_systick_init(1);
    
    init_v85xxp_rtc(1);
    
    lcd_init(AC_POWER_ON);
    
    start_emu_signal_pro();
    
    PMU->CONTROL |= PMU_CONTROL_INT_IOA_EN;
    
    NVIC_EnableIRQ(PMU_IRQn);
}

void mcu_main_power_off_config(void)
{
    cm0_systick_init(0);
    
    
}



int emu_uart_init(size_t index)
{
    UART_InitType* InitType = (UART_InitType*)index;
    
    mcu_uart_init(InitType, 3, 0);
    
    return true;
}



uint8 mcu_adc_start_once(uint8 chl_type)
{ 

    ADCInitType ADC_InitStruct;

    /* ADC initialization */
    ADC_DeInit();
    ADC_InitStruct.TrigMode = ADC_TRIGMODE_MANUAL;
    ADC_InitStruct.ConvMode = ADC_CONVMODE_SINGLECHANNEL;
    ADC_InitStruct.Channel = chl_type;
    ADC_InitStruct.ClockDivider = ADC_CLKDIV_4;
    ADC_InitStruct.ClockSource = ADC_CLKSRC_RCH;
    ADC_Init(&ADC_InitStruct);

    if (chl_type == 2)
    {
        /* Enable division */
        ANA->REG1 |= ADC_BAT_CAPDIV;
    }
    else if (chl_type == 10)
    {
        /* Configure 1/512 down-sampling rate */
        ADC_CICDownSamRateConfig(ADC_SDRSEL_DIV512);
    }
    
    /* Enable ADC */
    ADC_Cmd(ENABLE);

    /* Start a manual ADC conversion */
    ADC_StartManual();
    /* Waiting last operation done */
    ADC_WaitForManual();
    
    return true;

}

uint32 read_mcu_adc_measure_value(uint8 chl_type , int32 *adc_measure)
{
    float value = 0, Vbatcap;
    int32 ad = 0;
    int32 P0,P1,P2;
    uint32 retval;
    NVR_RTCINFO RTC_DataStruct;
    NVR_BATMEARES BAT_OffsetInfo;
    
    ad =  ADC_GetADCConversionValue(chl_type);

    if (chl_type == 2)
    {

        /* Get NVR BAT offset information */
        if (NVR_GetBATOffset(&BAT_OffsetInfo))
        {
            return false;
        }
        else  
        {
            Vbatcap = BAT_OffsetInfo.BATCAPResult; 
        }

        if (ADC_BAT_CAPDIV & ADC_BAT_CAPDIV)
        {
            value = ((float)ad  - 49.588)/7088.91575 + Vbatcap;
            value *= 10000;
        }
        *adc_measure = (int32)value;

    }
    else if(chl_type == 10)
    {

        /* Get RTC Px parameters */
        retval = NVR_GetInfo_LoadRTCData(&RTC_DataStruct);
        if (retval & 0x1U)
        {
            return false;
        }
        else
        {
            P0 = RTC_DataStruct.RTCTempP0;
            P1 = RTC_DataStruct.RTCTempP1;
            P2 = RTC_DataStruct.RTCTempP2;
        }  

        value = (float)((((P0 * ((ad*ad)>>16)) + P1*ad + P2) >> 8)*10 / 256.0);
        value *= 100;
        
        *adc_measure = (int32)value;
    }

//    *adc_measure = (uint32)value;
    
    /* ADC resource release */
    ADC_DeInit();
    
    return true;  
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

const struct bus_ops_t mcu_spi_ops = 
{
   BUS_SPI,
   1,
   v85xxp_spi_bus_init,
   __NULL,
   v85xxp_spi_send_byte,
   __NULL,
   v85xxp_spi_recive_byte,
   __NULL,
   __NULL,
   v85xxp_spi_ctrl,
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