#include "targets/idis_ip_meter/cosem/classes/nd_ic_clock.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_CLOCK_MAGIC 0x434C4B31u

typedef struct {
    uint32_t magic;
    nd_clock_config_t config;
    nd_datetime_t preset_time;
    uint16_t measuring_period;
    uint16_t time_shift_limit;
} nd_idis_clock_record_t;

static void put_datetime(nd_axdr_writer_t *out, const nd_dlms_datetime_t *time)
{
    nd_axdr_put_tag(out, ND_AXDR_DATE_TIME);
    nd_axdr_put_u16(out, time->year);
    nd_axdr_put_u8(out, time->month);
    nd_axdr_put_u8(out, time->day);
    nd_axdr_put_u8(out, time->day_of_week);
    nd_axdr_put_u8(out, time->hour);
    nd_axdr_put_u8(out, time->minute);
    nd_axdr_put_u8(out, time->second);
    nd_axdr_put_u8(out, time->hundredths);
    nd_axdr_put_i16(out, time->deviation);
    nd_axdr_put_u8(out, time->status);
}

static void put_clock_datetime(nd_axdr_writer_t *out,
                               const nd_clock_datetime_t *time)
{
    uint8_t data[12] = {0};
    data[0] = (uint8_t)(time->year >> 8);
    data[1] = (uint8_t)time->year;
    data[2] = time->month;
    data[3] = time->day;
    data[5] = time->hour;
    data[6] = time->minute;
    data[7] = time->second;
    if (time->wildcard_mask & ND_CLOCK_WILD_MONTH) {
        data[2] = 0xFF;
    }
    if (time->wildcard_mask & ND_CLOCK_WILD_DAY) {
        data[3] = 0xFF;
    }
    if (time->wildcard_mask & ND_CLOCK_WILD_HOUR) {
        data[5] = 0xFF;
    }
    if (time->wildcard_mask & ND_CLOCK_WILD_MINUTE) {
        data[6] = 0xFF;
    }
    if (time->wildcard_mask & ND_CLOCK_WILD_SECOND) {
        data[7] = 0xFF;
    }
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, sizeof(data));
    nd_axdr_put_octets(out, data, sizeof(data));
}

static int read_clock_datetime(nd_axdr_reader_t *in, nd_clock_datetime_t *out)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 12) {
        return -1;
    }
    uint8_t data[12];
    nd_axdr_get_octets(in, data, sizeof(data));
    *out = (nd_clock_datetime_t){0};
    if (data[2] == 0xFF) {
        out->wildcard_mask |= ND_CLOCK_WILD_MONTH;
    } else {
        out->month = data[2];
    }
    if (data[3] == 0xFF) {
        out->wildcard_mask |= ND_CLOCK_WILD_DAY;
    } else {
        out->day = data[3];
    }
    if (data[5] == 0xFF) {
        out->wildcard_mask |= ND_CLOCK_WILD_HOUR;
    } else {
        out->hour = data[5];
    }
    if (data[6] == 0xFF) {
        out->wildcard_mask |= ND_CLOCK_WILD_MINUTE;
    } else {
        out->minute = data[6];
    }
    if (data[7] == 0xFF) {
        out->wildcard_mask |= ND_CLOCK_WILD_SECOND;
    } else {
        out->second = data[7];
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_datetime(nd_axdr_reader_t *in, nd_datetime_t *out,
                         uint8_t *status)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_DATE_TIME) {
        return -1;
    }
    nd_datetime_t value = {0};
    value.year = nd_axdr_get_u16(in);
    value.month = nd_axdr_get_u8(in);
    value.day = nd_axdr_get_u8(in);
    (void)nd_axdr_get_u8(in);
    value.hour = nd_axdr_get_u8(in);
    value.minute = nd_axdr_get_u8(in);
    value.second = nd_axdr_get_u8(in);
    (void)nd_axdr_get_u8(in);
    (void)nd_axdr_get_i16(in);
    *status = nd_axdr_get_u8(in);
    *out = value;
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_time(nd_idis_clock_state_t *state, nd_datetime_t *time,
                     uint8_t *status)
{
    if (state->port == NULL || state->port->read == NULL) {
        return -1;
    }
    nd_dlms_datetime_t now;
    if (state->port->read(state->port->ctx, &now) != 0) {
        return -1;
    }
    *time = (nd_datetime_t){
        .second = now.second, .minute = now.minute, .hour = now.hour,
        .day = now.day, .month = now.month, .year = now.year,
    };
    *status = now.status;
    return 0;
}

static int write_time(nd_idis_clock_state_t *state, const nd_datetime_t *time,
                      uint8_t status)
{
    if (state->port == NULL || state->port->write == NULL
        || !nd_calendar_date_valid(time) || !nd_calendar_time_valid(time)) {
        return -1;
    }
    int16_t deviation = 0;
    (void)nd_clock_effective_deviation(&state->config, time, &deviation);
    uint8_t weekday = nd_calendar_weekday(time->day, time->month, time->year);
    nd_dlms_datetime_t now = {
        .year = time->year,
        .month = time->month,
        .day = time->day,
        .day_of_week = weekday == 0 ? 7 : weekday,
        .hour = time->hour,
        .minute = time->minute,
        .second = time->second,
        .deviation = deviation,
        .status = status,
    };
    return state->port->write(state->port->ctx, &now);
}

void nd_idis_clock_init(nd_idis_clock_state_t *state,
                        const nd_clock_port_t *port)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->port = port;
    state->measuring_period = 900;
    state->time_shift_limit = 60;
    nd_clock_config_init(&state->config);
}

int nd_ic_clock_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out)
{
    nd_idis_clock_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || state->port == NULL || ln == NULL || out == NULL
        || !nd_obis_eq(ln, &ND_OBIS_CLOCK)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (state->port->read == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_dlms_datetime_t now;
    if (state->port->read(state->port->ctx, &now) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_datetime_t local = {
        .second = now.second, .minute = now.minute, .hour = now.hour,
        .day = now.day, .month = now.month, .year = now.year,
    };
    switch (attribute_id) {
    case 2: {
        int16_t deviation = now.deviation;
        (void)nd_clock_effective_deviation(&state->config, &local, &deviation);
        now.deviation = deviation;
        put_datetime(out, &now);
        break;
    }
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_LONG);
        nd_axdr_put_i16(out, state->config.time_zone);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, now.status);
        break;
    case 5:
        put_clock_datetime(out, &state->config.daylight_begin);
        break;
    case 6:
        put_clock_datetime(out, &state->config.daylight_end);
        break;
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)(int8_t)state->config.daylight_deviation);
        break;
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
        nd_axdr_put_u8(out, state->config.daylight_enabled);
        break;
    case 9:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->config.clock_base);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_clock_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_clock_state_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL
        || !nd_obis_eq(ln, &ND_OBIS_CLOCK)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2) {
        nd_datetime_t time;
        uint8_t status = 0;
        if (read_datetime(in, &time, &status) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        return write_time(state, &time, status) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (attribute_id == 3) {
        if (nd_axdr_get_tag(in) != ND_AXDR_LONG) {
            return ND_ERR_TYPE_MISMATCH;
        }
        int16_t value = nd_axdr_get_i16(in);
        if (value < -720 || value > 840) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->config.time_zone = value;
        return ND_OK;
    }
    if (attribute_id == 5 || attribute_id == 6) {
        nd_clock_datetime_t value;
        if (read_clock_datetime(in, &value) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (attribute_id == 5) {
            state->config.daylight_begin = value;
        } else {
            state->config.daylight_end = value;
        }
        return ND_OK;
    }
    if (attribute_id == 7) {
        if (nd_axdr_get_tag(in) != ND_AXDR_INTEGER) {
            return ND_ERR_TYPE_MISMATCH;
        }
        int8_t value = (int8_t)nd_axdr_get_u8(in);
        if (value < -120 || value > 120) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->config.daylight_deviation = value;
        return ND_OK;
    }
    if (attribute_id == 8) {
        if (nd_axdr_get_tag(in) != ND_AXDR_BOOLEAN) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->config.daylight_enabled = nd_axdr_get_u8(in) != 0;
        return ND_OK;
    }
    if (attribute_id == 9) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t value = nd_axdr_get_u8(in);
        if (value > 5) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->config.clock_base = value;
        return ND_OK;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

int nd_ic_clock_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out)
{
    nd_idis_clock_state_t *state = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    if (state == NULL || ln == NULL
        || !nd_obis_eq(ln, &ND_OBIS_CLOCK)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_datetime_t time;
    uint8_t status = 0;
    if (read_time(state, &time, &status) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 1) {
        if (nd_clock_adjust_to_quarter(&time) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    } else if (method_id == 2) {
        if (nd_clock_adjust_to_period(&time, state->measuring_period) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    } else if (method_id == 3) {
        if (nd_clock_adjust_to_minute(&time) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    } else if (method_id == 4) {
        time = state->preset_time;
    } else if (method_id == 5) {
        state->preset_time = time;
        return ND_OK;
    } else if (method_id == 6) {
        int32_t seconds = 0;
        if (in != NULL && nd_axdr_get_tag(in) == ND_AXDR_INTEGER) {
            seconds = (int8_t)nd_axdr_get_u8(in);
        }
        if (nd_clock_shift(&time, seconds) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    } else {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return write_time(state, &time, status) == 0
               ? ND_OK
               : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_idis_clock_save(const nd_idis_clock_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_clock_record_t record = {
        .magic = ND_IDIS_CLOCK_MAGIC,
        .config = state->config,
        .preset_time = state->preset_time,
        .measuring_period = state->measuring_period,
        .time_shift_limit = state->time_shift_limit,
    };
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_clock_load(nd_idis_clock_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_clock_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_CLOCK_MAGIC) {
        return -1;
    }
    state->config = record.config;
    state->preset_time = record.preset_time;
    state->measuring_period = record.measuring_period;
    /* The field was reserved (always 0) before the shift limit was persisted;
     * keep the default limit for records written by that build. */
    state->time_shift_limit =
        record.time_shift_limit != 0 ? record.time_shift_limit : 60;
    return 0;
}
