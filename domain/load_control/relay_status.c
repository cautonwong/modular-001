#include "domain/load_control/relay_status.h"
#include <stddef.h>

static const nd_relay_status_port_t *bound;

void nd_relay_status_bind(const nd_relay_status_port_t *port)
{
    bound = port;
}

int nd_relay_status_is_off(void)
{
    if (bound != NULL && bound->is_off != NULL)
    {
        return bound->is_off(bound->ctx);
    }
    return 0;
}
