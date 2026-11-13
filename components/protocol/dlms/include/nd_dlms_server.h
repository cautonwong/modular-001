#ifndef ND_DLMS_SERVER_H
#define ND_DLMS_SERVER_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"

typedef struct {
    const nd_cosem_port_t *port;
} nd_dlms_server_t;

int nd_dlms_server_get(const nd_dlms_server_t *server, uint8_t client_sap,
                       uint16_t class_id, const nd_obis_t *ln,
                       uint8_t attribute_id,
                       const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out);
int nd_dlms_server_set(const nd_dlms_server_t *server, uint8_t client_sap,
                       uint16_t class_id, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_dlms_server_action(const nd_dlms_server_t *server, uint8_t client_sap,
                          uint16_t class_id, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out);

#endif
