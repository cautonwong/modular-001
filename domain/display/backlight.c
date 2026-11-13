#include "domain/display/backlight.h"
#include <stddef.h>

static const nd_backlight_port_t *bound;

void nd_backlight_bind(const nd_backlight_port_t *port)
{
    bound = port;
}

void nd_backlight_set(int on)
{
    if (bound != NULL && bound->set != NULL)
    {
        bound->set(bound->ctx, on);
    }
}
