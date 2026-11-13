#include "targets/idis_ip_meter/cosem/classes/nd_ic_demand.h"

#include "domain/common/calendar.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_extended_register.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_DEMAND_MAGIC 0x44454D31u

typedef struct {
    uint32_t magic;
    nd_idis_demand_state_t items[ND_IDIS_DEMAND_MAX];
} nd_idis_demand_record_t;

static void put_datetime_octets(nd_axdr_writer_t *out,
                                const nd_datetime_t *time)
{
    uint8_t buffer[12] = {0};
    buffer[0] = (uint8_t)(time->year >> 8);
    buffer[1] = (uint8_t)time->year;
    buffer[2] = time->month;
    buffer[3] = time->day;
    buffer[5] = time->hour;
    buffer[6] = time->minute;
    buffer[7] = time->second;
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, sizeof(buffer));
    nd_axdr_put_octets(out, buffer, sizeof(buffer));
}

static int demand_index(uint8_t group)
{
    switch (group) {
    case 1:
    case 2:
    case 3:
    case 4:
        return group - 1;
    case 9:
        return 4;
    case 10:
        return 5;
    default:
        return -1;
    }
}

static uint8_t demand_group(uint8_t index)
{
    static const uint8_t groups[ND_IDIS_DEMAND_MAX] = {1, 2, 3, 4, 9, 10};
    return index < ND_IDIS_DEMAND_MAX ? groups[index] : 0;
}

static nd_idis_demand_state_t *demand_state(nd_idis_objects_t *objects,
                                            const nd_obis_t *ln)
{
    if (objects == NULL || ln == NULL || ln->a != 1 || ln->b != 0
        || ln->e != 0 || ln->f != 255) {
        return NULL;
    }
    if (ln->d == 24 && ln->c == 1) {
        return &objects->demand[0]; /* average import power alias */
    }
    if (ln->d != 4) {
        return NULL;
    }
    int index = demand_index(ln->c);
    return index < 0 ? NULL : &objects->demand[index];
}

static uint8_t power_wire_unit(nd_measurement_unit_t unit)
{
    switch (unit) {
    case ND_MEASUREMENT_UNIT_WATT:
        return 27;
    case ND_MEASUREMENT_UNIT_VAR:
        return 31;
    case ND_MEASUREMENT_UNIT_VA:
        return 32;
    case ND_MEASUREMENT_UNIT_AMPERE:
        return 33;
    case ND_MEASUREMENT_UNIT_VOLT:
        return 35;
    default:
        return 0;
    }
}

static int average_power_get(nd_idis_objects_t *objects, const nd_obis_t *ln,
                             uint8_t attribute_id, nd_axdr_writer_t *out)
{
    if (objects->reg.measurement == NULL
        || objects->reg.measurement->read == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_measure_quantity_t quantity = ln->c == 15
        ? ND_MEASURE_TOTAL_ACTIVE_POWER
        : ND_MEASURE_ACTIVE_POWER_NET;
    nd_measurement_value_t value;
    if (objects->reg.measurement->read(objects->reg.measurement->ctx, quantity,
                                       0, &value) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_idis_average_power_t *last =
        &objects->average_power[ln->c == 15 ? 0 : 1];
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, (uint32_t)value.value);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, last->value);
        break;
    case 4:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)value.scaler);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, power_wire_unit(value.unit));
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, last->status);
        break;
    case 6:
    case 7: {
        uint8_t time[12] = {0};
        if (attribute_id == 6 && last->capture_time.year != 0) {
            time[0] = (uint8_t)(last->capture_time.year >> 8);
            time[1] = (uint8_t)last->capture_time.year;
            time[2] = last->capture_time.month;
            time[3] = last->capture_time.day;
            time[5] = last->capture_time.hour;
            time[6] = last->capture_time.minute;
            time[7] = last->capture_time.second;
        }
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, sizeof(time));
        nd_axdr_put_octets(out, time, sizeof(time));
        break;
    }
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, 0);
        break;
    case 9:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, 0);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_demand_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id,
                     const nd_axdr_reader_t *selector,
                     nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)selector;
    if (ln != NULL && ln->a == 1 && ln->b == 0 && ln->d == 24
        && (ln->c == 15 || ln->c == 16)) {
        return out != NULL ? average_power_get(objects, ln, attribute_id, out)
                           : ND_ERR_OBJECT_UNDEFINED;
    }
    nd_idis_demand_state_t *state = demand_state(objects, ln);
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
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->current.current_value);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->history.last_value);
        break;
    case 4:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, 0);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, 27); /* W */
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, state->history.status);
        break;
    case 6:
        put_datetime_octets(out, &state->history.last_capture_time);
        break;
    case 7:
        put_datetime_octets(out, &state->history.period_start);
        break;
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->current.period_seconds);
        break;
    case 9:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->current.periods);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t active_rate(const nd_idis_objects_t *objects)
{
    const nd_register_activation_state_t *energy = &objects->activation.energy;
    if (energy->length == 1 && energy->mask[0] >= '1'
        && energy->mask[0] <= '4') {
        return (uint8_t)(energy->mask[0] - '0');
    }
    return 0; /* unknown mask: update the total register only */
}

static void read_now(nd_idis_objects_t *objects, nd_datetime_t *now)
{
    *now = (nd_datetime_t){0};
    if (objects->clock.port != NULL && objects->clock.port->read != NULL) {
        nd_dlms_datetime_t value;
        if (objects->clock.port->read(objects->clock.port->ctx, &value) == 0) {
            now->year = value.year;
            now->month = value.month;
            now->day = value.day;
            now->hour = value.hour;
            now->minute = value.minute;
            now->second = value.second;
        }
    }
}

void nd_idis_demand_accumulate(nd_idis_demand_acc_t *acc, int32_t power)
{
    if (acc == NULL) {
        return;
    }
    acc->sum += power > 0 ? (uint64_t)power : 0;
    acc->samples++;
}

int nd_ic_demand_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                        uint8_t method_id, nd_axdr_reader_t *in,
                        nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    if (ln != NULL && ln->a == 1 && ln->b == 0 && ln->d == 24
        && (ln->c == 15 || ln->c == 16)) {
        nd_idis_average_power_t *last =
            &objects->average_power[ln->c == 15 ? 0 : 1];
        if (method_id == 1) {
            *last = (nd_idis_average_power_t){0};
            return ND_OK;
        }
        if (method_id == 2) {
            if (objects->reg.measurement == NULL
                || objects->reg.measurement->read == NULL) {
                return ND_ERR_OPERATION_NOT_POSSIBLE;
            }
            nd_measure_quantity_t quantity = ln->c == 15
                ? ND_MEASURE_TOTAL_ACTIVE_POWER
                : ND_MEASURE_ACTIVE_POWER_NET;
            nd_measurement_value_t value;
            if (objects->reg.measurement->read(
                    objects->reg.measurement->ctx, quantity, 0,
                    &value) != 0) {
                return ND_ERR_OPERATION_NOT_POSSIBLE;
            }
            last->value = (uint32_t)value.value;
            last->status = value.status;
            read_now(objects, &last->capture_time);
            return ND_OK;
        }
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_idis_demand_state_t *state = demand_state(objects, ln);
    if (state == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id == 1) {
        state->current = (nd_demand_snapshot_t){0};
        nd_demand_history_reset(&state->history);
        return ND_OK;
    }
    if (method_id == 2) {
        uint8_t acc_index = (uint8_t)(state - objects->demand);
        nd_idis_demand_acc_t *acc = &objects->demand_acc[acc_index];
        if (acc->samples > 0) {
            state->current.current_value =
                (uint32_t)(acc->sum / acc->samples);
            acc->sum = 0;
            acc->samples = 0;
        }
        nd_datetime_t capture;
        read_now(objects, &capture);
        if (capture.year == 0) {
            capture = state->history.last_capture_time;
        }
        nd_datetime_t start = state->history.period_start;
        if (nd_demand_history_capture(&state->history, &state->current,
                                      &capture, &start, 0) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        uint8_t group =
            demand_group((uint8_t)(state - objects->demand));
        uint8_t rate = active_rate(objects);
        uint32_t value = state->history.last_value;
        uint8_t status = state->history.status;
        (void)nd_idis_extended_update(objects->max_demand, group, 0, value,
                                      status, &capture);
        if (rate != 0) {
            (void)nd_idis_extended_update(objects->max_demand, group, rate,
                                          value, status, &capture);
        }
        state->current.current_value = 0;
        state->current.current_fee_value = 0;
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t demand_unit(uint8_t group)
{
    if (group == 3 || group == 4) {
        return 31; /* var */
    }
    if (group == 9 || group == 10) {
        return 32; /* VA */
    }
    return 27; /* W */
}

int nd_ic_demand_last_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)selector;
    if (objects == NULL || ln == NULL || ln->a != 1 || ln->b != 0
        || ln->d != 5 || ln->e != 0 || ln->f != 255 || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    int index = demand_index(ln->c);
    if (index < 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_idis_demand_state_t *state = &objects->demand[index];
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->history.last_value);
        break;
    case 3:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, 0);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, demand_unit((uint8_t)ln->c));
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, state->history.status);
        break;
    case 5:
        put_datetime_octets(out, &state->history.last_capture_time);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_demand_last_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    if (objects == NULL || ln == NULL || ln->a != 1 || ln->b != 0
        || ln->d != 5 || ln->e != 0 || ln->f != 255) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    int index = demand_index(ln->c);
    if (index < 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    objects->demand[index].history.last_value = 0;
    objects->demand[index].history.status = 0;
    return ND_OK;
}

int nd_ic_demand_save(const nd_idis_demand_state_t *states, nd_store_t *store,
                      uint32_t offset)
{
    if (states == NULL || store == NULL) {
        return -1;
    }
    nd_idis_demand_record_t record = {.magic = ND_IDIS_DEMAND_MAGIC};
    for (uint8_t index = 0; index < ND_IDIS_DEMAND_MAX; index++) {
        record.items[index] = states[index];
    }
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_ic_demand_load(nd_idis_demand_state_t *states, nd_store_t *store,
                      uint32_t offset)
{
    if (states == NULL || store == NULL) {
        return -1;
    }
    nd_idis_demand_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_DEMAND_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_DEMAND_MAX; index++) {
        states[index] = record.items[index];
    }
    return 0;
}
