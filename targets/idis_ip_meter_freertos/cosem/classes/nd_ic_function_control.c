#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_function_control.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

static void add_function(nd_function_control_state_t *state, const char *name)
{
    if (state->count >= ND_FUNCTION_CONTROL_MAX) {
        return;
    }
    nd_function_status_t *item = &state->functions[state->count++];
    size_t length = strlen(name);
    if (length > ND_FUNCTION_NAME_MAX) {
        length = ND_FUNCTION_NAME_MAX;
    }
    memcpy(item->name, name, length);
    item->length = (uint8_t)length;
    item->enabled = 1;
}

void nd_ic_function_control_init(nd_function_control_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    add_function(&state->local, "LPCAP_1");
    add_function(&state->local, "LPCAP_2");
    add_function(&state->local, "IF_LO_2W");
    add_function(&state->mbus, "LPCAP_M1");
    add_function(&state->mbus, "LPCAP_M2");
    add_function(&state->mbus, "LPCAP_M3");
    add_function(&state->mbus, "LPCAP_M4");
}

static nd_function_control_state_t *state_for(nd_function_control_t *state,
                                              const nd_obis_t *ln)
{
    if (state == NULL || ln == NULL) {
        return NULL;
    }
    if (nd_obis_eq(ln, &ND_OBIS_FUNCTION_CONTROL)) {
        return &state->local;
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_FUNCTION_CONTROL)) {
        return &state->mbus;
    }
    return NULL;
}

static nd_function_status_t *find_function(nd_function_control_state_t *state,
                                           const char *name, uint8_t length)
{
    for (uint8_t index = 0; index < state->count; index++) {
        nd_function_status_t *item = &state->functions[index];
        if (item->length == length
            && memcmp(item->name, name, length) == 0) {
            return item;
        }
    }
    return NULL;
}

static int read_name(nd_axdr_reader_t *in, char *name, uint8_t *length)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag != ND_AXDR_OCTET_STRING && tag != ND_AXDR_VISIBLE_STRING) {
        return -1;
    }
    uint16_t size = nd_axdr_get_length(in);
    if (size == 0 || size > ND_FUNCTION_NAME_MAX) {
        return -1;
    }
    nd_axdr_get_octets(in, (uint8_t *)name, size);
    *length = (uint8_t)size;
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

int nd_ic_function_control_get(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               const nd_axdr_reader_t *selector,
                               nd_axdr_writer_t *out)
{
    nd_function_control_state_t *state = state_for(ctx, ln);
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
        nd_axdr_put_array_header(out, state->count);
        for (uint8_t index = 0; index < state->count; index++) {
            const nd_function_status_t *item = &state->functions[index];
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->length);
            nd_axdr_put_octets(out, (const uint8_t *)item->name, item->length);
            nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
            nd_axdr_put_u8(out, item->enabled);
        }
    } else if (attribute_id == 3) {
        nd_axdr_put_array_header(out, state->count);
        for (uint8_t index = 0; index < state->count; index++) {
            const nd_function_status_t *item = &state->functions[index];
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->length);
            nd_axdr_put_octets(out, (const uint8_t *)item->name, item->length);
            nd_axdr_put_array_header(out, 0);
        }
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_function_control_action(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t method_id,
                                  nd_axdr_reader_t *in,
                                  nd_axdr_writer_t *out)
{
    nd_function_control_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    (void)out;
    if (state == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return ND_ERR_TYPE_MISMATCH;
    }
    uint16_t count = nd_axdr_get_length(in);

    /* Validate the whole list before changing anything. */
    nd_axdr_reader_t check = *in;
    for (uint16_t index = 0; index < count; index++) {
        if (nd_axdr_get_tag(&check) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(&check) != 2) {
            return ND_ERR_TYPE_MISMATCH;
        }
        char name[ND_FUNCTION_NAME_MAX];
        uint8_t length = 0;
        if (read_name(&check, name, &length) != 0
            || nd_axdr_get_tag(&check) != ND_AXDR_BOOLEAN
            || find_function(state, name, length) == NULL) {
            return ND_ERR_TYPE_MISMATCH;
        }
        (void)nd_axdr_get_u8(&check);
    }
    if (!nd_axdr_reader_ok(&check) || nd_axdr_remaining(&check) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }

    for (uint16_t index = 0; index < count; index++) {
        (void)nd_axdr_get_tag(in);
        (void)nd_axdr_get_length(in);
        char name[ND_FUNCTION_NAME_MAX];
        uint8_t length = 0;
        (void)read_name(in, name, &length);
        (void)nd_axdr_get_tag(in);
        uint8_t enabled = nd_axdr_get_u8(in);
        nd_function_status_t *item = find_function(state, name, length);
        if (item != NULL) {
            item->enabled = enabled != 0;
        }
    }
    return nd_axdr_reader_ok(in) && nd_axdr_remaining(in) == 0
               ? ND_OK
               : ND_ERR_TYPE_MISMATCH;
}
