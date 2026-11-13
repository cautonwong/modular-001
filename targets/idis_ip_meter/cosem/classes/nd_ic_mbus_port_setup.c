#include "targets/idis_ip_meter/cosem/classes/nd_ic_mbus_port_setup.h"

#include "targets/idis_ip_meter/cosem/nd_idis_mbus.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static uint8_t *speed_for(nd_idis_mbus_t *mbus, const nd_obis_t *ln)
{
    if (mbus == NULL || ln == NULL) {
        return NULL;
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_PORT_SETUP_1)) {
        return &mbus->port_speed[0];
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_PORT_SETUP_2)) {
        return &mbus->port_speed[1];
    }
    return NULL;
}

int nd_ic_mbus_port_setup_get(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t attribute_id,
                              const nd_axdr_reader_t *selector,
                              nd_axdr_writer_t *out)
{
    uint8_t *speed = speed_for(ctx, ln);
    (void)client_sap;
    (void)selector;
    if (speed == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    } else if (attribute_id == 2) {
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, *speed);
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}
