#include "domain/metering/metering_input.h"
#include <stddef.h>

int nd_metering_input_read(const nd_metering_input_port_t *port,
                           nd_metering_input_t input, int32_t *value)
{
    if (port != NULL && port->read != NULL && value != NULL)
    {
        return port->read(port->ctx, input, value);
    }
    return 0;
}
