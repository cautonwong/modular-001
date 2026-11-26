#include "domain/common/timer.h"
#include <stddef.h>

void nd_timer_start_one(const nd_timer_port_t *port, uint8_t timer_id,
                        uint32_t count)
{
    if (port != NULL && port->start_one != NULL)
    {
        port->start_one(port->ctx, timer_id, count);
    }
}
