#include "targets/stategrid_meter/app/timer_adapter.h"
#include "domain/common/timer.h"
#include "time_link.h"
#include <stddef.h>

static void board_start_one(void *ctx, uint8_t timer_id, uint32_t count)
{
    (void)ctx;
    ndv_timer_start_one(timer_id, count);
}

static const nd_timer_port_t timer_port = {
    .ctx = NULL,
    .start_one = board_start_one,
};

void stategrid_timer_bind(void)
{
    nd_timer_bind(&timer_port);
}
