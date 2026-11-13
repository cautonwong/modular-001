#ifndef ND_IC_MBUS_PORT_SETUP_H
#define ND_IC_MBUS_PORT_SETUP_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

int nd_ic_mbus_port_setup_get(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t attribute_id,
                              const nd_axdr_reader_t *selector,
                              nd_axdr_writer_t *out);

#endif
