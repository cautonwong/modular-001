#include "targets/idis_ip_meter/board/relay_pins.h"
#include "targets/idis_ip_meter/board/pin_ops.h"
#include "target.h"

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
