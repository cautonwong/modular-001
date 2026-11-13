#include "targets/idis_ip_meter/cosem/nd_idis_capture.h"

#include "domain/common/calendar.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static int read_unsigned(nd_axdr_reader_t *in, uint64_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    switch (tag) {
    case ND_AXDR_UNSIGNED:
        *value = nd_axdr_get_u8(in);
        break;
    case ND_AXDR_LONG_UNSIGNED:
        *value = nd_axdr_get_u16(in);
        break;
    case ND_AXDR_DOUBLE_LONG_UNSIGNED:
        *value = nd_axdr_get_u32(in);
        break;
    default:
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_signed(nd_axdr_reader_t *in, int64_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    switch (tag) {
    case ND_AXDR_INTEGER:
        *value = (int8_t)nd_axdr_get_u8(in);
        break;
    case ND_AXDR_LONG:
        *value = nd_axdr_get_i16(in);
        break;
    case ND_AXDR_DOUBLE_LONG:
        *value = (int32_t)nd_axdr_get_u32(in);
        break;
    default:
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_clock(nd_axdr_reader_t *in, nd_clock_value_t *clock)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_DATE_TIME) {
        return -1;
    }
    clock->time.year = nd_axdr_get_u16(in);
    clock->time.month = nd_axdr_get_u8(in);
    clock->time.day = nd_axdr_get_u8(in);
    clock->day_of_week = nd_axdr_get_u8(in);
    clock->time.hour = nd_axdr_get_u8(in);
    clock->time.minute = nd_axdr_get_u8(in);
    clock->time.second = nd_axdr_get_u8(in);
    clock->hundredths = nd_axdr_get_u8(in);
    clock->deviation = nd_axdr_get_i16(in);
    clock->status = nd_axdr_get_u8(in);
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int data_is_u32(const nd_obis_t *ln)
{
    return nd_obis_eq(ln, &ND_OBIS_ALARM_REGISTER_1)
        || nd_obis_eq(ln, &ND_OBIS_EVENT_COUNTER_1);
}

int nd_idis_capture_read(const nd_capture_object_t *capture,
                         const nd_axdr_reader_t *in,
                         nd_capture_value_t *value)
{
    if (capture == NULL || in == NULL || value == NULL) {
        return -1;
    }
    nd_axdr_reader_t reader = *in;
    value->status = 0;
    if (capture->class_id == ND_IC_CLOCK && capture->attribute_id == 2) {
        value->type = ND_CAPTURE_CLOCK;
        return read_clock(&reader, &value->value.clock);
    }
    if ((capture->class_id == ND_IC_REGISTER
         || capture->class_id == ND_IC_EXTENDED_REGISTER)
        && capture->attribute_id == 2) {
        uint8_t tag = nd_axdr_get_tag(&reader);
        if (!nd_axdr_reader_ok(&reader)) {
            return -1;
        }
        if (tag == ND_AXDR_INTEGER || tag == ND_AXDR_LONG
            || tag == ND_AXDR_DOUBLE_LONG) {
            value->type = ND_CAPTURE_SIGNED;
            reader.pos--;
            return read_signed(&reader, &value->value.signed_value);
        }
        value->type = ND_CAPTURE_UNSIGNED;
        if (tag == ND_AXDR_UNSIGNED) {
            value->value.unsigned_value = nd_axdr_get_u8(&reader);
        } else if (tag == ND_AXDR_LONG_UNSIGNED) {
            value->value.unsigned_value = nd_axdr_get_u16(&reader);
        } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
            value->value.unsigned_value = nd_axdr_get_u32(&reader);
        } else {
            return -1;
        }
        return nd_axdr_reader_ok(&reader) ? 0 : -1;
    }
    if (capture->class_id == ND_IC_REGISTER && capture->attribute_id == 3) {
        if (nd_axdr_get_tag(&reader) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(&reader) != 2
            || nd_axdr_get_tag(&reader) != ND_AXDR_INTEGER) {
            return -1;
        }
        value->type = ND_CAPTURE_MEASUREMENT;
        value->value.measurement.scaler = (int8_t)nd_axdr_get_u8(&reader);
        if (nd_axdr_get_tag(&reader) != ND_AXDR_ENUM) {
            return -1;
        }
        value->value.measurement.unit = nd_axdr_get_u8(&reader);
        return nd_axdr_reader_ok(&reader) ? 0 : -1;
    }
    if (capture->class_id == ND_IC_DATA && capture->attribute_id == 2) {
        value->type = ND_CAPTURE_UNSIGNED;
        return read_unsigned(&reader, &value->value.unsigned_value);
    }
    return -1;
}

int nd_idis_capture_write(const nd_capture_object_t *capture,
                          const nd_capture_value_t *value,
                          nd_axdr_writer_t *out)
{
    if (capture == NULL || value == NULL || out == NULL) {
        return -1;
    }
    if (value->type == ND_CAPTURE_NONE) {
        nd_axdr_put_tag(out, ND_AXDR_NULL);
        return 0;
    }
    if (capture->class_id == ND_IC_CLOCK && capture->attribute_id == 2
        && value->type == ND_CAPTURE_CLOCK) {
        const nd_clock_value_t *clock = &value->value.clock;
        uint8_t weekday = clock->day_of_week;
        if (weekday == 0) {
            weekday = nd_calendar_weekday(clock->time.day, clock->time.month,
                                          clock->time.year);
        }
        nd_axdr_put_tag(out, ND_AXDR_DATE_TIME);
        nd_axdr_put_u16(out, clock->time.year);
        nd_axdr_put_u8(out, clock->time.month);
        nd_axdr_put_u8(out, clock->time.day);
        nd_axdr_put_u8(out, weekday == 0 ? 7 : weekday);
        nd_axdr_put_u8(out, clock->time.hour);
        nd_axdr_put_u8(out, clock->time.minute);
        nd_axdr_put_u8(out, clock->time.second);
        nd_axdr_put_u8(out, clock->hundredths);
        nd_axdr_put_i16(out, clock->deviation);
        nd_axdr_put_u8(out, clock->status);
        return 0;
    }
    if ((capture->class_id == ND_IC_REGISTER
         || capture->class_id == ND_IC_EXTENDED_REGISTER)
        && capture->attribute_id == 2 && value->type == ND_CAPTURE_SIGNED) {
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG);
        nd_axdr_put_u32(out, (uint32_t)(int32_t)value->value.signed_value);
        return 0;
    }
    if ((capture->class_id == ND_IC_REGISTER
         || capture->class_id == ND_IC_EXTENDED_REGISTER)
        && capture->attribute_id == 2 && value->type == ND_CAPTURE_UNSIGNED) {
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, (uint32_t)value->value.unsigned_value);
        return 0;
    }
    if (capture->class_id == ND_IC_REGISTER && capture->attribute_id == 3
        && value->type == ND_CAPTURE_MEASUREMENT) {
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)value->value.measurement.scaler);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, value->value.measurement.unit);
        return 0;
    }
    if (capture->class_id == ND_IC_DATA && capture->attribute_id == 2
        && value->type == ND_CAPTURE_UNSIGNED) {
        if (data_is_u32(&capture->ln)) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, (uint32_t)value->value.unsigned_value);
        } else {
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, (uint8_t)value->value.unsigned_value);
        }
        return 0;
    }
    if (value->type == ND_CAPTURE_BYTES) {
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, value->value.bytes.len);
        nd_axdr_put_octets(out, value->value.bytes.data,
                           value->value.bytes.len);
        return 0;
    }
    return -1;
}
