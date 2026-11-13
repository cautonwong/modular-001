#ifndef ND_IDIS_COSEM_PORT_H
#define ND_IDIS_COSEM_PORT_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

int nd_cosem_port_get(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      const nd_axdr_reader_t *selector,
                      nd_axdr_writer_t *out);
int nd_cosem_port_set(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      nd_axdr_reader_t *in);
int nd_cosem_port_action(void *ctx, uint8_t client_sap, uint16_t class_id,
                         const nd_obis_t *ln, uint8_t method_id,
                         nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
