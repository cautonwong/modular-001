/**
  * @file    target_isr.h
  * @author  Application Team
  * @version V4.4.0
  * @date    2018-09-27
  * @brief   This file contains the headers of the interrupt handlers.
******************************************************************************/

#ifndef __TARGET_ISR_H
#define __TARGET_ISR_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "target.h"

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define RESET_HAND_OFFSET 0x4
#define NMI_HAND_OFFSET 0x8
#define HardFault_HAND_OFFSET   0xC
#define SVC_HAND_OFFSET         0x2C
#define PendSV_HAND_OFFSET      0x38
#define SysTick_HAND_OFFSET     0x3C
#define PMU_HAND_OFFSET         0x40
#define RTC_HAND_OFFSET         0x44
#define U32K0_HAND_OFFSET       0x48
#define U32K1_HAND_OFFSET       0x4C
#define I2C_HAND_OFFSET         0x50
#define SPI1_HAND_OFFSET        0x54
#define UART0_HAND_OFFSET       0x58
#define UART1_HAND_OFFSET       0x5C
#define UART2_HAND_OFFSET       0x60
#define UART3_HAND_OFFSET       0x64
#define UART4_HAND_OFFSET       0x68
#define UART5_HAND_OFFSET       0x6C
#define ESAM0_HAND_OFFSET       0x70
#define ESAM1_HAND_OFFSET       0x74
#define TMR0_HAND_OFFSET        0x78
#define TMR1_HAND_OFFSET        0x7C
#define TMR2_HAND_OFFSET        0x80
#define TMR3_HAND_OFFSET        0x84
#define PWM0_HAND_OFFSET        0x88
#define PWM1_HAND_OFFSET        0x8C
#define PWM2_HAND_OFFSET        0x90
#define PWM3_HAND_OFFSET        0x94
#define DMA_HAND_OFFSET         0x98 
#define FLASH_HAND_OFFSET       0x9C
#define ANA_HAND_OFFSET         0xA0
#define SPI2_HAND_OFFSET        0xA4
/* Exported functions ------------------------------------------------------- */
void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void PMU_IRQHandler(void);
void RTC_IRQHandler(void);
void U32K0_IRQHandler(void);
void U32K1_IRQHandler(void);
void I2C_IRQHandler(void);
void SPI1_IRQHandler(void);
void UART0_IRQHandler(void);
void UART1_IRQHandler(void);
void UART2_IRQHandler(void);
void UART3_IRQHandler(void);
void UART4_IRQHandler(void);
void UART5_IRQHandler(void);
void ISO78160_IRQHandler(void);
void ISO78161_IRQHandler(void);
void TMR0_IRQHandler(void);
void TMR1_IRQHandler(void);
void TMR2_IRQHandler(void);
void TMR3_IRQHandler(void);
void PWM0_IRQHandler(void);
void PWM1_IRQHandler(void);
void PWM2_IRQHandler(void);
void PWM3_IRQHandler(void);
void DMA_IRQHandler(void);
void FLASH_IRQHandler(void);
void ANA_IRQHandler(void);
void SPI2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif

/*********************************** END OF FILE ******************************/
