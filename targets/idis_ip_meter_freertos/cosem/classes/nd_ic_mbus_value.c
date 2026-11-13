#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_value.h"

#include "targets/idis_ip_meter_freertos/cosem/nd_idis_mbus.h"

int nd_ic_mbus_value_matches(const nd_obis_t *ln)
{
    return ln != NULL && ln->a == 0 && ln->b >= 1 && ln->b <= 4
        && ln->c == 24 && ln->d == 2 && ln->e >= 1 && ln->e <= 8
        && ln->f == 255;
}

static nd_idis_mbus_value_t *value_for(nd_idis_mbus_t *mbus,
                                       const nd_obis_t *ln)
{
    if (mbus == NULL || !nd_ic_mbus_value_matches(ln)) {
        return NULL;
    }
    return &mbus->values[ln->b - 1][ln->e - 1];
}

static void put_datetime(nd_axdr_writer_t *out, const nd_datetime_t *time)
{
    uint8_t buffer[12] = {0};
    buffer[0] = (uint8_t)(time->year >> 8);
    buffer[1] = (uint8_t)time->year;
    buffer[2] = time->month;
    buffer[3] = time->day;
    buffer[5] = time->hour;
    buffer[6] = time->minute;
    buffer[7] = time->second;
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, sizeof(buffer));
    nd_axdr_put_octets(out, buffer, sizeof(buffer));
}

int nd_ic_mbus_value_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id,
                         const nd_axdr_reader_t *selector,
                         nd_axdr_writer_t *out)
{
    nd_idis_mbus_value_t *value = value_for(ctx, ln);
    (void)client_sap;
    (void)selector;
    if (value == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, value->value);
        break;
    case 3:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)value->scaler);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, value->unit);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, value->status);
        break;
    case 5:
        put_datetime(out, &value->capture_time);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_mbus_value_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_mbus_value_t *value = value_for(ctx, ln);
    (void)client_sap;
    if (value == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id != 3) { /* only scaler_unit is writable (IDIS option) */
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 2
        || nd_axdr_get_tag(in) != ND_AXDR_INTEGER) {
        return ND_ERR_TYPE_MISMATCH;
    }
    int8_t scaler = (int8_t)nd_axdr_get_u8(in);
    if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return ND_ERR_TYPE_MISMATCH;
    }
    uint8_t unit = nd_axdr_get_u8(in);
    if (!nd_axdr_reader_ok(in)) {
        return ND_ERR_TYPE_MISMATCH;
    }
    value->scaler = scaler;
    value->unit = unit;
    return ND_OK;
}

int nd_ic_mbus_value_action(void *ctx, uint8_t client_sap,
                            const nd_obis_t *ln, uint8_t method_id,
                            nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_mbus_value_t *value = value_for(ctx, ln);
    (void)client_sap;
    (void)out;
    if (value == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1) { /* reset */
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_INTEGER || nd_axdr_get_u8(in) != 0
        || !nd_axdr_reader_ok(in)) {
        return ND_ERR_TYPE_MISMATCH;
    }
    value->value = 0;
    value->status = 0;
    value->capture_time = (nd_datetime_t){0};
    return ND_OK;
}
