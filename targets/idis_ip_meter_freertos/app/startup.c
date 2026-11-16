#include <stdint.h>

#include "linker_symbols.h"
#include "target_isr.h"

int main(void);

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
