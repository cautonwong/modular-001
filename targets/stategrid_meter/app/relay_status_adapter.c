#include "targets/stategrid_meter/app/relay_status_adapter.h"
#include "domain/load_control/relay_status.h"
#include "targets/stategrid_meter/board/board_io.h"
#include <stddef.h>

static int board_relay_is_off(void *ctx)
{
    (void)ctx;
    return nd_board_relay_is_off();
}

void stategrid_relay_status_attach(stategrid_context_t *instance)
{
    instance->relay_status.ctx = NULL;
    instance->relay_status.is_off = board_relay_is_off;
}
