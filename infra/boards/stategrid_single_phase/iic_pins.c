#include "infra/boards/stategrid_single_phase/iic_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_iic_eeprom_power_set(int on)
{
    if (on)
    {
        nd_pin_reg_clear(GPIOC_OEN, BIT(6));
        nd_pin_reg_clear(GPIOC_IE, BIT(6));
        nd_pin_write(GPIOC_DAT, 6, 1);
    }
    else
    {
        nd_pin_write(GPIOC_DAT, 6, 0);
    }
}

void nd_iic_sda_output(void)
{
    nd_pin_reg_clear(GPIOC_OEN, BIT(5));
    nd_pin_reg_clear(GPIOC_IE, BIT(5));
}

void nd_iic_sda_input(void)
{
    nd_pin_reg_set(GPIOC_IE, BIT(5));
    nd_pin_reg_set(GPIOC_OEN, BIT(5));
}

void nd_iic_sda_set(int high)
{
    nd_pin_write(GPIOC_DAT, 5, high);
}

int nd_iic_sda_read(void)
{
    return nd_pin_read(GPIOC_STS, 5);
}

void nd_iic_scl_output(void)
{
    nd_pin_reg_clear(GPIOC_OEN, BIT(4));
    nd_pin_reg_clear(GPIOC_IE, BIT(4));
}

void nd_iic_scl_set(int high)
{
    nd_pin_write(GPIOC_DAT, 4, high);
}
