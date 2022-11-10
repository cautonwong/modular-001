/**
  * @file    target_isr.c
  * @author  Application Team
  * @version V4.4.0
  * @date    2018-09-27
  * @brief   Main Interrupt Service Routines.
******************************************************************************/
#include "target_isr.h"
#include "Includes.H"

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + NMI_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + HardFault_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + SVC_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PendSV_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + SysTick_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles PMU interrupt request.
  * @param  None
  * @retval None
  */
void PMU_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PMU_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles RTC interrupt request.
  * @param  None
  * @retval None
  */
unsigned char Ir_Cou;
void RTC_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + RTC_HAND_OFFSET));
    (*p)();

}

/**
  * @brief  This function handles U32K0 interrupt request.
  * @param  None
  * @retval None
  */
void U32K0_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + U32K0_HAND_OFFSET));
    (*p)();

}

/**
  * @brief  This function handles U32K1 interrupt request.
  * @param  None
  * @retval None
  */
void U32K1_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + U32K1_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles I2C interrupt request.
  * @param  None
  * @retval None
  */
void I2C_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + I2C_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles SPI1 interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + SPI1_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART0 interrupt request.
  * @param  None
  * @retval None
  */
void UART0_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART0_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART1 interrupt request.
  * @param  None
  * @retval None
  */
void UART1_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART1_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART2 interrupt request.
  * @param  None
  * @retval None
  */
void UART2_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART2_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART3 interrupt request.
  * @param  None
  * @retval None
  */
void UART3_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART3_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART4 interrupt request.
  * @param  None
  * @retval None
  */
void UART4_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART4_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles UART5 interrupt request.
  * @param  None
  * @retval None
  */
void UART5_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + UART5_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles ISO78160 interrupt request.
  * @param  None
  * @retval None
  */
void ISO78160_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + ESAM0_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles ISO78161 interrupt request.
  * @param  None
  * @retval None
  */
void ISO78161_IRQHandler(void)
{
    pFunc p;
    
        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + ESAM1_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles TMR0 interrupt request.   interval  0.1ms
  * @param  None
  * @retval None
  */
void TMR0_IRQHandler(void)
{
    pFunc p;

    
        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + TMR0_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles TMR1 interrupt request.		interval  10ms
  * @param  None
  * @retval None
  */
void TMR1_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + TMR1_HAND_OFFSET));
    (*p)();

}

/**
  * @brief  This function handles TMR2 interrupt request.
  * @param  None
  * @retval None
  */
void TMR2_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + TMR2_HAND_OFFSET));
    (*p)();

}

/**
  * @brief  This function handles TMR3 interrupt request.
  * @param  None
  * @retval None
  */
void TMR3_IRQHandler(void)
{
    pFunc p;


    
        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + TMR3_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles PWM0 interrupt request.
  * @param  None
  * @retval None
  */
void PWM0_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PWM0_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles PWM1 interrupt request.
  * @param  None
  * @retval None
  */
void PWM1_IRQHandler(void)
{
    pFunc p;

     p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PWM1_HAND_OFFSET));
     (*p)();
}

/**
  * @brief  This function handles PWM2 interrupt request.
  * @param  None
  * @retval None
  */
void PWM2_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PWM2_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles PWM3 interrupt request.
  * @param  None
  * @retval None
  */
void PWM3_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + PWM3_HAND_OFFSET));
    (*p)();
}

/**
  * @brief  This function handles DMA interrupt request.
  * @param  None
  * @retval None
  */
void DMA_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + DMA_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles FLASH interrupt request.
  * @param  None
  * @retval None
  */
void FLASH_IRQHandler(void)
{
    pFunc p;



        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + FLASH_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles ANA interrupt request.
  * @param  None
  * @retval None
  */
void ANA_IRQHandler(void)
{
    pFunc p;


        p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + ANA_HAND_OFFSET));
        (*p)();

}

/**
  * @brief  This function handles SPI2 interrupt request.
  * @param  None
  * @retval None
  */
void SPI2_IRQHandler(void)
{
    pFunc p;

    p = (pFunc)(*(__IO uint32_t*)(APP_START_ADDR + SPI2_HAND_OFFSET));
    (*p)();
}

/*********************************** END OF FILE ******************************/
