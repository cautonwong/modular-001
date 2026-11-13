#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_client.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/cosem/nd_idis_mbus.h"

static nd_mbus_client_t *client_for(nd_idis_mbus_t *mbus, const nd_obis_t *ln,
                                    uint8_t *channel)
{
    if (mbus == NULL || ln == NULL || ln->a != 0 || ln->b < 1
        || ln->b > ND_MBUS_CHANNELS || ln->c != 24 || ln->d != 1
        || ln->e != 0 || ln->f != 255) {
        return NULL;
    }
    if (channel != NULL) {
        *channel = ln->b;
    }
    return &mbus->state.clients[ln->b - 1];
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        *value = nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_zero(nd_axdr_reader_t *in)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_INTEGER || tag == ND_AXDR_UNSIGNED) {
        (void)nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        (void)nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        (void)nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

int nd_ic_mbus_client_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out)
{
    nd_idis_mbus_t *mbus = ctx;
    uint8_t channel = 0;
    nd_mbus_client_t *client = client_for(mbus, ln, &channel);
    (void)client_sap;
    (void)selector;
    if (client == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2: {
        uint8_t port = client->port_index >= 1 && client->port_index <= 2
            ? client->port_index
            : 1;
        uint8_t port_ln[6] = {0, (uint8_t)(port - 1), 24, 6, 0, 255};
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, port_ln, sizeof(port_ln));
        break;
    }
    case 3: {
        uint8_t index = channel > 0 ? (uint8_t)(channel - 1) : 0;
        nd_axdr_put_array_header(out, mbus->capture_count[index]);
        for (uint8_t item = 0; item < mbus->capture_count[index]; item++) {
            const nd_idis_mbus_capture_t *capture =
                &mbus->captures[index][item];
            nd_axdr_put_structure_header(out, 4);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, capture->class_id);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, 6);
            nd_axdr_put_octets(out, (const uint8_t *)&capture->ln, 6);
            nd_axdr_put_tag(out, ND_AXDR_INTEGER);
            nd_axdr_put_u8(out, capture->attribute);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, capture->index);
        }
        break;
    }
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, client->capture_period);
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->primary_address);
        break;
    case 6:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, client->identification_number);
        break;
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, client->manufacturer_id);
        break;
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->version);
        break;
    case 9:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->device_type);
        break;
    case 10:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->access_number);
        break;
    case 11:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->status);
        break;
    case 12:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, client->alarm);
        break;
    case 13:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, client->configuration);
        break;
    case 14:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, client->key_status);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_mbus_client_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_mbus_t *mbus = ctx;
    uint8_t channel = 0;
    nd_mbus_client_t *client = client_for(mbus, ln, &channel);
    (void)client_sap;
    if (client == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 3) {
        uint8_t channel = 0;
        if (client_for(mbus, ln, &channel) == NULL
            || nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_IDIS_MBUS_CAPTURE_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_idis_mbus_capture_t captures[ND_IDIS_MBUS_CAPTURE_MAX] = {0};
        for (uint16_t item = 0; item < count; item++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) < 4) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint32_t class_id = 0;
            if (read_unsigned(in, &class_id) != 0 || class_id > 0xFFFF
                || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
                || nd_axdr_get_length(in) != 6) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, (uint8_t *)&captures[item].ln, 6);
            uint32_t attribute = 0;
            if (read_unsigned(in, &attribute) != 0 || attribute > 0xFF) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint32_t index = 0;
            if (read_unsigned(in, &index) != 0 || index > 0xFFFF) {
                return ND_ERR_TYPE_MISMATCH;
            }
            captures[item].class_id = (uint16_t)class_id;
            captures[item].attribute = (uint8_t)attribute;
            captures[item].index = (uint16_t)index;
        }
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(mbus->captures[channel - 1], captures, sizeof(captures));
        mbus->capture_count[channel - 1] = (uint8_t)count;
        return ND_OK;
    }
    uint32_t value = 0;
    if (read_unsigned(in, &value) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (attribute_id == 4) {
        client->capture_period = value;
        return ND_OK;
    }
    if (attribute_id == 5) {
        if (value > 250) {
            return ND_ERR_TYPE_MISMATCH;
        }
        /* Writing means the client may operate with this address immediately;
         * address 0 marks the slave as unconfigured. */
        client->primary_address = (uint8_t)value;
        return ND_OK;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

static void store_readout(nd_idis_mbus_t *mbus, uint8_t channel,
                          const nd_mbus_readout_t *readout)
{
    for (uint8_t index = 0; index < readout->value_count; index++) {
        nd_idis_mbus_value_t *value = &mbus->values[channel - 1][index];
        value->value = readout->values[index].value < 0
            ? 0
            : (uint32_t)readout->values[index].value;
        value->scaler = readout->values[index].scaler;
        value->unit = readout->values[index].unit;
        value->status = readout->status;
        value->capture_time = readout->time;
    }
    for (uint8_t index = readout->value_count;
         index < ND_IDIS_MBUS_VALUES_PER_CHANNEL; index++) {
        mbus->values[channel - 1][index] = (nd_idis_mbus_value_t){0};
    }
}

int nd_idis_mbus_on_second(nd_idis_mbus_t *mbus, uint8_t channel)
{
    if (mbus == NULL || channel < 1 || channel > ND_MBUS_CHANNELS) {
        return -1;
    }
    uint8_t index = (uint8_t)(channel - 1);
    uint32_t period = mbus->state.clients[index].capture_period;
    if (period == 0) {
        mbus->capture_timer[index] = 0;
        return 0;
    }
    if (++mbus->capture_timer[index] < period) {
        return 0;
    }
    mbus->capture_timer[index] = 0;
    nd_mbus_readout_t readout;
    if (nd_mbus_capture(&mbus->state, channel, &readout) != 0) {
        return -1;
    }
    store_readout(mbus, channel, &readout);
    return 1;
}

int nd_ic_mbus_client_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_mbus_t *mbus = ctx;
    uint8_t channel = 0;
    nd_mbus_client_t *client = client_for(mbus, ln, &channel);
    (void)client_sap;
    (void)out;
    if (client == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id == 1) {
        uint32_t address = 0;
        if (read_unsigned(in, &address) != 0 || address > 250) {
            return ND_ERR_TYPE_MISMATCH;
        }
        return nd_mbus_install(&mbus->state, channel, (uint8_t)address) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 2) {
        if (read_zero(in) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        return nd_mbus_deinstall(&mbus->state, channel) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 3) {
        if (read_zero(in) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_mbus_readout_t readout;
        if (nd_mbus_capture(&mbus->state, channel, &readout) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        store_readout(mbus, channel, &readout);
        return ND_OK;
    }
    if (method_id == 4) {
        if (read_zero(in) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        return nd_mbus_reset_alarm(&mbus->state, channel) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 5) {
        if (read_zero(in) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        return nd_mbus_sync_clock(&mbus->state, channel) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 6) {
        if (in == NULL) {
            return ND_ERR_TYPE_MISMATCH;
        }
        size_t start = in->pos;
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        /* The array is passed to the link driver as A-XDR bytes; encoding it
         * into EN 13757 frames belongs to the M-Bus protocol stack. */
        size_t length = (in->pos - start) + nd_axdr_remaining(in);
        return nd_mbus_data_send(&mbus->state, channel, in->buf + start,
                                 length) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 7 || method_id == 8) {
        if (in == NULL || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        if (length > ND_MBUS_KEY_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t key[ND_MBUS_KEY_MAX];
        nd_axdr_get_octets(in, key, length);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        int result = method_id == 7
            ? nd_mbus_set_key(&mbus->state, channel, key, length)
            : nd_mbus_transfer_key(&mbus->state, channel, key, length);
        return result == 0 ? ND_OK : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}
