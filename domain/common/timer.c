#include "domain/common/timer.h"
#include <stddef.h>

static const nd_timer_port_t *bound;

void nd_timer_bind(const nd_timer_port_t *port)
{
    bound = port;
}

void nd_timer_start_one(uint8_t timer_id, uint32_t count)
{
    if (bound != NULL && bound->start_one != NULL)
    {
        bound->start_one(bound->ctx, timer_id, count);
    }
}
