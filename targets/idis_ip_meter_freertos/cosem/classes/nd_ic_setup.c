#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_setup.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/cosem/classes/nd_idis_setup_table.h"

#define ND_SETUP_MAGIC 0x53455431u

static const nd_setup_desc_t *desc_for(const nd_obis_t *ln,
                                       uint8_t attribute, int *index)
{
    for (int slot = 0; slot < ND_SETUP_COUNT; slot++) {
        const nd_setup_desc_t *desc = &nd_setup_table[slot];
        if (desc->attr == attribute && memcmp(desc->ln, ln, 6) == 0) {
            if (index != NULL) {
                *index = slot;
            }
            return desc;
        }
    }
    return NULL;
}

static int object_exists_any(const nd_obis_t *ln)
{
    for (int slot = 0; slot < ND_SETUP_COUNT; slot++) {
        if (memcmp(nd_setup_table[slot].ln, ln, 6) == 0) {
            return 1;
        }
    }
    return 0;
}

void nd_idis_setup_init(nd_idis_setup_t *setup)
{
    if (setup == NULL) {
        return;
    }
    memset(setup, 0, sizeof(*setup));
}

void nd_idis_setup_bind(nd_idis_setup_t *setup, nd_store_t *store,
                        uint32_t offset)
{
    if (setup == NULL) {
        return;
    }
    setup->store = store;
    setup->offset = offset;
}

static uint32_t value_offset(const nd_idis_setup_t *setup, int index)
{
    return setup->offset + sizeof(uint32_t)
         + (uint32_t)index * sizeof(nd_setup_value_t);
}

static int load_value(const nd_idis_setup_t *setup, int index,
                      nd_setup_value_t *value)
{
    if (setup->store == NULL) {
        return -1;
    }
    return nd_store_read(setup->store, value_offset(setup, index),
                         (uint8_t *)value, sizeof(*value));
}

static int store_value(const nd_idis_setup_t *setup, int index,
                       const nd_setup_value_t *value)
{
    if (setup->store == NULL) {
        return -1;
    }
    return nd_store_write(setup->store, value_offset(setup, index),
                          (const uint8_t *)value, sizeof(*value));
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED || tag == ND_AXDR_ENUM
        || tag == ND_AXDR_BOOLEAN) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_INTEGER) {
        *value = (uint8_t)nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        *value = nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

int nd_ic_setup_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out)
{
    nd_idis_setup_t *setup = ctx;
    (void)client_sap;
    (void)selector;
    if (setup == NULL || ln == NULL || out == NULL
        || !object_exists_any(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    int index = 0;
    const nd_setup_desc_t *desc = desc_for(ln, attribute_id, &index);
    if (desc == NULL) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    nd_setup_value_t value_storage;
    if (load_value(setup, index, &value_storage) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    const nd_setup_value_t *value = &value_storage;
    switch (desc->type) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, (uint8_t)value->number);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, (uint8_t)value->number);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, (uint16_t)value->number);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, value->number);
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
        nd_axdr_put_u8(out, value->number != 0);
        break;
    case 6:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, value->length);
        nd_axdr_put_octets(out, value->blob, value->length);
        break;
    default:
        if (value->length > 0) {
            nd_axdr_put_octets(out, value->blob, value->length);
        } else {
            nd_axdr_put_array_header(out, 0);
        }
        break;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_setup_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_setup_t *setup = ctx;
    (void)client_sap;
    if (setup == NULL || ln == NULL || in == NULL
        || !object_exists_any(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    int index = 0;
    const nd_setup_desc_t *desc = desc_for(ln, attribute_id, &index);
    if (desc == NULL) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    nd_setup_value_t value_storage;
    if (load_value(setup, index, &value_storage) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_setup_value_t *value = &value_storage;
    if (desc->type <= 5) {
        uint32_t number = 0;
        if (read_unsigned(in, &number) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        value->number = number;
        return store_value(setup, index, value) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (desc->type == 6) {
        if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        uint8_t capacity = desc->max_len > ND_SETUP_BLOB_MAX
            ? ND_SETUP_BLOB_MAX
            : desc->max_len;
        if (length > capacity) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_axdr_get_octets(in, value->blob, length);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        value->length = (uint8_t)length;
        return store_value(setup, index, value) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    /* Complex type: keep the raw A-XDR encoding and replay it on GET. */
    size_t remaining = nd_axdr_remaining(in);
    if (remaining == 0 || remaining > ND_SETUP_BLOB_MAX) {
        return ND_ERR_TYPE_MISMATCH;
    }
    memcpy(value->blob, in->buf + in->pos, remaining);
    value->length = (uint8_t)remaining;
    return store_value(setup, index, value) == 0
               ? ND_OK
               : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_setup_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out)
{
    nd_idis_setup_t *setup = ctx;
    (void)client_sap;
    (void)in;
    if (setup == NULL || ln == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id == 1) {
        for (int slot = 0; slot < ND_SETUP_COUNT; slot++) {
            if (memcmp(nd_setup_table[slot].ln, ln, 6) == 0) {
                /* reset: counters/objects return to their defaults */
                nd_setup_value_t value = {
                    .number = (uint32_t)nd_setup_table[slot].default_value,
                };
                if (store_value(setup, slot, &value) != 0) {
                    return ND_ERR_OPERATION_NOT_POSSIBLE;
                }
            }
        }
        return ND_OK;
    }
    if (method_id == 2 || method_id == 3) {
        if (out != NULL) {
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, 0);
        }
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_idis_setup_save(const nd_idis_setup_t *setup, nd_store_t *store,
                       uint32_t offset)
{
    (void)setup;
    (void)store;
    (void)offset;
    /* Each successful SET writes its value through the bound store. */
    return 0;
}

int nd_idis_setup_load(nd_idis_setup_t *setup, nd_store_t *store,
                       uint32_t offset)
{
    if (setup == NULL || store == NULL) {
        return -1;
    }
    nd_idis_setup_bind(setup, store, offset);
    uint32_t magic = 0;
    if (nd_store_read(store, offset, (uint8_t *)&magic, sizeof(magic)) == 0
        && magic == ND_SETUP_MAGIC) {
        return 0;
    }
    magic = ND_SETUP_MAGIC;
    if (nd_store_write(store, offset, (const uint8_t *)&magic,
                       sizeof(magic)) != 0) {
        return -1;
    }
    for (int slot = 0; slot < ND_SETUP_COUNT; slot++) {
        nd_setup_value_t value = {
            .number = (uint32_t)nd_setup_table[slot].default_value,
        };
        if (store_value(setup, slot, &value) != 0) {
            return -1;
        }
    }
    return 0;
}
