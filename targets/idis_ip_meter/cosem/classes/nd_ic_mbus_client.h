#ifndef ND_IC_MBUS_CLIENT_H
#define ND_IC_MBUS_CLIENT_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

int nd_ic_mbus_client_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out);
int nd_ic_mbus_client_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_mbus_client_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
