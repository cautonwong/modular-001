#include "targets/stategrid_meter/app/display/backlight.h"
#include <stddef.h>

void nd_backlight_set(const nd_backlight_port_t *port, int on)
{
    if (port != NULL && port->set != NULL)
    {
        port->set(port->ctx, on);
    }
}
