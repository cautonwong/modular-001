#include "target.h"
#include "target_isr.h"
#include "type_def.h"
#include "cpu_surport.h"

/*
 * The V8500 vector table lives in the product startup file.  Peripheral
 * drivers register their callbacks here by logical IRQ number; the handlers
 * below dispatch to the registered callback and clear the source flag.
 */
static struct isr_reg_ctx_t isr_table[IRQ_NUM];

uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *isr_reg, uint8_t irq)
{
    if (isr_reg == NULL || irq >= IRQ_NUM
        || isr_reg->IsrFunctionRegister == NULL) {
        return 0;
    }
    isr_table[irq] = *isr_reg;
    isr_table[irq].irq_index = irq;
    return 1;
}

static void isr_dispatch(uint8_t irq)
{
    if (irq < IRQ_NUM && isr_table[irq].IsrFunctionRegister != NULL
        && isr_table[irq].irq_index == irq) {
        isr_table[irq].IsrFunctionRegister(isr_table[irq].para);
    }
}

static void uart_irq(UART_TypeDef *uart, uint8_t tx_irq)
{
    if ((uart->INTSTS & UART_INTSTS_TXDONEIF) != 0u) {
        isr_dispatch(tx_irq);
        uart->INTSTS = UART_INTSTS_TXDONEIF;
    }
    if ((uart->INTSTS & UART_INTSTS_RXIF) != 0u) {
        isr_dispatch((uint8_t)(tx_irq + 1u));
        uart->INTSTS = UART_INTSTS_RXIF;
    }
    if ((uart->INTSTS & 0x00000062u) != 0u) {
        uart->INTSTS = 0x00000062u;
    }
}

void PMU_IRQHandler(void)
{
    isr_dispatch(IRQ_PMU);
}

void RTC_IRQHandler(void)
{
    if ((RTC->INTSTS & BIT(0)) != 0u) {
        isr_dispatch(IRQ_RTC);
    }
    RTC->INTSTS = 0xFF;
}

void UART0_IRQHandler(void) { uart_irq(UART0, IRQ_UART0_TX); }
void UART1_IRQHandler(void) { uart_irq(UART1, IRQ_UART1_TX); }
void UART2_IRQHandler(void) { uart_irq(UART2, IRQ_UART2_TX); }
void UART3_IRQHandler(void) { uart_irq(UART3, IRQ_UART3_TX); }
void UART4_IRQHandler(void) { uart_irq(UART4, IRQ_UART4_TX); }
void UART5_IRQHandler(void) { uart_irq(UART5, IRQ_UART5_TX); }

void TMR0_IRQHandler(void)
{
    TMR0->INT |= TMR_INT_INT;
    isr_dispatch(IRQ_TMR0);
}

void TMR1_IRQHandler(void)
{
    if (TMR_GetINTStatus(TMR1)) {
        TMR_ClearINTStatus(TMR1);
        isr_dispatch(IRQ_TMR1);
    }
}

void TMR2_IRQHandler(void)
{
    if (TMR_GetINTStatus(TMR2)) {
        TMR_ClearINTStatus(TMR2);
        isr_dispatch(IRQ_TMR2);
    }
}

void TMR3_IRQHandler(void)
{
    if (TMR_GetINTStatus(TMR3)) {
        TMR_ClearINTStatus(TMR3);
        isr_dispatch(IRQ_TMR3);
    }
}

void DMA_IRQHandler(void) { isr_dispatch(IRQ_DMA); }
void U32K0_IRQHandler(void) {}
void U32K1_IRQHandler(void) {}
void I2C_IRQHandler(void) {}
void SPI1_IRQHandler(void) {}
void ISO78160_IRQHandler(void) {}
void ISO78161_IRQHandler(void) {}
void PWM0_IRQHandler(void) {}
void PWM1_IRQHandler(void) {}
void PWM2_IRQHandler(void) {}
void PWM3_IRQHandler(void) {}
void FLASH_IRQHandler(void) {}
void ANA_IRQHandler(void) {}
void SPI2_IRQHandler(void) {}
