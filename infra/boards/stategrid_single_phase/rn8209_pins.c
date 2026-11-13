#include "infra/boards/stategrid_single_phase/rn8209_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_rn8209_reset_line_output(void)
{
    nd_pin_reg_clear(GPIOA_OEN, BIT(15));
    nd_pin_reg_clear(GPIOA_IE, BIT(15));
}

void nd_rn8209_reset_line_input(void)
{
    nd_pin_reg_set(GPIOA_IE, BIT(15));
    nd_pin_reg_set(GPIOA_OEN, BIT(15));
}

void nd_rn8209_reset_line_set(int high)
{
    nd_pin_write(GPIOA_DAT, 15, high);
}

void nd_rn8209_tx_line_output(void)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(5));
    nd_pin_reg_clear(GPIOB_IE, BIT(5));
}

void nd_rn8209_tx_line_set(int high)
{
    if (high)
    {
        nd_pin_write(GPIOB_DAT, 5, 1);
    }
}
