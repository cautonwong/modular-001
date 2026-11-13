#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_monitor.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_MONITOR_MAGIC 0x4D4F4E31u

typedef struct {
    uint32_t magic;
    nd_monitor_state_t items[ND_IDIS_MONITOR_MAX];
} nd_idis_monitor_record_t;

static const nd_obis_t monitor_lns[ND_IDIS_MONITOR_MAX] = {
    {0, 0, 16, 1, 0, 255}, /* Alarm monitor 1 */
    {0, 0, 16, 1, 1, 255}, /* Alarm monitor 2 */
    {1, 0, 31, 4, 0, 255}, /* Supervision monitor L1 */
    {1, 0, 51, 4, 0, 255}, /* Supervision monitor L2 */
    {1, 0, 71, 4, 0, 255}, /* Supervision monitor L3 */
};

void nd_idis_monitors_init(nd_idis_monitors_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->count = ND_IDIS_MONITOR_MAX;
    for (uint8_t index = 0; index < ND_IDIS_MONITOR_MAX; index++) {
        state->items[index].ln = monitor_lns[index];
    }
}

static nd_monitor_state_t *state_for(nd_idis_monitors_t *state,
                                     const nd_obis_t *ln)
{
    for (uint8_t index = 0; state != NULL && index < state->count; index++) {
        if (nd_obis_eq(&state->items[index].ln, ln)) {
            return &state->items[index];
        }
    }
    return NULL;
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

static int set_thresholds(nd_monitor_state_t *state, nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > ND_IDIS_MONITOR_THRESHOLDS) {
        return -1;
    }
    uint32_t values[ND_IDIS_MONITOR_THRESHOLDS];
    for (uint16_t index = 0; index < count; index++) {
        if (read_unsigned(in, &values[index]) != 0) {
            return -1;
        }
    }
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    memcpy(state->thresholds, values, sizeof(values));
    state->threshold_count = (uint8_t)count;
    return 0;
}

static int set_monitored(nd_monitor_state_t *state, nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 3) {
        return -1;
    }
    uint32_t class_id = 0;
    if (read_unsigned(in, &class_id) != 0 || class_id > 0xFFFF
        || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 6) {
        return -1;
    }
    uint8_t ln[6];
    nd_axdr_get_octets(in, ln, 6);
    uint32_t attribute = 0;
    if (read_unsigned(in, &attribute) != 0 || attribute > 0xFF
        || !nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    state->target_class = (uint16_t)class_id;
    memcpy(state->target_ln, ln, sizeof(ln));
    state->target_attr = (uint8_t)attribute;
    return 0;
}

static int set_actions(nd_monitor_state_t *state, nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > ND_IDIS_MONITOR_THRESHOLDS) {
        return -1;
    }
    nd_monitor_action_t up[ND_IDIS_MONITOR_THRESHOLDS];
    nd_monitor_action_t down[ND_IDIS_MONITOR_THRESHOLDS];
    memset(up, 0, sizeof(up));
    memset(down, 0, sizeof(down));
    for (uint16_t index = 0; index < count; index++) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2) {
            return -1;
        }
        for (uint8_t direction = 0; direction < 2; direction++) {
            nd_monitor_action_t *action = direction == 0 ? &up[index]
                                                         : &down[index];
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) != 2
                || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
                || nd_axdr_get_length(in) != 6) {
                return -1;
            }
            nd_axdr_get_octets(in, action->script, 6);
            uint32_t selector = 0;
            if (read_unsigned(in, &selector) != 0 || selector > 0xFFFF) {
                return -1;
            }
            action->selector = (uint16_t)selector;
        }
    }
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    memcpy(state->up, up, sizeof(up));
    memcpy(state->down, down, sizeof(down));
    return 0;
}

int nd_ic_register_monitor_get(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               const nd_axdr_reader_t *selector,
                               nd_axdr_writer_t *out)
{
    nd_monitor_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    (void)selector;
    if (state == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_array_header(out, state->threshold_count);
        for (uint8_t index = 0; index < state->threshold_count; index++) {
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, (uint16_t)state->thresholds[index]);
        }
        break;
    case 3:
        nd_axdr_put_structure_header(out, 3);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->target_class);
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, state->target_ln, 6);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, state->target_attr);
        break;
    case 4:
        nd_axdr_put_array_header(out, state->threshold_count);
        for (uint8_t index = 0; index < state->threshold_count; index++) {
            nd_axdr_put_structure_header(out, 2);
            for (uint8_t direction = 0; direction < 2; direction++) {
                const nd_monitor_action_t *action =
                    direction == 0 ? &state->up[index]
                                   : &state->down[index];
                nd_axdr_put_structure_header(out, 2);
                nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
                nd_axdr_put_length(out, 6);
                nd_axdr_put_octets(out, action->script, 6);
                nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
                nd_axdr_put_u16(out, action->selector);
            }
        }
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_register_monitor_set(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               nd_axdr_reader_t *in)
{
    nd_monitor_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    if (state == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    int result = -1;
    if (attribute_id == 2) {
        result = set_thresholds(state, in);
    } else if (attribute_id == 3) {
        result = set_monitored(state, in);
    } else if (attribute_id == 4) {
        result = set_actions(state, in);
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return result == 0 ? ND_OK : ND_ERR_TYPE_MISMATCH;
}

static int read_monitored(const nd_cosem_port_t *port, uint8_t client_sap,
                          const nd_monitor_state_t *state, uint32_t *value)
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
                      const nd_monitor_action_t *action)
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

int nd_idis_monitor_on_second(nd_idis_monitors_t *state,
                              const nd_cosem_port_t *port,
                              const nd_script_state_t *scripts,
                              uint8_t client_sap)
{
    if (state == NULL || port == NULL || scripts == NULL) {
        return -1;
    }
    int executed = 0;
    for (uint8_t index = 0; index < state->count; index++) {
        nd_monitor_state_t *item = &state->items[index];
        if (item->threshold_count == 0) {
            continue;
        }
        uint32_t value = 0;
        if (read_monitored(port, client_sap, item, &value) != 0) {
            continue;
        }
        int8_t level = -1;
        for (uint8_t threshold = 0; threshold < item->threshold_count;
             threshold++) {
            if (value >= item->thresholds[threshold]) {
                level = (int8_t)threshold;
            }
        }
        if (!item->last_valid) {
            item->last_valid = 1;
            item->last_value = value;
            item->last_level = level;
            continue;
        }
        if (level > item->last_level) {
            executed += run_action(scripts, port, client_sap,
                                   &item->up[level]);
        } else if (level < item->last_level && item->last_level >= 0) {
            executed += run_action(scripts, port, client_sap,
                                   &item->down[item->last_level]);
        }
        item->last_value = value;
        item->last_level = level;
    }
    return executed;
}

int nd_idis_monitor_save(const nd_idis_monitors_t *state, nd_store_t *store,
                         uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_monitor_record_t record = {.magic = ND_IDIS_MONITOR_MAGIC};
    memcpy(record.items, state->items, sizeof(record.items));
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_monitor_load(nd_idis_monitors_t *state, nd_store_t *store,
                         uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_monitor_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_MONITOR_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_MONITOR_MAX; index++) {
        state->items[index] = record.items[index];
        state->items[index].ln = monitor_lns[index];
    }
    state->count = ND_IDIS_MONITOR_MAX;
    return 0;
}
