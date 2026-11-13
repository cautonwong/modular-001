#include "infra/boards/stategrid_single_phase/board_io.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

/* Display. */
void nd_board_backlight_set(int on)
{
    nd_pin_write(GPIOB_DAT, 4, on);
    nd_pin_reg_clear(GPIOB_OEN, BIT(4));
}

/* Communication module event output. */
void nd_board_plc_event_set(int on)
{
    nd_pin_write(GPIOC_DAT, 10, on);
    nd_pin_reg_clear(GPIOC_OEN, BIT(10));
}

/* Relay LED and feedback. */
void nd_board_relay_led_set(int on)
{
    nd_pin_write(GPIOE_DAT, 12, on);
    nd_pin_reg_clear(GPIOE_OEN, BIT(12));
}

void nd_board_relay_feedback_input_enable(void)
{
    nd_pin_reg_set(GPIOC_IE, BIT(8));
    nd_pin_reg_set(GPIOC_OEN, BIT(8));
}

int nd_board_relay_is_off(void)
{
    return nd_pin_read(GPIOC_STS, 8) == 1;
}

/* Meter cover. */
void nd_board_cover_input_enable(void)
{
    nd_pin_reg_set(GPIOA_IE, BIT(14));
    nd_pin_reg_set(GPIOA_OEN, BIT(14));
}

int nd_board_cover_closed(void)
{
    return nd_pin_read(GPIOA_STS, 14) == 1;
}

/* Display key. */
void nd_board_key_input_enable(void)
{
    nd_pin_reg_set(GPIOA_IE, BIT(2));
    nd_pin_reg_set(GPIOA_OEN, BIT(2));
}

int nd_board_key_released(void)
{
    return nd_pin_read(GPIOA_STS, 2) == 1;
}

int nd_board_key_pressed(void)
{
    return nd_pin_read(GPIOA_STS, 2) == 0;
}
