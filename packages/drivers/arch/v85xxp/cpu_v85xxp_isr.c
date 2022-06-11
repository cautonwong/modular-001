
#include "target.h"
#include "target_isr.h"
#include "type_def.h"
#include "lib_conf.h"
#include "cpu_surport.h"
#include "pin_function_def.h"


#define IS_ACPOWER_OFF  1







static struct isr_reg_ctx_t Isr_Function[IRQ_NUM] = {0};




uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *isrRegctx , uint8_t irq)
{
   if(irq<IRQ_NUM)
   {
      if(isrRegctx->IsrFunctionRegister != 0)
      {
          Isr_Function[irq].IsrFunctionRegister = isrRegctx->IsrFunctionRegister;
          Isr_Function[irq].para = isrRegctx->para;
          Isr_Function[irq].irq_index = irq;
          return 1;
      }
   }
   return 0;
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
extern uint16_t relay_off_chk,relay_on_chk; //继电器状态监测
void SysTick_Handler(void) //10ms
{    
    if((Isr_Function[IRQ_SYSTICK].IsrFunctionRegister)&&(Isr_Function[IRQ_SYSTICK].irq_index==IRQ_SYSTICK))
    {
        Isr_Function[IRQ_SYSTICK].IsrFunctionRegister(Isr_Function[IRQ_SYSTICK].para);
    }
    
    if(RELAY_CHECK())
    {
         if(relay_off_chk < Relay_Ck_Time)  relay_off_chk++;
         relay_on_chk = 0;
    }
    else
    {
         if(relay_on_chk < Relay_Ck_Time)   relay_on_chk++;
         relay_off_chk = 0;
    }
}

//¶¨Ê±Æ÷
void TMR0_IRQHandler(void)
{
    
    TMR0->INT |= TMR_INT_INT;
}


void TMR1_IRQHandler(void)
{
   if( TMR_GetINTStatus(TMR1))
    {
        TMR_ClearINTStatus(TMR1);
        
    }
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
    if( TMR_GetINTStatus(TMR3))
    {
        TMR_ClearINTStatus(TMR3);
        if((Isr_Function[IRQ_TMR3].IsrFunctionRegister)&&(Isr_Function[IRQ_TMR3].irq_index==IRQ_TMR3))
        {
            Isr_Function[IRQ_TMR3].IsrFunctionRegister(Isr_Function[IRQ_TMR3].para);
        }
    }
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
    if(RTC->INTSTS & BIT(0))
    {
        if((Isr_Function[IRQ_RTC].IsrFunctionRegister)&&(Isr_Function[IRQ_RTC].irq_index==IRQ_RTC))
        {
            Isr_Function[IRQ_RTC].IsrFunctionRegister(Isr_Function[IRQ_RTC].para);
        }        
    }
    RTC->INTSTS = 0xFF; 
}

extern void IO_Interrupt(void);
/**
  * @brief  This function handles PMU interrupt request.
  * @param  None
  * @retval None
  */
void PMU_IRQHandler(void)
{
    IO_Interrupt();
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
        if((Isr_Function[IRQ_UART0_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART0_TX].irq_index==IRQ_UART0_TX))
        {
            Isr_Function[IRQ_UART0_TX].IsrFunctionRegister(Isr_Function[IRQ_UART0_TX].para);
        }
        UART0->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART0->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART0_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART0_RX].irq_index==IRQ_UART0_RX))
        {
            Isr_Function[IRQ_UART0_RX].IsrFunctionRegister(Isr_Function[IRQ_UART0_RX].para);
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
    if(UART1->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if((Isr_Function[IRQ_UART1_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART1_TX].irq_index==IRQ_UART1_TX))
        {
            Isr_Function[IRQ_UART1_TX].IsrFunctionRegister(Isr_Function[IRQ_UART1_TX].para);
        }
        UART1->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART1->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART1_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART1_RX].irq_index==IRQ_UART1_RX))
        {
            Isr_Function[IRQ_UART1_RX].IsrFunctionRegister(Isr_Function[IRQ_UART1_RX].para);
        }
        UART1->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART1->INTSTS & 0x00000062)
    {
        UART1->INTSTS = 0x00000062;
    }
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
    if(UART2->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if((Isr_Function[IRQ_UART2_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART2_TX].irq_index==IRQ_UART2_TX))
        {
            Isr_Function[IRQ_UART2_TX].IsrFunctionRegister(Isr_Function[IRQ_UART2_TX].para);
        }
        UART2->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART2->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART2_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART2_RX].irq_index==IRQ_UART2_RX))
        {
            Isr_Function[IRQ_UART2_RX].IsrFunctionRegister(Isr_Function[IRQ_UART2_RX].para);
        }
        UART2->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART2->INTSTS & 0x00000062)
    {
        UART2->INTSTS = 0x00000062;
    }
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
    if(UART3->INTSTS & UART_INTSTS_TXDONEIF)
    {
        if((Isr_Function[IRQ_UART3_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART3_TX].irq_index==IRQ_UART3_TX))
        {
            Isr_Function[IRQ_UART3_TX].IsrFunctionRegister(Isr_Function[IRQ_UART3_TX].para);
        }
        UART3->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART3->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART3_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART3_RX].irq_index==IRQ_UART3_RX))
        {
            Isr_Function[IRQ_UART3_RX].IsrFunctionRegister(Isr_Function[IRQ_UART3_RX].para);
        }
        UART3->INTSTS = UART_INTSTS_RXIF;
    }
    if(UART3->INTSTS & 0x00000062)
    {
        UART3->INTSTS = 0x00000062;
    }
}
//uint32_t test_ir_RX_CNT = 0;//¸Ä»ØÀ´
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
        if((Isr_Function[IRQ_UART4_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART4_TX].irq_index==IRQ_UART4_TX))
        {
            Isr_Function[IRQ_UART4_TX].IsrFunctionRegister(Isr_Function[IRQ_UART4_TX].para);
        }
        UART4->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART4->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART4_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART4_RX].irq_index==IRQ_UART4_RX))
        {
            Isr_Function[IRQ_UART4_RX].IsrFunctionRegister(Isr_Function[IRQ_UART4_RX].para);
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
        if((Isr_Function[IRQ_UART5_TX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART5_TX].irq_index==IRQ_UART5_TX))
        {
            Isr_Function[IRQ_UART5_TX].IsrFunctionRegister(Isr_Function[IRQ_UART5_TX].para);
        }
        UART5->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if(UART5->INTSTS & UART_INTSTS_RXIF)
    {
        if((Isr_Function[IRQ_UART5_RX].IsrFunctionRegister)&&(Isr_Function[IRQ_UART5_RX].irq_index==IRQ_UART5_RX))
        {
            Isr_Function[IRQ_UART5_RX].IsrFunctionRegister(Isr_Function[IRQ_UART5_RX].para);
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

