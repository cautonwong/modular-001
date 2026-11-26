#include "application/ports/platform.h"

#include <stddef.h>

void nd_platform_watchdog_kick(const nd_platform_port_t *port)
{
    if ((port != NULL) && (port->watchdog_kick != NULL)) {
        port->watchdog_kick(port->ctx);
    }
}

void nd_platform_event_output_set(const nd_platform_port_t *port, bool enable)
{
    if ((port != NULL) && (port->event_output_set != NULL)) {
        port->event_output_set(port->ctx, enable);
    }
}

bool nd_platform_device_error(const nd_platform_port_t *port)
{
    if ((port != NULL) && (port->device_error != NULL)) {
        return port->device_error(port->ctx);
    }
    return false;
}

void nd_platform_device_error_reported(const nd_platform_port_t *port)
{
    if ((port != NULL) && (port->device_error_reported != NULL)) {
        port->device_error_reported(port->ctx);
    }
}
