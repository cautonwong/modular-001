#include "components/protocol/dlms/include/nd_dlms_server.h"

int nd_dlms_server_get(const nd_dlms_server_t *server, uint8_t client_sap,
                       uint16_t class_id, const nd_obis_t *ln,
                       uint8_t attribute_id,
                       const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out)
{
    if (server == NULL || server->port == NULL || server->port->get == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return server->port->get(server->port->ctx, client_sap, class_id, ln,
                             attribute_id, selector, out);
}

int nd_dlms_server_set(const nd_dlms_server_t *server, uint8_t client_sap,
                       uint16_t class_id, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in)
{
    if (server == NULL || server->port == NULL || server->port->set == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return server->port->set(server->port->ctx, client_sap, class_id, ln,
                             attribute_id, in);
}

int nd_dlms_server_action(const nd_dlms_server_t *server, uint8_t client_sap,
                          uint16_t class_id, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out)
{
    if (server == NULL || server->port == NULL || server->port->action == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return server->port->action(server->port->ctx, client_sap, class_id, ln,
                                method_id, in, out);
}
