#include "domain/load_control/relay_status.h"
#include <stddef.h>

int nd_relay_status_is_off(const nd_relay_status_port_t *port)
{
    if (port != NULL && port->is_off != NULL)
    {
        return port->is_off(port->ctx);
    }
    return 0;
}
