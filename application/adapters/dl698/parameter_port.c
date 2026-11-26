#include "application/adapters/dl698/parameter_port.h"

int dl698_parameter_read(const nd_dl698_parameter_port_t *port,
                         dl698_parameter_t parameter, uint8_t *buffer,
                         size_t length)
{
    if (port == NULL || port->read == NULL || parameter >= DL698_PARAM_COUNT) {
        return -1;
    }
    return port->read(port->ctx, parameter, buffer, length);
}

int dl698_parameter_write(const nd_dl698_parameter_port_t *port,
                          dl698_parameter_t parameter,
                          const uint8_t *buffer, size_t length)
{
    if (port == NULL || port->write == NULL || parameter >= DL698_PARAM_COUNT) {
        return -1;
    }
    return port->write(port->ctx, parameter, buffer, length);
}
