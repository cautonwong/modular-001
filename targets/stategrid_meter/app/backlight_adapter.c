#include "targets/stategrid_meter/app/backlight_adapter.h"
#include "targets/stategrid_meter/app/display/backlight.h"
#include "targets/stategrid_meter/board/board_io.h"
#include <stddef.h>

static void board_backlight_set(void *ctx, int on)
{
    (void)ctx;
    nd_board_backlight_set(on);
}

void stategrid_backlight_attach(stategrid_context_t *instance)
{
    instance->backlight.ctx = NULL;
    instance->backlight.set = board_backlight_set;
}
