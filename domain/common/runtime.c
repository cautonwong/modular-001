#include "domain/common/runtime.h"
#include <stddef.h>

static const nd_runtime_port_t *bound;

void nd_runtime_bind(const nd_runtime_port_t *port)
{
    bound = port;
}

void nd_runtime_delay_us(uint32_t us)
{
    if (bound != NULL && bound->delay_us != NULL)
    {
        bound->delay_us(bound->ctx, us);
    }
}

void nd_runtime_watchdog_clear(void)
{
    if (bound != NULL && bound->watchdog_clear != NULL)
    {
        bound->watchdog_clear(bound->ctx);
    }
}
