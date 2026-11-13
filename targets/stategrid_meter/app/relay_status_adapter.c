#include "targets/stategrid_meter/app/relay_status_adapter.h"
#include "domain/load_control/relay_status.h"
#include "infra/boards/stategrid_single_phase/board_io.h"
#include <stddef.h>

static int board_relay_is_off(void *ctx)
{
    (void)ctx;
    return nd_board_relay_is_off();
}

static const nd_relay_status_port_t relay_status_port = {
    .ctx = NULL,
    .is_off = board_relay_is_off,
};

void stategrid_relay_status_bind(void)
{
    nd_relay_status_bind(&relay_status_port);
}
