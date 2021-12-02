/**
 * @ingroup    RN8318
 * @brief      RN8318 isr¶¨Òå
 *
 * @{
 * @file
 *
 */

#include "target.h"
#include "target_isr.h"
#include "type_def.h"
#include "lib_conf.h"
#include "cpu_surport.h"



#define IS_ACPOWER_OFF  1






static isr_handle isr[IRQ_NUM] = {0};


void ndv_isr_handle_regsit(isr_handle isr_handle_f , uint8_t irq)
{
   if((isr_handle_f) && (irq<IRQ_NUM))
   {
      isr[irq] = isr_handle_f;
   }
}



/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}



/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick_Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
    __asm("MOV R0, LR");
    __asm("MOVS R1, #0x04");
    /* »ñÈ¡SP´úÂëÎ´Íê³É,µ÷ÊÔÊ±ÇëÊÖ¶¯²é¿´MSP»òPSPµÄÄÚÈÝ.*/
    
    NVIC_SystemReset();  /* Ö±½Ó¸´Î»CM3¼°ÍâÎ§µçÂ· */
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick_Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
    __asm("B HardFault_Handler");
}
/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick_Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern void test_pin_turn(void);
void SysTick_Handler(void)
{
    if(isr[IRQ_SYSTICK])
    {
        isr[IRQ_SYSTICK]();
    }
}

//¶¨Ê±Æ÷
void TMR0_IRQHandler(void)
{
    #ifndef _BOOT_
    TMR0->INT |= TMR_INT_INT;
#endif
}


void TMR1_IRQHandler(void)
{
#ifndef _BOOT_
   if( TMR_GetINTStatus(TMR1))
    {
        TMR_ClearINTStatus(TMR1);
        
    }
#endif
}

/**
  * @brief  This function handles TMR2 interrupt request.
  * @param  None
  * @retval None
  */
void TMR2_IRQHandler(void)
{
}

/**
  * @brief  This function handles TMR3 interrupt request.
  * @param  None
  * @retval None
  */
void TMR3_IRQHandler(void) 
{
#ifndef _BOOT_
    if( TMR_GetINTStatus(TMR3))
    {
        TMR_ClearINTStatus(TMR3);
        if(isr[IRQ_TMR3])
        {
            isr[IRQ_TMR3]();
        }
    }
#endif
}

/**
  * @brief  This function handles PWM0 interrupt request.
  * @param  None
  * @retval None
  */
void PWM0_IRQHandler(void)
{
}

/**
  * @brief  This function handles PWM1 interrupt request.
  * @param  None
  * @retval None
  */
void PWM1_IRQHandler(void)
{
}

/**
  * @brief  This function handles PWM2 interrupt request.
  * @param  None
  * @retval None
  */
void PWM2_IRQHandler(void)
{
}

/**
  * @brief  This function handles PWM3 interrupt request.
  * @param  None
  * @retval None
  */
void PWM3_IRQHandler(void)
{
}

/**
  * @brief  This function handles DMA interrupt request.
  * @param  None
  * @retval None
  */
void DMA_IRQHandler(void)
{
}

/**
  * @brief  This function handles FLASH interrupt request.
  * @param  None
  * @retval None
  */
void FLASH_IRQHandler(void)
{
}

/**
  * @brief  This function handles ANA interrupt request.
  * @param  None
  * @retval None
  */
void ANA_IRQHandler(void)
{
}

/**
  * @brief  This function handles SPI2 interrupt request.
  * @param  None
  * @retval None
  */
void SPI2_IRQHandler(void)
{
}


/**
  * @brief  This function handles SPI3 interrupt request.
  * @param  None
  * @retval None
  */
void SPI3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick_Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void RTC_IRQHandler(void)
{
#ifndef _BOOT_
    if(RTC->INTSTS & BIT(0))
    {
        if(isr[IRQ_RTC])
        {
            isr[IRQ_RTC]();
        }
    }
    RTC->INTSTS = 0xFF; 
#endif
}


/**
  * @brief  This function handles PMU interrupt request.
  * @param  None
  * @retval None
  */
void PMU_IRQHandler(void)
{
#ifndef _BOOT_
    if(isr[IRQ_PMU])
    {
        isr[IRQ_PMU]();
    }
#endif
}


/*******************************************************************************
* Function Name  : UART0_HANDLER
* Description    : This function handles UART0 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART0_IRQHandler(void)
{
    if(UART0->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART0_TX])
        {
            isr[IRQ_UART0_TX]();
        }
        UART0->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART0->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART0_RX])
        {
            isr[IRQ_UART0_RX]();
        }        
        UART0->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART0->INTSTS & 0x00000062)
    {
        UART0->INTSTS = 0x00000062;
    }
}
/*******************************************************************************
* Function Name  : UART1_HANDLER
* Description    : This function handles UART1 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART1_IRQHandler(void)
{
#ifndef _BOOT_
    if(UART1->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART1_TX])
        {
            isr[IRQ_UART1_TX]();
        }
        UART1->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART1->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART1_RX])
        {
            isr[IRQ_UART1_RX]();
        }        
        UART1->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART1->INTSTS & 0x00000062)
    {
        UART1->INTSTS = 0x00000062;
    }
#endif
}

/*******************************************************************************
* Function Name  : UART2_HANDLER
* Description    : This function handles UART2 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART2_IRQHandler(void)
{
#ifndef _BOOT_
    if(UART2->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART2_TX])
        {
            isr[IRQ_UART2_TX]();
        }
        UART2->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART2->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART2_RX])
        {
            isr[IRQ_UART2_RX]();
        }
        UART2->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART2->INTSTS & 0x00000062)
    {
        UART2->INTSTS = 0x00000062;
    }
#endif
}

/*******************************************************************************
* Function Name  : UART3_HANDLER
* Description    : This function handles UART3 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART3_IRQHandler(void)
{
#ifndef _BOOT_
    if(UART3->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART3_TX])
        {
            isr[IRQ_UART3_TX]();
        }
        UART3->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART3->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART3_RX])
        {
            isr[IRQ_UART3_RX]();
        }
        UART3->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART3->INTSTS & 0x00000062)
    {
        UART3->INTSTS = 0x00000062;
    }
#endif
}
//uint32 test_ir_RX_CNT = 0;//¸Ä»ØÀ´
/*******************************************************************************
* Function Name  : UART4_HANDLER
* Description    : This function handles UART4 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART4_IRQHandler(void)
{
    if(UART4->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART4_TX])
        {
            isr[IRQ_UART4_TX]();
        }
        UART4->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART4->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART4_RX])
        {
            isr[IRQ_UART4_RX]();
        }       
        UART4->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART4->INTSTS & 0x00000062)
    {
        UART4->INTSTS = 0x00000062;
    }
}

/*******************************************************************************
* Function Name  : UART5_HANDLER
* Description    : This function handles UART5 HANDLER.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART5_IRQHandler(void)
{
    if(UART5->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if(isr[IRQ_UART5_TX])
        {
            isr[IRQ_UART5_TX]();
        }
        UART5->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART5->INTSTS & UART_INTSTS_RXIF)
    {
        if(isr[IRQ_UART5_RX])
        {
            isr[IRQ_UART5_RX]();
        }
        UART5->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART5->INTSTS & 0x00000062)
    {
        UART5->INTSTS = 0x00000062;
    } 
}



/*******************************************************************************
* Function Name  : CMP_HANDLER
* Description    : This function handles CMP_HANDLER
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ISO78160_IRQHandler(void)
{
    



}

/*******************************************************************************
* Function Name  : CMP_HANDLER
* Description    : This function handles CMP_HANDLER
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ISO78161_IRQHandler(void)
{

}

/** @} */

