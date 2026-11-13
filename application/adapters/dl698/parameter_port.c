#include "application/adapters/dl698/parameter_port.h"

static const nd_dl698_parameter_port_t *bound;

void dl698_bind_parameters(const nd_dl698_parameter_port_t *port)
{
    bound = port;
}

int dl698_parameter_read(dl698_parameter_t parameter, uint8_t *buffer,
                         size_t length)
{
    if (bound == NULL || bound->read == NULL || parameter >= DL698_PARAM_COUNT) {
        return -1;
    }
    return bound->read(bound->ctx, parameter, buffer, length);
}

int dl698_parameter_write(dl698_parameter_t parameter,
                          const uint8_t *buffer, size_t length)
{
    if (bound == NULL || bound->write == NULL || parameter >= DL698_PARAM_COUNT) {
        return -1;
    }
    return bound->write(bound->ctx, parameter, buffer, length);
}
