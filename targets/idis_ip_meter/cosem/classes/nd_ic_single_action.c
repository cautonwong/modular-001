#include "targets/idis_ip_meter/cosem/classes/nd_ic_single_action.h"

#include <string.h>

#include "domain/common/calendar.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_SINGLE_ACTION_MAGIC 0x53415331u

typedef struct {
    uint32_t magic;
    nd_single_action_state_t items[ND_SINGLE_ACTION_MAX];
} nd_idis_single_action_record_t;

static const nd_obis_t single_action_lns[ND_SINGLE_ACTION_MAX] = {
    ND_OBIS_SINGLE_ACTION_BILLING,
    ND_OBIS_SINGLE_ACTION_DISCONNECT,
    ND_OBIS_SINGLE_ACTION_IMAGE,
    ND_OBIS_SINGLE_ACTION_FUNCTION,
    ND_OBIS_SINGLE_ACTION_MBUS_DISCONNECT,
    ND_OBIS_SINGLE_ACTION_MBUS_FUNCTION,
    ND_OBIS_SINGLE_ACTION_PUSH_1,
    ND_OBIS_SINGLE_ACTION_PUSH_2,
    ND_OBIS_SINGLE_ACTION_PUSH_3,
    ND_OBIS_SINGLE_ACTION_PUSH_4,
};

void nd_idis_single_action_init(nd_single_action_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->count = ND_SINGLE_ACTION_MAX;
    for (uint8_t index = 0; index < ND_SINGLE_ACTION_MAX; index++) {
        state->items[index].ln = single_action_lns[index];
    }
}


static nd_single_action_state_t *state_for(nd_single_action_t *state,
                                           const nd_obis_t *ln);

void nd_idis_single_action_apply_defaults(nd_single_action_t *state)
{
    if (state == NULL) {
        return;
    }
    nd_single_action_state_t *billing = state_for(state,
                                                  &ND_OBIS_SINGLE_ACTION_BILLING);
    if (billing == NULL) {
        return;
    }
    int configured = billing->time_count != 0;
    for (uint8_t index = 0; index < sizeof(billing->script_ln); index++) {
        if (billing->script_ln[index] != 0) {
            configured = 1;
            break;
        }
    }
    if (configured) {
        return;
    }
    static const uint8_t script[6] = {0, 0, 10, 0, 1, 255};
    memcpy(billing->script_ln, script, sizeof(script));
    billing->selector = 1;
    billing->type = 1;
    billing->time_count = 1;
    billing->times[0] = (nd_single_action_time_t){
        .time = {0, 0, 0, 0},
        .time_len = 4,
        .date = {0xFF, 0xFF, 0xFF, 0x01, 0xFF},
        .date_len = 5,
    };
}

static nd_single_action_state_t *state_for(nd_single_action_t *state,
                                           const nd_obis_t *ln)
{
    for (uint8_t index = 0; state != NULL && index < state->count; index++) {
        if (nd_obis_eq(&state->items[index].ln, ln)) {
            return &state->items[index];
        }
    }
    return NULL;
}

int nd_ic_single_action_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                            uint8_t attribute_id,
                            const nd_axdr_reader_t *selector,
                            nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    nd_single_action_state_t *state =
        objects != NULL ? state_for(&objects->single_action, ln) : NULL;
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
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, state->script_ln, 6);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->selector);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->type);
        break;
    case 4:
        nd_axdr_put_array_header(out, state->time_count);
        for (uint8_t index = 0; index < state->time_count; index++) {
            const nd_single_action_time_t *item = &state->times[index];
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->time_len);
            nd_axdr_put_octets(out, item->time, item->time_len);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->date_len);
            nd_axdr_put_octets(out, item->date, item->date_len);
        }
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int set_time_list(nd_single_action_state_t *state, nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > ND_SINGLE_ACTION_TIMES_MAX) {
        return -1;
    }
    nd_single_action_time_t times[ND_SINGLE_ACTION_TIMES_MAX];
    memset(times, 0, sizeof(times));
    for (uint16_t index = 0; index < count; index++) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return -1;
        }
        uint16_t length = nd_axdr_get_length(in);
        if (length > sizeof(times[index].time)) {
            return -1;
        }
        nd_axdr_get_octets(in, times[index].time, length);
        times[index].time_len = (uint8_t)length;
        if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return -1;
        }
        length = nd_axdr_get_length(in);
        if (length > sizeof(times[index].date)) {
            return -1;
        }
        nd_axdr_get_octets(in, times[index].date, length);
        times[index].date_len = (uint8_t)length;
    }
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    memcpy(state->times, times, sizeof(times));
    state->time_count = (uint8_t)count;
    return 0;
}

int nd_ic_single_action_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                            uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_objects_t *objects = ctx;
    nd_single_action_state_t *state =
        objects != NULL ? state_for(&objects->single_action, ln) : NULL;
    (void)client_sap;
    if (state == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
            || nd_axdr_get_length(in) != 6) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t script[6];
        nd_axdr_get_octets(in, script, 6);
        uint32_t selector = 0;
        if (read_unsigned(in, &selector) != 0 || selector > 0xFFFF) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(state->script_ln, script, sizeof(script));
        state->selector = (uint16_t)selector;
        return ND_OK;
    }
    if (attribute_id == 3) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t type = nd_axdr_get_u8(in);
        if (type < 1 || type > 5 || !nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->type = type;
        return ND_OK;
    }
    if (attribute_id == 4) {
        return set_time_list(state, in) == 0 ? ND_OK
                                             : ND_ERR_TYPE_MISMATCH;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

static int byte_match(uint8_t pattern, uint8_t value)
{
    return pattern == 0xFF || pattern == value;
}

static int date_matches(const nd_single_action_time_t *item,
                        const nd_datetime_t *now)
{
    if (item->date_len == 0) {
        return 1;
    }
    if (item->date_len != 5) {
        return 0;
    }
    uint16_t year = (uint16_t)(((uint16_t)item->date[0] << 8) | item->date[1]);
    if (year != 0xFFFF && year != now->year) {
        return 0;
    }
    if (!byte_match(item->date[2], now->month)
        || !byte_match(item->date[3], now->day)) {
        return 0;
    }
    uint8_t weekday = item->date[4];
    if (weekday == 0xFF || weekday == 0) {
        return 1;
    }
    uint8_t actual = nd_calendar_weekday(now->day, now->month, now->year);
    return actual == 0 ? weekday == 7 : actual == weekday;
}

static int time_matches(const nd_single_action_time_t *item,
                        const nd_datetime_t *now)
{
    return item->time_len == 4 && byte_match(item->time[0], now->hour)
        && byte_match(item->time[1], now->minute)
        && byte_match(item->time[2], now->second)
        && (item->time[3] == 0 || item->time[3] == 0xFF);
}

static int script_configured(const uint8_t script_ln[6])
{
    for (uint8_t index = 0; index < 6; index++) {
        if (script_ln[index] != 0) {
            return 1;
        }
    }
    return 0;
}

int nd_idis_single_action_on_second(nd_single_action_t *state,
                                    const nd_cosem_port_t *port,
                                    const nd_script_state_t *scripts,
                                    uint8_t client_sap,
                                    const nd_datetime_t *now)
{
    if (state == NULL || port == NULL || scripts == NULL || now == NULL) {
        return -1;
    }
    int executed = 0;
    for (uint8_t index = 0; index < state->count; index++) {
        nd_single_action_state_t *item = &state->items[index];
        if (item->time_count == 0 || !script_configured(item->script_ln)) {
            continue;
        }
        for (uint8_t slot = 0; slot < item->time_count; slot++) {
            if (!date_matches(&item->times[slot], now)
                || !time_matches(&item->times[slot], now)) {
                continue;
            }
            if (item->executed_valid && item->executed_index == slot
                && item->executed_year == now->year
                && item->executed_month == now->month
                && item->executed_day == now->day
                && item->executed_hour == now->hour
                && item->executed_minute == now->minute
                && item->executed_second == now->second) {
                continue;
            }
            nd_obis_t script;
            memcpy(&script, item->script_ln, sizeof(script));
            if (nd_idis_script_execute(scripts, port, client_sap, &script,
                                       item->selector) == ND_OK) {
                item->executed_valid = 1;
                item->executed_year = now->year;
                item->executed_month = now->month;
                item->executed_day = now->day;
                item->executed_hour = now->hour;
                item->executed_minute = now->minute;
                item->executed_second = now->second;
                item->executed_index = slot;
                executed++;
            }
        }
    }
    return executed;
}

int nd_idis_single_action_save(const nd_single_action_t *state,
                               nd_store_t *store, uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_single_action_record_t record = {
        .magic = ND_IDIS_SINGLE_ACTION_MAGIC};
    memcpy(record.items, state->items, sizeof(record.items));
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_single_action_load(nd_single_action_t *state, nd_store_t *store,
                               uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_single_action_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_SINGLE_ACTION_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_SINGLE_ACTION_MAX; index++) {
        state->items[index] = record.items[index];
        state->items[index].ln = single_action_lns[index];
    }
    state->count = ND_SINGLE_ACTION_MAX;
    return 0;
}
