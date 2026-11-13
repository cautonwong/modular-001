#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_calendar.h"

#include <stddef.h>
#include <string.h>

#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

static nd_calendar_bank_t bank_view(nd_idis_calendar_state_t *state,
                                    nd_idis_calendar_bank_t *storage)
{
    return (nd_calendar_bank_t){
        .name = storage->name,
        .season_count = storage->season_count,
        .week_count = storage->week_count,
        .day_count = storage->day_count,
        .seasons = storage->seasons,
        .weeks = storage->weeks,
        .days = storage->days,
        .special_day_count = state->special_day_count,
        .special_days = state->special_days,
    };
}

static void bind_days(nd_idis_calendar_bank_t *storage)
{
    for (uint8_t index = 0; index < ND_IDIS_CAL_MAX_DAYS; index++) {
        storage->days[index].actions = storage->actions[index];
        storage->days[index].action_count = storage->action_count[index];
    }
}

static int is_calendar_ln(const nd_obis_t *ln)
{
    for (uint8_t index = 0; index < 4; index++) {
        if (nd_obis_eq(ln, &ND_OBIS_ACTIVITY_CALENDARS[index])) {
            return 1;
        }
    }
    return 0;
}

static int is_special_days_ln(const nd_obis_t *ln)
{
    for (uint8_t index = 0; index < 4; index++) {
        if (nd_obis_eq(ln, &ND_OBIS_SPECIAL_DAYS_TABLES[index])) {
            return 1;
        }
    }
    return 0;
}

void nd_idis_calendar_init(nd_idis_calendar_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    bind_days(&state->active);
    bind_days(&state->passive);
    state->calendar.active = bank_view(state, &state->active);
    state->calendar.passive = bank_view(state, &state->passive);
}


void nd_idis_calendar_apply_defaults(nd_idis_calendar_state_t *state,
                                     const nd_obis_t *script_ln,
                                     uint16_t selector)
{
    if (state == NULL || script_ln == NULL) {
        return;
    }
    nd_idis_calendar_bank_t *bank = &state->active;
    memset(bank, 0, sizeof(*bank));
    bank->name = (nd_calendar_name_t){
        .len = 7, .data = {'D', 'E', 'F', 'A', 'U', 'L', 'T'}};
    bank->season_count = 1;
    bank->seasons[0].name = (nd_calendar_name_t){.len = 2, .data = {'S', '1'}};
    bank->seasons[0].start.wildcard_mask =
        ND_CALENDAR_WILD_YEAR | ND_CALENDAR_WILD_MONTH
        | ND_CALENDAR_WILD_DAY | ND_CALENDAR_WILD_HOUR
        | ND_CALENDAR_WILD_MINUTE | ND_CALENDAR_WILD_SECOND;
    bank->seasons[0].week =
        (nd_calendar_name_t){.len = 2, .data = {'W', '1'}};
    bank->week_count = 1;
    bank->weeks[0].name = (nd_calendar_name_t){.len = 2, .data = {'W', '1'}};
    for (uint8_t day = 0; day < 7; day++) {
        bank->weeks[0].day_ids[day] = 1;
    }
    bank->day_count = 1;
    bank->days[0].day_id = 1;
    bank->action_count[0] = 1;
    bank->actions[0][0] = (nd_calendar_action_t){
        .start = {.hour = 0, .minute = 0, .second = 0, .hundredths = 0},
        .script_selector = selector,
    };
    memcpy(bank->actions[0][0].script_ln, script_ln, 6);
    bind_days(bank);
    state->calendar.active = bank_view(state, bank);
    state->calendar.passive = bank_view(state, &state->passive);
}

static void deep_activate(nd_idis_calendar_state_t *state)
{
    state->active = state->passive;
    bind_days(&state->active);
    state->calendar.active = bank_view(state, &state->active);
    state->calendar.passive = bank_view(state, &state->passive);
    state->calendar.has_passive = 0;
}

static int read_string(nd_axdr_reader_t *in, uint8_t *data, uint8_t max,
                       uint8_t *length)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
        return -1;
    }
    uint16_t len = nd_axdr_get_length(in);
    if (len > max) {
        return -1;
    }
    nd_axdr_get_octets(in, data, len);
    *length = (uint8_t)len;
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_name(nd_axdr_reader_t *in, nd_calendar_name_t *name)
{
    return read_string(in, name->data, ND_CALENDAR_NAME_MAX, &name->len);
}

static int read_datetime(nd_axdr_reader_t *in, nd_calendar_datetime_t *out)
{
    uint8_t data[12];
    uint8_t length = 0;
    if (read_string(in, data, sizeof(data), &length) != 0 || length != 12) {
        return -1;
    }
    *out = (nd_calendar_datetime_t){0};
    if (data[0] == 0xFF && data[1] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_YEAR;
    } else {
        out->year = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    }
    if (data[2] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_MONTH;
    } else {
        out->month = data[2];
    }
    if (data[3] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_DAY;
    } else {
        out->day = data[3];
    }
    out->hour = data[5] == 0xFF ? 0 : data[5];
    out->minute = data[6] == 0xFF ? 0 : data[6];
    out->second = data[7] == 0xFF ? 0 : data[7];
    if (data[5] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_HOUR;
    }
    if (data[6] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_MINUTE;
    }
    if (data[7] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_SECOND;
    }
    return 0;
}

static void put_string(nd_axdr_writer_t *out, const uint8_t *data, uint8_t len)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, len);
    nd_axdr_put_octets(out, data, len);
}

static void put_datetime(nd_axdr_writer_t *out,
                         const nd_calendar_datetime_t *value)
{
    uint8_t data[12] = {0};
    data[0] = (uint8_t)(value->year >> 8);
    data[1] = (uint8_t)value->year;
    data[2] = value->month;
    data[3] = value->day;
    data[5] = value->hour;
    data[6] = value->minute;
    data[7] = value->second;
    if (value->wildcard_mask & ND_CALENDAR_WILD_YEAR) {
        data[0] = data[1] = 0xFF;
    }
    if (value->wildcard_mask & ND_CALENDAR_WILD_MONTH) {
        data[2] = 0xFF;
    }
    if (value->wildcard_mask & ND_CALENDAR_WILD_DAY) {
        data[3] = 0xFF;
    }
    if (value->wildcard_mask & ND_CALENDAR_WILD_HOUR) {
        data[5] = 0xFF;
    }
    if (value->wildcard_mask & ND_CALENDAR_WILD_MINUTE) {
        data[6] = 0xFF;
    }
    if (value->wildcard_mask & ND_CALENDAR_WILD_SECOND) {
        data[7] = 0xFF;
    }
    put_string(out, data, sizeof(data));
}

static void put_name(nd_axdr_writer_t *out, const nd_calendar_name_t *name)
{
    put_string(out, name->data, name->len);
}

static void put_bank(nd_axdr_writer_t *out, const nd_idis_calendar_bank_t *bank,
                     uint8_t attribute_id)
{
    if (attribute_id == 3 || attribute_id == 7) {
        nd_axdr_put_array_header(out, bank->season_count);
        for (uint8_t index = 0; index < bank->season_count; index++) {
            const nd_calendar_season_t *season = &bank->seasons[index];
            nd_axdr_put_structure_header(out, 3);
            put_name(out, &season->name);
            put_datetime(out, &season->start);
            put_name(out, &season->week);
        }
    } else if (attribute_id == 4 || attribute_id == 8) {
        nd_axdr_put_array_header(out, bank->week_count);
        for (uint8_t index = 0; index < bank->week_count; index++) {
            const nd_calendar_week_t *week = &bank->weeks[index];
            nd_axdr_put_structure_header(out, 8);
            put_name(out, &week->name);
            for (uint8_t day = 0; day < 7; day++) {
                nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
                nd_axdr_put_u8(out, week->day_ids[day]);
            }
        }
    } else if (attribute_id == 5 || attribute_id == 9) {
        nd_axdr_put_array_header(out, bank->day_count);
        for (uint8_t index = 0; index < bank->day_count; index++) {
            const nd_calendar_day_t *day = &bank->days[index];
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, day->day_id);
            nd_axdr_put_array_header(out, day->action_count);
            for (uint8_t action = 0; action < day->action_count; action++) {
                const nd_calendar_action_t *item = &day->actions[action];
                nd_axdr_put_structure_header(out, 3);
                put_string(out, (const uint8_t *)&item->start, 4);
                put_string(out, item->script_ln, sizeof(item->script_ln));
                nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
                nd_axdr_put_u16(out, item->script_selector);
            }
        }
    }
}

int nd_ic_calendar_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out)
{
    nd_idis_calendar_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || out == NULL
        || !is_calendar_ln(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        put_string(out, (const uint8_t *)ln, 6);
    } else if (attribute_id == 2) {
        put_name(out, &state->active.name);
    } else if (attribute_id >= 3 && attribute_id <= 5) {
        put_bank(out, &state->active, attribute_id);
    } else if (attribute_id == 6) {
        put_name(out, &state->passive.name);
    } else if (attribute_id >= 7 && attribute_id <= 9) {
        put_bank(out, &state->passive, attribute_id);
    } else if (attribute_id == 10) {
        put_datetime(out, &state->calendar.activate_time);
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int read_bank(nd_axdr_reader_t *in, nd_idis_calendar_bank_t *bank,
                     uint8_t attribute_id)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (attribute_id == 7) {
        if (count > ND_IDIS_CAL_MAX_SEASONS) {
            return -1;
        }
        bank->season_count = (uint8_t)count;
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) != 3) {
                return -1;
            }
            nd_calendar_season_t *season = &bank->seasons[index];
            if (read_name(in, &season->name) != 0
                || read_datetime(in, &season->start) != 0
                || read_name(in, &season->week) != 0) {
                return -1;
            }
        }
    } else if (attribute_id == 8) {
        if (count > ND_IDIS_CAL_MAX_WEEKS) {
            return -1;
        }
        bank->week_count = (uint8_t)count;
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) != 8) {
                return -1;
            }
            nd_calendar_week_t *week = &bank->weeks[index];
            if (read_name(in, &week->name) != 0) {
                return -1;
            }
            for (uint8_t day = 0; day < 7; day++) {
                if (nd_axdr_get_tag(in) != ND_AXDR_UNSIGNED) {
                    return -1;
                }
                week->day_ids[day] = nd_axdr_get_u8(in);
            }
        }
    } else if (attribute_id == 9) {
        if (count > ND_IDIS_CAL_MAX_DAYS) {
            return -1;
        }
        bank->day_count = (uint8_t)count;
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) != 2
                || nd_axdr_get_tag(in) != ND_AXDR_UNSIGNED) {
                return -1;
            }
            bank->days[index].day_id = nd_axdr_get_u8(in);
            if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
                return -1;
            }
            uint16_t action_count = nd_axdr_get_length(in);
            if (action_count > ND_IDIS_CAL_MAX_ACTIONS) {
                return -1;
            }
            bank->action_count[index] = (uint8_t)action_count;
            bank->days[index].actions = bank->actions[index];
            bank->days[index].action_count = (uint8_t)action_count;
            for (uint16_t action = 0; action < action_count; action++) {
                if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                    || nd_axdr_get_length(in) != 3) {
                    return -1;
                }
                nd_calendar_action_t *item = &bank->actions[index][action];
                uint8_t time_length = 0;
                uint8_t script_length = 0;
                if (read_string(in, (uint8_t *)&item->start, 4,
                                &time_length) != 0
                    || time_length != 4
                    || read_string(in, item->script_ln, 6,
                                   &script_length) != 0
                    || script_length != 6
                    || nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
                    return -1;
                }
                item->script_selector = nd_axdr_get_u16(in);
            }
        }
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

int nd_ic_calendar_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_calendar_state_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL
        || !is_calendar_ln(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 6) {
        if (read_name(in, &state->passive.name) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->calendar.passive = bank_view(state, &state->passive);
        state->calendar.has_passive = 1;
        return ND_OK;
    }
    if (attribute_id == 10) {
        if (read_datetime(in, &state->calendar.activate_time) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->calendar.has_passive = 1;
        return ND_OK;
    }
    if (attribute_id >= 7 && attribute_id <= 9) {
        if (read_bank(in, &state->passive, attribute_id) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->calendar.passive = bank_view(state, &state->passive);
        state->calendar.has_passive = 1;
        return ND_OK;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

int nd_ic_calendar_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out)
{
    nd_idis_calendar_state_t *state = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    if (state == NULL || ln == NULL
        || !is_calendar_ln(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1 || !state->calendar.has_passive) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    deep_activate(state);
    return ND_OK;
}

#define ND_IDIS_CALENDAR_MAGIC 0x43414C31u

typedef struct {
    uint32_t magic;
    nd_idis_calendar_bank_t active;
    nd_idis_calendar_bank_t passive;
    nd_calendar_datetime_t activate_time;
    uint8_t has_passive;
    nd_calendar_special_day_t special_days[ND_IDIS_SPECIAL_DAYS_MAX];
    uint16_t special_index[ND_IDIS_SPECIAL_DAYS_MAX];
    uint8_t special_day_count;
} nd_idis_calendar_record_t;

static void flatten_bank(nd_idis_calendar_bank_t *bank)
{
    for (uint8_t index = 0; index < ND_IDIS_CAL_MAX_DAYS; index++) {
        bank->days[index].actions = NULL;
    }
}

int nd_idis_calendar_save(const nd_idis_calendar_state_t *state,
                          nd_store_t *store, uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_calendar_record_t record = {
        .magic = ND_IDIS_CALENDAR_MAGIC,
        .active = state->active,
        .passive = state->passive,
        .activate_time = state->calendar.activate_time,
        .has_passive = state->calendar.has_passive,
        .special_day_count = state->special_day_count,
    };
    memcpy(record.special_days, state->special_days,
           sizeof(record.special_days));
    memcpy(record.special_index, state->special_index,
           sizeof(record.special_index));
    flatten_bank(&record.active);
    flatten_bank(&record.passive);
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_calendar_load(nd_idis_calendar_state_t *state, nd_store_t *store,
                          uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_calendar_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_CALENDAR_MAGIC
        || record.special_day_count > ND_IDIS_SPECIAL_DAYS_MAX) {
        return -1;
    }
    state->active = record.active;
    state->passive = record.passive;
    state->calendar.activate_time = record.activate_time;
    state->calendar.has_passive = record.has_passive;
    state->special_day_count = record.special_day_count;
    memcpy(state->special_days, record.special_days,
           sizeof(state->special_days));
    memcpy(state->special_index, record.special_index,
           sizeof(state->special_index));
    bind_days(&state->active);
    bind_days(&state->passive);
    state->calendar.active = bank_view(state, &state->active);
    state->calendar.passive = bank_view(state, &state->passive);
    return 0;
}

int nd_idis_calendar_tick(nd_idis_calendar_state_t *state,
                          const nd_datetime_t *now)
{
    if (state == NULL || now == NULL || !state->calendar.has_passive) {
        return 0;
    }
    if (!nd_calendar_activation_due(&state->calendar, now)) {
        return 0;
    }
    deep_activate(state);
    return 1;
}

static int read_special_date(nd_axdr_reader_t *in, nd_calendar_datetime_t *out)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 5) {
        return -1;
    }
    uint8_t data[5];
    nd_axdr_get_octets(in, data, sizeof(data));
    *out = (nd_calendar_datetime_t){0};
    if (data[0] == 0xFF && data[1] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_YEAR;
    } else {
        out->year = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    }
    if (data[2] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_MONTH;
    } else {
        out->month = data[2];
    }
    if (data[3] == 0xFF) {
        out->wildcard_mask |= ND_CALENDAR_WILD_DAY;
    } else {
        out->day = data[3];
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static void put_special_date(nd_axdr_writer_t *out,
                             const nd_calendar_datetime_t *date)
{
    uint8_t data[5] = {0};
    data[0] = (uint8_t)(date->year >> 8);
    data[1] = (uint8_t)date->year;
    data[2] = date->month;
    data[3] = date->day;
    if (date->wildcard_mask & ND_CALENDAR_WILD_YEAR) {
        data[0] = data[1] = 0xFF;
    }
    if (date->wildcard_mask & ND_CALENDAR_WILD_MONTH) {
        data[2] = 0xFF;
    }
    if (date->wildcard_mask & ND_CALENDAR_WILD_DAY) {
        data[3] = 0xFF;
    }
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, sizeof(data));
    nd_axdr_put_octets(out, data, sizeof(data));
}

static int same_special_date(const nd_calendar_datetime_t *left,
                             const nd_calendar_datetime_t *right)
{
    return left->year == right->year && left->month == right->month
        && left->day == right->day
        && left->wildcard_mask == right->wildcard_mask;
}

int nd_ic_special_days_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                           uint8_t attribute_id,
                           const nd_axdr_reader_t *selector,
                           nd_axdr_writer_t *out)
{
    nd_idis_calendar_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || out == NULL
        || !is_special_days_ln(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    } else if (attribute_id == 2) {
        nd_axdr_put_array_header(out, state->special_day_count);
        for (uint8_t index = 0; index < state->special_day_count; index++) {
            nd_axdr_put_structure_header(out, 3);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, state->special_index[index]);
            put_special_date(out, &state->special_days[index].day);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, state->special_days[index].day_id);
        }
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_special_days_action(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t method_id,
                              nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_calendar_state_t *state = ctx;
    (void)client_sap;
    (void)out;
    if (state == NULL || ln == NULL || in == NULL
        || !is_special_days_ln(ln)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id == 1) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 3
            || nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t entry_index = nd_axdr_get_u16(in);
        nd_calendar_datetime_t date;
        if (read_special_date(in, &date) != 0
            || nd_axdr_get_tag(in) != ND_AXDR_UNSIGNED) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t day_id = nd_axdr_get_u8(in);
        int slot = -1;
        for (uint8_t index = 0; index < state->special_day_count; index++) {
            if (state->special_index[index] == entry_index
                || same_special_date(&state->special_days[index].day, &date)) {
                slot = index;
                break;
            }
        }
        if (slot < 0) {
            if (state->special_day_count >= ND_IDIS_SPECIAL_DAYS_MAX) {
                return ND_ERR_OPERATION_NOT_POSSIBLE;
            }
            slot = state->special_day_count++;
        }
        state->special_index[slot] = entry_index;
        state->special_days[slot].day = date;
        state->special_days[slot].day_id = day_id;
        state->calendar.active = bank_view(state, &state->active);
        state->calendar.passive = bank_view(state, &state->passive);
        return ND_OK;
    }
    if (method_id == 2) {
        if (nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t entry_index = nd_axdr_get_u16(in);
        for (uint8_t index = 0; index < state->special_day_count; index++) {
            if (state->special_index[index] == entry_index) {
                for (uint8_t move = index; move + 1 < state->special_day_count;
                     move++) {
                    state->special_index[move] = state->special_index[move + 1];
                    state->special_days[move] = state->special_days[move + 1];
                }
                state->special_day_count--;
                state->calendar.active = bank_view(state, &state->active);
                state->calendar.passive = bank_view(state, &state->passive);
                return ND_OK;
            }
        }
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}
