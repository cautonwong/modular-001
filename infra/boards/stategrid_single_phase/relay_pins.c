#include "infra/boards/stategrid_single_phase/relay_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_relay_on_pulse(int active)
{
    nd_pin_write(GPIOE_DAT, 11, active);
    nd_pin_reg_clear(GPIOE_OEN, BIT(11));
}

void nd_relay_off_pulse(int active)
{
    nd_pin_write(GPIOC_DAT, 11, active);
    nd_pin_reg_clear(GPIOC_OEN, BIT(11));
}
