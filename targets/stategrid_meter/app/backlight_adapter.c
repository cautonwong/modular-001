#include "targets/stategrid_meter/app/backlight_adapter.h"
#include "domain/display/backlight.h"
#include "infra/boards/stategrid_single_phase/board_io.h"
#include <stddef.h>

static void board_backlight_set(void *ctx, int on)
{
    (void)ctx;
    nd_board_backlight_set(on);
}

static const nd_backlight_port_t backlight_port = {
    .ctx = NULL,
    .set = board_backlight_set,
};

void stategrid_backlight_bind(void)
{
    nd_backlight_bind(&backlight_port);
}
