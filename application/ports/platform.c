#include "application/ports/platform.h"

#include <stddef.h>

static const nd_platform_port_t *bound;

void nd_platform_bind(const nd_platform_port_t *port)
{
    bound = port;
}

void nd_platform_watchdog_kick(void)
{
    if ((bound != NULL) && (bound->watchdog_kick != NULL)) {
        bound->watchdog_kick(bound->ctx);
    }
}

void nd_platform_event_output_set(bool enable)
{
    if ((bound != NULL) && (bound->event_output_set != NULL)) {
        bound->event_output_set(bound->ctx, enable);
    }
}
