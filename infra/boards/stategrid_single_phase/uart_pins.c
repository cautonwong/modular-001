#include "infra/boards/stategrid_single_phase/uart_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_uart_rs485_receiver_enable(void)
{
    nd_pin_write(GPIOA_DAT, 7, 1);
    nd_pin_reg_clear(GPIOA_OEN, BIT(7));
}

void nd_uart_rs485_receiver_disable(void)
{
    nd_pin_write(GPIOA_DAT, 7, 0);
    nd_pin_reg_clear(GPIOA_OEN, BIT(7));
}

void nd_uart_rs485_rx_input(void)
{
    nd_pin_reg_set(GPIOA_IE, BIT(12));
    nd_pin_reg_set(GPIOA_OEN, BIT(12));
}

void nd_uart_rs485_tx_output(void)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(2));
    nd_pin_reg_clear(GPIOB_IE, BIT(2));
}

void nd_uart_rs485_tx_set(int high)
{
    if (high)
    {
        nd_pin_write(GPIOB_DAT, 2, 1);
    }
}
