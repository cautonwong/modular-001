#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_image_transfer.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_IMAGE_MAGIC 0x494D4731u

typedef struct {
    uint32_t magic;
    nd_image_state_t state; /* store pointer is cleared before writing */
} nd_idis_image_record_t;

void nd_idis_image_bind_store(nd_image_state_t *state, nd_store_t *store,
                              uint32_t offset)
{
    if (state != NULL) {
        state->store = store;
        state->store_offset = offset;
    }
}

void nd_idis_image_init(nd_image_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->ln = ND_OBIS_IMAGE_TRANSFER;
    state->block_size = 256;
    state->enabled = 1;
}

static uint8_t block_bit(const nd_image_state_t *state, uint32_t number)
{
    return (uint8_t)(state->blocks[(number - 1) / 8] & (1u << ((number - 1) % 8)));
}

static int stored_image_readable(const nd_image_state_t *state)
{
    if (state->store == NULL) {
        return 1; /* RAM-only model */
    }
    uint8_t buffer[256];
    for (uint32_t number = 1; number <= state->block_count; number++) {
        uint32_t remaining = state->image_size
            - (number - 1) * state->block_size;
        size_t length = remaining < state->block_size
            ? remaining : state->block_size;
        if (length > sizeof(buffer)
            || nd_store_read(state->store,
                             state->store_offset
                                 + (number - 1) * state->block_size,
                             buffer, length) != 0) {
            return 0;
        }
    }
    return 1;
}

static int all_blocks_transferred(const nd_image_state_t *state)
{
    for (uint32_t number = 1; number <= state->block_count; number++) {
        if (!block_bit(state, number)) {
            return 0;
        }
    }
    return state->block_count > 0;
}

static void update_first_missing(nd_image_state_t *state)
{
    for (uint32_t number = 1; number <= state->block_count; number++) {
        if (!block_bit(state, number)) {
            state->first_missing = number;
            return;
        }
    }
    state->first_missing = (uint32_t)state->block_count + 1;
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
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

int nd_ic_image_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out)
{
    nd_image_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || !nd_obis_eq(ln, &state->ln)
        || out == NULL) {
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
        nd_axdr_put_u32(out, state->block_size);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_BIT_STRING);
        nd_axdr_put_length(out, state->block_count);
        nd_axdr_put_octets(out, state->blocks,
                           (size_t)(state->block_count + 7u) / 8u);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->first_missing);
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
        nd_axdr_put_u8(out, state->enabled);
        break;
    case 6:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->status);
        break;
    case 7:
        nd_axdr_put_array_header(out, 0);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_image_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out)
{
    nd_image_state_t *state = ctx;
    (void)client_sap;
    (void)out;
    if (state == NULL || ln == NULL || !nd_obis_eq(ln, &state->ln)
        || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (!state->enabled) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 1) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        if (length > sizeof(state->id)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t id[16] = {0};
        nd_axdr_get_octets(in, id, length);
        uint32_t size = 0;
        if (read_unsigned(in, &size) != 0 || size == 0
            || state->block_size == 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint32_t blocks = (size + state->block_size - 1) / state->block_size;
        if (blocks == 0 || blocks > ND_IDIS_IMAGE_BLOCKS_MAX) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        memset(state->blocks, 0, sizeof(state->blocks));
        memcpy(state->id, id, length);
        state->id_len = (uint8_t)length;
        state->image_size = size;
        state->block_count = (uint8_t)blocks;
        state->first_missing = 1;
        state->verified = 0;
        state->status = 1;
        return ND_OK;
    }
    if (method_id == 2) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint32_t number = 0;
        if (read_unsigned(in, &number) != 0 || number < 1
            || number > state->block_count
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        uint8_t data[256];
        if (length > state->block_size || length > sizeof(data)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_axdr_get_octets(in, data, length);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (state->store != NULL) {
            uint32_t offset = state->store_offset
                + (number - 1) * state->block_size;
            if (nd_store_write(state->store, offset, data, length) != 0) {
                return ND_ERR_OPERATION_NOT_POSSIBLE;
            }
        }
        state->blocks[(number - 1) / 8] |= (uint8_t)(1u << ((number - 1) % 8));
        update_first_missing(state);
        return ND_OK;
    }
    if (method_id == 3) {
        if (read_unsigned(in, &(uint32_t){0}) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->status = all_blocks_transferred(state)
                                && stored_image_readable(state)
                            ? 3
                            : 4;
        state->verified = state->status == 3 ? 1 : 0;
        return ND_OK;
    }
    if (method_id == 4) {
        if (read_unsigned(in, &(uint32_t){0}) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (!state->verified) {
            state->status = all_blocks_transferred(state)
                                    && stored_image_readable(state)
                                ? 3
                                : 4;
            state->verified = state->status == 3 ? 1 : 0;
        }
        state->status = state->verified ? 6 : 7;
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_idis_image_save(const nd_image_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_image_record_t record = {.magic = ND_IDIS_IMAGE_MAGIC,
                                     .state = *state};
    record.state.store = NULL;
    record.state.store_offset = 0;
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_image_load(nd_image_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_image_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_IMAGE_MAGIC) {
        return -1;
    }
    *state = record.state;
    state->ln = ND_OBIS_IMAGE_TRANSFER;
    state->store = NULL;
    state->store_offset = 0;
    return 0;
}
