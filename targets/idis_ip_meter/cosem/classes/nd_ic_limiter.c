#include "targets/idis_ip_meter/cosem/classes/nd_ic_limiter.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_LIMITER_MAGIC 0x4C494D31u

typedef struct {
    uint32_t magic;
    nd_limiter_state_t state;
} nd_idis_limiter_record_t;

void nd_idis_limiter_init(nd_limiter_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->ln = (nd_obis_t){0, 0, 17, 0, 0, 255};
    state->emergency_active = 0;
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

int nd_ic_limiter_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t attribute_id, const nd_axdr_reader_t *selector,
                      nd_axdr_writer_t *out)
{
    nd_limiter_state_t *state = ctx;
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
        nd_axdr_put_structure_header(out, 3);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->target_class);
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, state->target_ln, 6);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, state->target_attr);
        break;
    case 3:
    case 4:
    case 5: {
        uint32_t value = attribute_id == 3 ? state->threshold_active
            : attribute_id == 4                ? state->threshold_normal
                                               : state->threshold_emergency;
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, (uint16_t)value);
        break;
    }
    case 6:
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, attribute_id == 6 ? state->min_over_duration
                                               : state->min_under_duration);
        break;
    case 8: {
        uint8_t empty[12] = {0};
        nd_axdr_put_structure_header(out, 3);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->emergency_profile_id);
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, sizeof(empty));
        nd_axdr_put_octets(out, empty, sizeof(empty));
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->emergency_duration);
        break;
    }
    case 9:
        nd_axdr_put_array_header(out, 0);
        break;
    case 10:
        nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
        nd_axdr_put_u8(out, state->emergency_active);
        break;
    case 11:
        nd_axdr_put_structure_header(out, 2);
        for (uint8_t direction = 0; direction < 2; direction++) {
            const nd_limiter_action_t *action =
                direction == 0 ? &state->up : &state->down;
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, 6);
            nd_axdr_put_octets(out, action->script, 6);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, action->selector);
        }
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_limiter_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_limiter_state_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || !nd_obis_eq(ln, &state->ln)
        || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 3) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint32_t class_id = 0;
        if (read_unsigned(in, &class_id) != 0 || class_id > 0xFFFF
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
            || nd_axdr_get_length(in) != 6) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t target[6];
        nd_axdr_get_octets(in, target, 6);
        uint32_t attribute = 0;
        if (read_unsigned(in, &attribute) != 0 || attribute > 0xFF) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->target_class = (uint16_t)class_id;
        memcpy(state->target_ln, target, sizeof(target));
        state->target_attr = (uint8_t)attribute;
        return ND_OK;
    }
    if (attribute_id == 8) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 3) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint32_t id = 0;
        if (read_unsigned(in, &id) != 0 || id > 0xFFFF) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        uint8_t ignore[12];
        if (length > sizeof(ignore)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_axdr_get_octets(in, ignore, length);
        uint32_t duration = 0;
        if (read_unsigned(in, &duration) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->emergency_profile_id = (uint16_t)id;
        state->emergency_duration = duration;
        return ND_OK;
    }
    if (attribute_id == 11) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_limiter_action_t parsed[2];
        for (uint8_t direction = 0; direction < 2; direction++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) != 2
                || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
                || nd_axdr_get_length(in) != 6) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, parsed[direction].script, 6);
            uint32_t selector = 0;
            if (read_unsigned(in, &selector) != 0 || selector > 0xFFFF) {
                return ND_ERR_TYPE_MISMATCH;
            }
            parsed[direction].selector = (uint16_t)selector;
        }
        memcpy(&state->up, &parsed[0], sizeof(state->up));
        memcpy(&state->down, &parsed[1], sizeof(state->down));
        return ND_OK;
    }
    uint32_t value = 0;
    if (read_unsigned(in, &value) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (attribute_id == 3) {
        state->threshold_active = value;
    } else if (attribute_id == 4) {
        state->threshold_normal = value;
        if (state->threshold_active == 0) {
            state->threshold_active = value;
        }
    } else if (attribute_id == 5) {
        state->threshold_emergency = value;
    } else if (attribute_id == 6) {
        state->min_over_duration = value;
    } else if (attribute_id == 7) {
        state->min_under_duration = value;
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return ND_OK;
}

static int read_monitored(const nd_limiter_state_t *state,
                          const nd_cosem_port_t *port, uint8_t client_sap,
                          uint32_t *value)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_obis_t target;
    memcpy(&target, state->target_ln, sizeof(target));
    if (port->get == NULL
        || port->get(port->ctx, client_sap, state->target_class, &target,
                     state->target_attr, NULL, &out) != ND_OK) {
        return -1;
    }
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return read_unsigned(&in, value);
}

static int run_action(const nd_script_state_t *scripts,
                      const nd_cosem_port_t *port, uint8_t client_sap,
                      const nd_limiter_action_t *action)
{
    int configured = 0;
    for (uint8_t index = 0; index < 6; index++) {
        configured |= action->script[index] != 0;
    }
    if (!configured) {
        return 0;
    }
    nd_obis_t script;
    memcpy(&script, action->script, sizeof(script));
    return nd_idis_script_execute(scripts, port, client_sap, &script,
                                  action->selector) == ND_OK
               ? 1
               : 0;
}

int nd_idis_limiter_on_second(nd_limiter_state_t *state,
                              const nd_cosem_port_t *port,
                              const nd_script_state_t *scripts,
                              uint8_t client_sap)
{
    if (state == NULL || port == NULL || scripts == NULL
        || state->threshold_active == 0) {
        return 0;
    }
    uint32_t value = 0;
    if (read_monitored(state, port, client_sap, &value) != 0) {
        return 0;
    }
    uint8_t above = value >= state->threshold_active;
    int executed = 0;
    if (above) {
        state->under_timer = 0;
        if (state->over_timer < 0xFFFFFFFFu) {
            state->over_timer++;
        }
        if (state->above_valid && !state->above
            && state->over_timer >= state->min_over_duration) {
            executed += run_action(scripts, port, client_sap, &state->up);
            state->above = 1;
        }
    } else {
        state->over_timer = 0;
        if (state->under_timer < 0xFFFFFFFFu) {
            state->under_timer++;
        }
        if (state->above_valid && state->above
            && state->under_timer >= state->min_under_duration) {
            executed += run_action(scripts, port, client_sap, &state->down);
            state->above = 0;
        }
    }
    if (!state->above_valid) {
        state->above = above;
        state->above_valid = 1;
    }
    return executed;
}

int nd_idis_limiter_save(const nd_limiter_state_t *state, nd_store_t *store,
                         uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_limiter_record_t record = {.magic = ND_IDIS_LIMITER_MAGIC,
                                       .state = *state};
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_limiter_load(nd_limiter_state_t *state, nd_store_t *store,
                         uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_limiter_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_LIMITER_MAGIC) {
        return -1;
    }
    *state = record.state;
    state->ln = (nd_obis_t){0, 0, 17, 0, 0, 255};
    return 0;
}
