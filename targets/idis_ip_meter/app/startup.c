#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern int main(void);
extern void SysTick_Handler(void);
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void PMU_IRQHandler(void);
extern void RTC_IRQHandler(void);
extern void U32K0_IRQHandler(void);
extern void U32K1_IRQHandler(void);
extern void I2C_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void UART0_IRQHandler(void);
extern void UART1_IRQHandler(void);
extern void UART2_IRQHandler(void);
extern void UART3_IRQHandler(void);
extern void UART4_IRQHandler(void);
extern void UART5_IRQHandler(void);
extern void ISO78160_IRQHandler(void);
extern void ISO78161_IRQHandler(void);
extern void TMR0_IRQHandler(void);
extern void TMR1_IRQHandler(void);
extern void TMR2_IRQHandler(void);
extern void TMR3_IRQHandler(void);
extern void PWM0_IRQHandler(void);
extern void PWM1_IRQHandler(void);
extern void PWM2_IRQHandler(void);
extern void PWM3_IRQHandler(void);
extern void DMA_IRQHandler(void);
extern void FLASH_IRQHandler(void);
extern void ANA_IRQHandler(void);
extern void SPI2_IRQHandler(void);

void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    for (uint32_t *target = &_sdata; target < &_edata;) {
        *target++ = *source++;
    }
    for (uint32_t *target = &_sbss; target < &_ebss;) {
        *target++ = 0;
    }
    (void)main();
    for (;;) {
    }
}

static void Default_Handler(void)
{
    for (;;) {
    }
}

__attribute__((section(".isr_vector"), used))
void (*const vector_table[])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    Default_Handler, /* NMI */
    Default_Handler, /* HardFault */
    Default_Handler, /* 4 */
    Default_Handler, /* 5 */
    Default_Handler, /* 6 */
    Default_Handler, /* 7 */
    Default_Handler, /* 8 */
    Default_Handler, /* 9 */
    Default_Handler, /* 10 */
    SVC_Handler,     /* 11 */
    Default_Handler, /* 12 */
    Default_Handler, /* 13 */
    PendSV_Handler,  /* 14 */
    SysTick_Handler, /* 15 */
    PMU_IRQHandler,      /* IRQ0 */
    RTC_IRQHandler,      /* IRQ1 */
    U32K0_IRQHandler,    /* IRQ2 */
    U32K1_IRQHandler,    /* IRQ3 */
    I2C_IRQHandler,      /* IRQ4 */
    SPI1_IRQHandler,     /* IRQ5 */
    UART0_IRQHandler,    /* IRQ6 */
    UART1_IRQHandler,    /* IRQ7 */
    UART2_IRQHandler,    /* IRQ8 */
    UART3_IRQHandler,    /* IRQ9 */
    UART4_IRQHandler,    /* IRQ10 */
    UART5_IRQHandler,    /* IRQ11 */
    ISO78160_IRQHandler, /* IRQ12 */
    ISO78161_IRQHandler, /* IRQ13 */
    TMR0_IRQHandler,     /* IRQ14 */
    TMR1_IRQHandler,     /* IRQ15 */
    TMR2_IRQHandler,     /* IRQ16 */
    TMR3_IRQHandler,     /* IRQ17 */
    PWM0_IRQHandler,     /* IRQ18 */
    PWM1_IRQHandler,     /* IRQ19 */
    PWM2_IRQHandler,     /* IRQ20 */
    PWM3_IRQHandler,     /* IRQ21 */
    DMA_IRQHandler,      /* IRQ22 */
    FLASH_IRQHandler,    /* IRQ23 */
    ANA_IRQHandler,      /* IRQ24 */
    Default_Handler,     /* IRQ25 reserved */
    Default_Handler,     /* IRQ26 reserved */
    SPI2_IRQHandler,     /* IRQ27 */
    Default_Handler,     /* IRQ28 reserved */
    Default_Handler,     /* IRQ29 reserved */
    Default_Handler,     /* IRQ30 reserved */
    Default_Handler,     /* IRQ31 reserved */
};
