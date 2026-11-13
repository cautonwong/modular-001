#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_activation.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static void init_state(nd_register_activation_state_t *state, uint8_t d)
{
    /* Default mask list "1".."4" and four tariff register assignments. */
    state->mask_count = 4;
    state->assign_count = 4;
    for (uint8_t index = 0; index < 4; index++) {
        state->masks[index][0] = (uint8_t)('1' + index);
        state->mask_len[index] = 1;
        state->assign[index] = (nd_obis_t){1, 0, 1, d,
                                           (uint8_t)(index + 1), 255};
    }
}

void nd_ic_register_activation_init(nd_register_activation_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    init_state(&state->energy, 8);
    init_state(&state->demand, 6);
}

static nd_register_activation_state_t *state_for(
    nd_register_activation_t *state, const nd_obis_t *ln)
{
    if (state == NULL || ln == NULL) {
        return NULL;
    }
    if (nd_obis_eq(ln, &ND_OBIS_TARIFF_ACTIVE_MASK_1)) {
        return &state->energy;
    }
    if (nd_obis_eq(ln, &ND_OBIS_TARIFF_ACTIVE_MASK_2)) {
        return &state->demand;
    }
    return NULL;
}

uint8_t nd_ic_register_activation_tariff(
    const nd_register_activation_t *state)
{
    if (state != NULL && state->energy.length > 0
        && state->energy.mask[0] >= '1' && state->energy.mask[0] <= '4') {
        return (uint8_t)(state->energy.mask[0] - '0');
    }
    return 1;
}

int nd_ic_register_activation_get(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t attribute_id,
                                  const nd_axdr_reader_t *selector,
                                  nd_axdr_writer_t *out)
{
    nd_register_activation_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    (void)selector;
    if (state == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    } else if (attribute_id == 2) {
        nd_axdr_put_array_header(out, state->assign_count);
        for (uint8_t index = 0; index < state->assign_count; index++) {
            nd_axdr_put_structure_header(out, 4);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, 3); /* target class: register */
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, 6);
            nd_axdr_put_octets(out, (const uint8_t *)&state->assign[index], 6);
            nd_axdr_put_tag(out, ND_AXDR_INTEGER);
            nd_axdr_put_u8(out, 2); /* value attribute */
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, 0);
        }
    } else if (attribute_id == 3) {
        nd_axdr_put_array_header(out, state->mask_count);
        for (uint8_t index = 0; index < state->mask_count; index++) {
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, state->mask_len[index]);
            nd_axdr_put_octets(out, state->masks[index],
                               state->mask_len[index]);
        }
    } else if (attribute_id == 4) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, state->length);
        nd_axdr_put_octets(out, state->mask, state->length);
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_register_activation_set(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t attribute_id,
                                  nd_axdr_reader_t *in)
{
    nd_register_activation_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    if (state == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_ACTIVATION_MASKS_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_obis_t assignment[ND_ACTIVATION_MASKS_MAX];
        uint8_t stored = 0;
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) < 2
                || (nd_axdr_get_tag(in) != ND_AXDR_UNSIGNED
                    && nd_axdr_get_tag(in) != ND_AXDR_INTEGER)) {
                return ND_ERR_TYPE_MISMATCH;
            }
            (void)nd_axdr_get_u8(in); /* class id is fixed to register */
            if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
                || nd_axdr_get_length(in) != 6) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, (uint8_t *)&assignment[stored], 6);
            stored++;
        }
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(state->assign, assignment, sizeof(assignment[0]) * stored);
        state->assign_count = stored;
        return ND_OK;
    }
    if (attribute_id == 3) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_ACTIVATION_MASKS_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t masks[ND_ACTIVATION_MASKS_MAX][ND_ACTIVATION_MASK_LEN];
        uint8_t lengths[ND_ACTIVATION_MASKS_MAX];
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint16_t length = nd_axdr_get_length(in);
            if (length > ND_ACTIVATION_MASK_LEN) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, masks[index], length);
            lengths[index] = (uint8_t)length;
        }
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(state->masks, masks, sizeof(masks[0]) * count);
        memcpy(state->mask_len, lengths, count);
        state->mask_count = (uint8_t)count;
        return ND_OK;
    }
    if (attribute_id != 4) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
        return ND_ERR_TYPE_MISMATCH;
    }
    uint16_t length = nd_axdr_get_length(in);
    if (length > ND_ACTIVE_MASK_MAX) {
        return ND_ERR_TYPE_MISMATCH;
    }
    nd_axdr_get_octets(in, state->mask, length);
    if (!nd_axdr_reader_ok(in)) {
        return ND_ERR_TYPE_MISMATCH;
    }
    state->length = (uint8_t)length;
    return ND_OK;
}
