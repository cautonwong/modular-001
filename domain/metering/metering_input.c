#include "domain/metering/metering_input.h"
#include <stddef.h>

static const nd_metering_input_port_t *bound;

void nd_metering_input_bind(const nd_metering_input_port_t *port)
{
    bound = port;
}

int nd_metering_input_read(nd_metering_input_t input, int32_t *value)
{
    if (bound != NULL && bound->read != NULL && value != NULL)
    {
        return bound->read(bound->ctx, input, value);
    }
    return 0;
}
