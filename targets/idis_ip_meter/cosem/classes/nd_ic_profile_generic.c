#include "targets/idis_ip_meter/cosem/classes/nd_ic_profile_generic.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include <string.h>

#define PROFILE_SELECT_MAX 16
/* Range values are copied whole; 16 bytes covers clock and numeric columns. */
#define PROFILE_VALUE_MAX 16

typedef struct {
    uint8_t kind; /* 0 all, 1 range, 2 entry */
    uint16_t restricting_col;
    uint32_t from_entry;
    uint32_t to_entry;
    uint16_t from_col;
    uint16_t to_col;
    uint8_t from_value[PROFILE_VALUE_MAX];
    uint8_t to_value[PROFILE_VALUE_MAX];
    uint8_t from_length;
    uint8_t to_length;
    uint8_t cols[PROFILE_SELECT_MAX];
    uint8_t col_count;
} profile_selection_t;

static void put_capture_object(nd_axdr_writer_t *out,
                               const nd_capture_object_t *capture)
{
    nd_axdr_put_structure_header(out, 4);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, capture->class_id);
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)&capture->ln, 6);
    nd_axdr_put_tag(out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(out, capture->attribute_id);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, capture->data_index);
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    switch (tag) {
    case ND_AXDR_UNSIGNED:
    case ND_AXDR_INTEGER:
    case ND_AXDR_ENUM:
        *value = nd_axdr_get_u8(in);
        break;
    case ND_AXDR_LONG_UNSIGNED:
    case ND_AXDR_LONG:
        *value = nd_axdr_get_u16(in);
        break;
    case ND_AXDR_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_DOUBLE_LONG:
        *value = nd_axdr_get_u32(in);
        break;
    default:
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_capture_object(nd_axdr_reader_t *in,
                               nd_capture_object_t *capture)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 4
        || nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
        return -1;
    }
    capture->class_id = nd_axdr_get_u16(in);
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 6) {
        return -1;
    }
    nd_axdr_get_octets(in, (uint8_t *)&capture->ln, 6);
    if (nd_axdr_get_tag(in) != ND_AXDR_INTEGER) {
        return -1;
    }
    capture->attribute_id = nd_axdr_get_u8(in);
    if (nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
        return -1;
    }
    capture->data_index = nd_axdr_get_u16(in);
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_value_slice(nd_axdr_reader_t *in, uint8_t *value,
                            uint8_t *length)
{
    size_t start = in->pos;
    if (nd_axdr_skip_value(in) != 0) {
        return -1;
    }
    size_t size = in->pos - start;
    if (size == 0 || size > PROFILE_VALUE_MAX) {
        return -1;
    }
    memcpy(value, in->buf + start, size);
    *length = (uint8_t)size;
    return 0;
}

static int find_capture(const nd_idis_profile_state_t *profile,
                        const nd_capture_object_t *capture)
{
    for (uint8_t index = 0; index < profile->capture_count; index++) {
        if (profile->captures[index].class_id == capture->class_id
            && profile->captures[index].attribute_id == capture->attribute_id
            && profile->captures[index].data_index == capture->data_index
            && nd_obis_eq(&profile->captures[index].ln, &capture->ln)) {
            return index;
        }
    }
    return -1;
}

static int parse_range(nd_axdr_reader_t *in,
                       const nd_idis_profile_state_t *profile,
                       profile_selection_t *selection)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 4) {
        return -1;
    }
    nd_capture_object_t restricting;
    if (read_capture_object(in, &restricting) != 0) {
        return -1;
    }
    int col = find_capture(profile, &restricting);
    if (col < 0) {
        return -1;
    }
    selection->restricting_col = (uint16_t)col;
    if (read_value_slice(in, selection->from_value,
                         &selection->from_length) != 0
        || read_value_slice(in, selection->to_value,
                            &selection->to_length) != 0) {
        return -1;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > profile->capture_count || count > PROFILE_SELECT_MAX) {
        return -1;
    }
    if (count == 0) {
        for (uint8_t index = 0; index < profile->capture_count; index++) {
            selection->cols[index] = index;
        }
        selection->col_count = profile->capture_count;
    } else {
        for (uint16_t index = 0; index < count; index++) {
            nd_capture_object_t capture;
            if (read_capture_object(in, &capture) != 0) {
                return -1;
            }
            int found = find_capture(profile, &capture);
            if (found < 0) {
                return -1;
            }
            selection->cols[selection->col_count++] = (uint8_t)found;
        }
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int parse_entry(nd_axdr_reader_t *in, uint32_t entries,
                       const nd_idis_profile_state_t *profile,
                       profile_selection_t *selection)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 4) {
        return -1;
    }
    uint32_t from_col = 0;
    uint32_t to_col = 0;
    if (read_unsigned(in, &selection->from_entry) != 0
        || read_unsigned(in, &selection->to_entry) != 0
        || read_unsigned(in, &from_col) != 0
        || read_unsigned(in, &to_col) != 0) {
        return -1;
    }
    selection->from_col = (uint16_t)from_col;
    selection->to_col = (uint16_t)to_col;
    if (selection->from_entry == 0) {
        selection->from_entry = 1;
    }
    if (entries == 0) {
        selection->from_entry = 1;
        selection->to_entry = 0;
    } else if (selection->to_entry == 0 || selection->to_entry > entries) {
        selection->to_entry = entries;
    }
    if (selection->from_col == 0) {
        selection->from_col = 1;
    }
    if (selection->to_col == 0 || selection->to_col > profile->capture_count) {
        selection->to_col = profile->capture_count;
    }
    if ((entries != 0 && selection->from_entry > selection->to_entry)
        || selection->from_col > selection->to_col
        || selection->from_col > profile->capture_count) {
        return -1;
    }
    for (uint16_t col = selection->from_col; col <= selection->to_col; col++) {
        selection->cols[selection->col_count++] = (uint8_t)(col - 1);
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int parse_selection(const nd_axdr_reader_t *selector,
                           uint32_t entries,
                           const nd_idis_profile_state_t *profile,
                           profile_selection_t *selection)
{
    memset(selection, 0, sizeof(*selection));
    if (selector == NULL) {
        for (uint8_t index = 0; index < profile->capture_count; index++) {
            selection->cols[index] = index;
        }
        selection->col_count = profile->capture_count;
        return 0;
    }
    nd_axdr_reader_t in = *selector;
    if (nd_axdr_get_tag(&in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(&in) != 2) {
        return -1;
    }
    uint32_t access_selector = 0;
    if (read_unsigned(&in, &access_selector) != 0) {
        return -1;
    }
    if (access_selector == 1) {
        selection->kind = 1;
        return parse_range(&in, profile, selection);
    }
    if (access_selector == 2) {
        selection->kind = 2;
        return parse_entry(&in, entries, profile, selection);
    }
    return -1;
}

static int record_offsets(const uint8_t *record, size_t length,
                          uint8_t count, uint16_t *offsets,
                          uint16_t *lengths)
{
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, record, length);
    for (uint8_t index = 0; index < count; index++) {
        size_t start = in.pos;
        if (nd_axdr_skip_value(&in) != 0) {
            return -1;
        }
        offsets[index] = (uint16_t)start;
        lengths[index] = (uint16_t)(in.pos - start);
    }
    return 0;
}

static int numeric_value(const uint8_t *value, uint8_t length,
                         int64_t *out, int *is_signed)
{
    if (length < 2) {
        return -1;
    }
    uint8_t tag = value[0];
    uint8_t bytes = (uint8_t)(length - 1);
    uint64_t raw = 0;
    for (uint8_t index = 0; index < bytes; index++) {
        raw = (raw << 8) | value[1 + index];
    }
    switch (tag) {
    case ND_AXDR_INTEGER:
    case ND_AXDR_LONG:
    case ND_AXDR_DOUBLE_LONG:
    case ND_AXDR_LONG64:
        *is_signed = 1;
        if (bytes < 8 && (raw & ((uint64_t)1 << (bytes * 8 - 1))) != 0) {
            raw |= ~((((uint64_t)1 << (bytes * 8)) - 1));
        }
        *out = (int64_t)raw;
        return 0;
    case ND_AXDR_UNSIGNED:
    case ND_AXDR_LONG_UNSIGNED:
    case ND_AXDR_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_LONG64_UNSIGNED:
        *is_signed = 0;
        *out = (int64_t)raw;
        return 0;
    default:
        return -1;
    }
}

static int64_t datetime_seconds(const uint8_t *value)
{
    static const uint8_t month_days[12] = {31, 28, 31, 30, 31, 30,
                                           31, 31, 30, 31, 30, 31};
    uint16_t year = (uint16_t)((uint16_t)value[1] << 8 | value[2]);
    uint8_t month = value[3];
    uint8_t day = value[4];
    uint8_t hour = value[6];
    uint8_t minute = value[7];
    uint8_t second = value[8];
    int64_t days = 0;
    for (uint16_t y = 2000; y < year; y++) {
        days += ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) ? 366 : 365;
    }
    for (uint8_t m = 1; m < month && m <= 12; m++) {
        days += month_days[m - 1];
        if (m == 2) {
            days += ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
        }
    }
    days += day - 1;
    return ((days * 24 + hour) * 60 + minute) * 60 + second;
}

static int compare_value(const uint8_t *left, uint8_t left_length,
                         const uint8_t *right, uint8_t right_length,
                         int *result)
{
    if (left_length == 0 || right_length == 0 || left[0] != right[0]) {
        return -1;
    }
    if (left[0] == ND_AXDR_DATE_TIME) {
        if (left_length != 13 || right_length != 13) {
            return -1;
        }
        int64_t l = datetime_seconds(left);
        int64_t r = datetime_seconds(right);
        *result = l < r ? -1 : (l > r ? 1 : 0);
        return 0;
    }
    if (left[0] == ND_AXDR_OCTET_STRING || left[0] == ND_AXDR_VISIBLE_STRING
        || left[0] == ND_AXDR_UTF8_STRING) {
        if (left_length < 2 || right_length < 2) {
            return -1;
        }
        uint8_t llen = left[1];
        uint8_t rlen = right[1];
        if (left_length != (uint8_t)(2 + llen)
            || right_length != (uint8_t)(2 + rlen)) {
            return -1;
        }
        size_t common = llen < rlen ? llen : rlen;
        int cmp = memcmp(left + 2, right + 2, common);
        *result = cmp != 0 ? cmp : (llen < rlen ? -1 : (llen > rlen ? 1 : 0));
        return 0;
    }
    int64_t l = 0;
    int64_t r = 0;
    int ls = 0;
    int rs = 0;
    if (numeric_value(left, left_length, &l, &ls) != 0
        || numeric_value(right, right_length, &r, &rs) != 0
        || ls != rs) {
        return -1;
    }
    *result = l < r ? -1 : (l > r ? 1 : 0);
    return 0;
}

static int row_matches_range(const nd_idis_profile_state_t *profile,
                             uint32_t index,
                             const profile_selection_t *selection)
{
    uint8_t record[ND_IDIS_PROFILE_RECORD_MAX];
    size_t length = 0;
    if (nd_profile_store_read(&profile->store, index, record, sizeof(record),
                              &length) != 0) {
        return -1;
    }
    uint16_t offsets[ND_IDIS_PROFILE_MAX_CAPTURES];
    uint16_t lengths[ND_IDIS_PROFILE_MAX_CAPTURES];
    if (record_offsets(record, length, profile->capture_count, offsets,
                       lengths) != 0) {
        return -1;
    }
    const uint8_t *value = record + offsets[selection->restricting_col];
    uint16_t value_length = lengths[selection->restricting_col];
    if (value_length > 255) {
        return -1;
    }
    int lower = 0;
    int upper = 0;
    if (compare_value(value, (uint8_t)value_length, selection->from_value,
                      selection->from_length, &lower) != 0
        || compare_value(value, (uint8_t)value_length, selection->to_value,
                         selection->to_length, &upper) != 0) {
        return -1;
    }
    return lower >= 0 && upper <= 0;
}

static int count_rows(const nd_idis_profile_state_t *profile,
                      const profile_selection_t *selection)
{
    uint32_t entries = nd_profile_store_entries(&profile->store);
    if (selection->kind == 0) {
        return (int)entries;
    }
    if (selection->kind == 2) {
        if (selection->from_entry > entries) {
            return 0;
        }
        return (int)(selection->to_entry - selection->from_entry + 1);
    }
    int count = 0;
    for (uint32_t index = 0; index < entries; index++) {
        int match = row_matches_range(profile, index, selection);
        if (match < 0) {
            return -1;
        }
        count += match;
    }
    return count;
}

static int value_is_clock(const nd_idis_profile_state_t *profile, uint8_t col)
{
    const nd_capture_object_t *capture = &profile->captures[col];
    return capture->class_id == ND_IC_CLOCK && capture->attribute_id == 2;
}

static int should_compress(const nd_idis_profile_state_t *profile, uint8_t col,
                           const uint8_t *current, uint8_t current_length,
                           const uint8_t *previous, uint8_t previous_length)
{
    if (value_is_clock(profile, col)) {
        if (profile->capture_period == 0
            || current_length != 13 || previous_length != 13
            || current[0] != ND_AXDR_DATE_TIME
            || previous[0] != ND_AXDR_DATE_TIME) {
            return 0;
        }
        return datetime_seconds(current)
            == datetime_seconds(previous) + profile->capture_period;
    }
    return current_length == previous_length
        && memcmp(current, previous, current_length) == 0;
}


static int write_delta(nd_axdr_writer_t *out, const uint8_t *current,
                       uint8_t current_length, const uint8_t *previous,
                       uint8_t previous_length)
{
    int64_t current_value = 0;
    int64_t previous_value = 0;
    int current_signed = 0;
    int previous_signed = 0;
    if (numeric_value(current, current_length, &current_value,
                      &current_signed) != 0
        || numeric_value(previous, previous_length, &previous_value,
                         &previous_signed) != 0
        || current_signed != previous_signed) {
        return 0;
    }
    if (current_signed) {
        int64_t delta = current_value - previous_value;
        if (delta >= -128 && delta <= 127) {
            nd_axdr_put_tag(out, ND_AXDR_DELTA_INTEGER);
            nd_axdr_put_u8(out, (uint8_t)(int8_t)delta);
            return 1;
        }
        if (delta >= -32768 && delta <= 32767) {
            nd_axdr_put_tag(out, ND_AXDR_DELTA_LONG);
            nd_axdr_put_i16(out, (int16_t)delta);
            return 1;
        }
        if (delta >= -2147483648LL && delta <= 2147483647LL) {
            nd_axdr_put_tag(out, ND_AXDR_DELTA_DOUBLE_LONG);
            nd_axdr_put_u32(out, (uint32_t)(int32_t)delta);
            return 1;
        }
        return 0;
    }
    if (current_value < previous_value) {
        return 0;
    }
    uint64_t delta = (uint64_t)current_value - (uint64_t)previous_value;
    if (delta <= 0xFFu) {
        nd_axdr_put_tag(out, ND_AXDR_DELTA_UNSIGNED);
        nd_axdr_put_u8(out, (uint8_t)delta);
        return 1;
    }
    if (delta <= 0xFFFFu) {
        nd_axdr_put_tag(out, ND_AXDR_DELTA_LONG_UNSIGNED);
        nd_axdr_put_u16(out, (uint16_t)delta);
        return 1;
    }
    if (delta <= 0xFFFFFFFFu) {
        nd_axdr_put_tag(out, ND_AXDR_DELTA_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, (uint32_t)delta);
        return 1;
    }
    return 0;
}

static int write_row(nd_axdr_writer_t *out,
                     const nd_idis_profile_state_t *profile,
                     const profile_selection_t *selection,
                     const uint8_t *record, size_t length,
                     uint8_t previous[][PROFILE_VALUE_MAX],
                     uint8_t previous_length[],
                     int have_previous)
{
    uint16_t offsets[ND_IDIS_PROFILE_MAX_CAPTURES];
    uint16_t lengths[ND_IDIS_PROFILE_MAX_CAPTURES];
    if (record_offsets(record, length, profile->capture_count, offsets,
                       lengths) != 0) {
        return -1;
    }
    nd_axdr_put_structure_header(out, selection->col_count);
    for (uint8_t index = 0; index < selection->col_count; index++) {
        uint8_t col = selection->cols[index];
        const uint8_t *value = record + offsets[col];
        if (lengths[col] > PROFILE_VALUE_MAX) {
            return -1;
        }
        uint8_t value_length = (uint8_t)lengths[col];
        if (have_previous && profile->compressed
            && should_compress(profile, col, value, value_length,
                               previous[col], previous_length[col])) {
            nd_axdr_put_tag(out, ND_AXDR_NULL);
        } else if (have_previous && profile->delta
                   && write_delta(out, value, value_length, previous[col],
                                  previous_length[col])) {
            /* delta already written */
        } else {
            nd_axdr_put_octets(out, value, value_length);
        }
        memcpy(previous[col], value, value_length);
        previous_length[col] = value_length;
    }
    return 0;
}

static int write_rows(nd_axdr_writer_t *out,
                      const nd_idis_profile_state_t *profile,
                      const profile_selection_t *selection, int rows)
{
    uint8_t record[ND_IDIS_PROFILE_RECORD_MAX];
    uint8_t previous[ND_IDIS_PROFILE_MAX_CAPTURES][PROFILE_VALUE_MAX];
    uint8_t previous_length[ND_IDIS_PROFILE_MAX_CAPTURES] = {0};
    int have_previous = 0;
    nd_axdr_put_array_header(out, (uint16_t)rows);
    uint32_t entries = nd_profile_store_entries(&profile->store);
    for (uint32_t index = 0; index < entries; index++) {
        if (selection->kind == 2
            && (index + 1 < selection->from_entry
                || index + 1 > selection->to_entry)) {
            continue;
        }
        if (selection->kind == 1) {
            int match = row_matches_range(profile, index, selection);
            if (match < 0) {
                return -1;
            }
            if (!match) {
                continue;
            }
        }
        size_t length = 0;
        if (nd_profile_store_read(&profile->store, index, record,
                                  sizeof(record), &length) != 0) {
            return -1;
        }
        if (write_row(out, profile, selection, record, length, previous,
                      previous_length, have_previous) != 0) {
            return -1;
        }
        have_previous = 1;
    }
    return 0;
}

int nd_ic_profile_generic_get(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t attribute_id,
                              const nd_axdr_reader_t *selector,
                              nd_axdr_writer_t *out)
{
    nd_idis_profile_state_t *profile = nd_idis_profiles_find(ctx, ln);
    (void)client_sap;
    if (profile == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)&profile->ln, 6);
        break;
    case 2: {
        profile_selection_t selection;
        if (parse_selection(selector, nd_profile_store_entries(&profile->store),
                            profile, &selection) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        int rows = count_rows(profile, &selection);
        if (rows < 0 || write_rows(out, profile, &selection, rows) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        break;
    }
    case 3:
        nd_axdr_put_array_header(out, profile->capture_count);
        for (uint8_t index = 0; index < profile->capture_count; index++) {
            put_capture_object(out, &profile->captures[index]);
        }
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, profile->capture_period);
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, 1); /* FIFO */
        break;
    case 6:
        nd_axdr_put_structure_header(out, 0);
        break;
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, nd_profile_store_entries(&profile->store));
        break;
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, nd_profile_store_capacity(&profile->store));
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int read_unsigned_param(nd_axdr_reader_t *in, uint32_t *value)
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

static int write_row_at(nd_axdr_writer_t *out,
                        const nd_idis_profile_state_t *profile,
                        uint32_t index)
{
    uint8_t record[ND_IDIS_PROFILE_RECORD_MAX];
    size_t length = 0;
    if (nd_profile_store_read(&profile->store, index, record, sizeof(record),
                              &length) != 0) {
        return -1;
    }
    profile_selection_t selection = {0};
    selection.col_count = profile->capture_count;
    for (uint8_t col = 0; col < profile->capture_count; col++) {
        selection.cols[col] = col;
    }
    uint8_t previous[ND_IDIS_PROFILE_MAX_CAPTURES][PROFILE_VALUE_MAX];
    uint8_t previous_length[ND_IDIS_PROFILE_MAX_CAPTURES] = {0};
    return write_row(out, profile, &selection, record, length, previous,
                     previous_length, 0);
}

int nd_ic_profile_generic_action(void *ctx, uint8_t client_sap,
                                 const nd_obis_t *ln, uint8_t method_id,
                                 nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_profile_state_t *profile = nd_idis_profiles_find(ctx, ln);
    (void)client_sap;
    (void)in;
    (void)out;
    if (profile == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (method_id) {
    case 1:
        nd_profile_store_reset(&profile->store);
        return ND_OK;
    case 3: { /* get_buffer_by_range(from, to) */
        uint32_t from = 0;
        uint32_t to = 0;
        if (read_unsigned_param(in, &from) != 0
            || read_unsigned_param(in, &to) != 0 || from == 0 || to < from) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint32_t entries = nd_profile_store_entries(&profile->store);
        if (to > entries) {
            to = entries;
        }
        if (from > to) {
            nd_axdr_put_array_header(out, 0);
            return ND_OK;
        }
        nd_axdr_put_array_header(out, (uint16_t)(to - from + 1));
        for (uint32_t index = from; index <= to; index++) {
            if (write_row_at(out, profile, index - 1) != 0) {
                return ND_ERR_OPERATION_NOT_POSSIBLE;
            }
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    case 4: { /* get_buffer_by_index(index) */
        uint32_t index = 0;
        if (read_unsigned_param(in, &index) != 0 || index == 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (index > nd_profile_store_entries(&profile->store)) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        nd_axdr_put_array_header(out, 1);
        if (write_row_at(out, profile, index - 1) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    case 2: {
        if (profile->port == NULL || profile->clock == NULL
            || profile->clock->read == NULL) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        nd_dlms_datetime_t now;
        if (profile->clock->read(profile->clock->ctx, &now) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        nd_clock_value_t clock = {
            .time = {
                .second = now.second,
                .minute = now.minute,
                .hour = now.hour,
                .day = now.day,
                .month = now.month,
                .year = now.year,
            },
            .day_of_week = now.day_of_week,
            .deviation = now.deviation,
            .status = now.status,
        };
        return nd_idis_profile_capture(profile, profile->port, client_sap,
                                       &clock);
    }
    default:
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
}
