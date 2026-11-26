#include "targets/stategrid_meter/app/timer_adapter.h"
#include "domain/common/timer.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include <stddef.h>

static void board_start_one(void *ctx, uint8_t timer_id, uint32_t count)
{
    (void)ctx;
    ndv_timer_start_one(timer_id, count);
}

void stategrid_timer_attach(stategrid_context_t *instance)
{
    instance->timer.ctx = NULL;
    instance->timer.start_one = board_start_one;
}
