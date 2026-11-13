#include "infra/boards/stategrid_single_phase/esam_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_esam_cs_set(int high)
{
    nd_pin_reg_clear(GPIOC_OEN, BIT(0));
    nd_pin_reg_set(GPIOC_IE, BIT(0));
    nd_pin_write(GPIOC_DAT, 0, high);
}

void nd_esam_power_set(int on)
{
    nd_pin_reg_clear(GPIOC_OEN, BIT(7));
    nd_pin_reg_set(GPIOC_IE, BIT(7));
    nd_pin_write(GPIOC_DAT, 7, on ? 0 : 1);
}
