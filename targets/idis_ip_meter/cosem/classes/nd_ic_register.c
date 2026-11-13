#include "targets/idis_ip_meter/cosem/classes/nd_ic_register.h"

#include "targets/idis_ip_meter/mapper_idis/nd_idis_energy.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

/* Parameter registers backed by target configuration. */
typedef enum {
    PARAM_NONE = 0,
    PARAM_CLOCK_SHIFT,
    PARAM_PQ_SAG,
    PARAM_PQ_SWELL,
    PARAM_PQ_CUT,
    PARAM_PQ_SAG_TIME,
    PARAM_PQ_SWELL_TIME,
    PARAM_PQ_CUT_TIME,
    PARAM_POWER_DURATION,
    PARAM_POWER_THRESHOLD,
    PARAM_MEASUREMENT_PERIOD
} param_kind_t;

static param_kind_t param_kind_for(const nd_obis_t *ln)
{
    if (nd_obis_eq(ln, &ND_OBIS_CLOCK_SHIFT_LIMIT)) {
        return PARAM_CLOCK_SHIFT;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_THRESHOLD)) {
        return PARAM_PQ_SAG;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_THRESHOLD)) {
        return PARAM_PQ_SWELL;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_CUT_THRESHOLD)) {
        return PARAM_PQ_CUT;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_TIME)) {
        return PARAM_PQ_SAG_TIME;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_TIME)) {
        return PARAM_PQ_SWELL_TIME;
    }
    if (nd_obis_eq(ln, &ND_OBIS_PQ_CUT_TIME)) {
        return PARAM_PQ_CUT_TIME;
    }
    if (nd_obis_eq(ln, &ND_OBIS_POWER_FAILURE_DURATION)) {
        return PARAM_POWER_DURATION;
    }
    if (nd_obis_eq(ln, &ND_OBIS_POWER_FAILURE_THRESHOLD)) {
        return PARAM_POWER_THRESHOLD;
    }
    if (nd_obis_eq(ln, &ND_OBIS_MEASUREMENT_PERIOD_3)) {
        return PARAM_MEASUREMENT_PERIOD;
    }
    return PARAM_NONE;
}

static int read_unsigned_value(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        *value = nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int32_t time_value(uint16_t specific, uint16_t fallback)
{
    return specific != 0 ? specific : fallback;
}

static int32_t param_value(const nd_idis_register_state_t *state,
                           param_kind_t kind, int8_t *scaler, uint8_t *unit)
{
    *scaler = -1;
    *unit = 35; /* V, in 0.1 V */
    switch (kind) {
    case PARAM_CLOCK_SHIFT:
        *scaler = 0;
        *unit = 7; /* seconds */
        return state->clock != NULL ? state->clock->time_shift_limit : 0;
    case PARAM_PQ_SAG:
        return state->pq != NULL ? state->pq->config.voltage_sag : 0;
    case PARAM_PQ_SWELL:
        return state->pq != NULL ? state->pq->config.voltage_swell : 0;
    case PARAM_PQ_CUT:
        return state->pq != NULL ? state->pq->config.voltage_cut : 0;
    case PARAM_PQ_SAG_TIME:
        *scaler = 0;
        *unit = 7;
        return state->pq != NULL
                   ? time_value(state->pq->config.sag_debounce_seconds,
                                state->pq->config.debounce_seconds)
                   : 0;
    case PARAM_PQ_SWELL_TIME:
        *scaler = 0;
        *unit = 7;
        return state->pq != NULL
                   ? time_value(state->pq->config.swell_debounce_seconds,
                                state->pq->config.debounce_seconds)
                   : 0;
    case PARAM_PQ_CUT_TIME:
        *scaler = 0;
        *unit = 7;
        return state->pq != NULL
                   ? time_value(state->pq->config.cut_debounce_seconds,
                                state->pq->config.debounce_seconds)
                   : 0;
    case PARAM_POWER_DURATION:
        *scaler = 0;
        *unit = 7;
        return state->power != NULL ? (int32_t)state->power->last_duration : 0;
    case PARAM_POWER_THRESHOLD:
        *scaler = 0;
        *unit = 7;
        return state->power != NULL ? state->power->threshold_seconds : 0;
    case PARAM_MEASUREMENT_PERIOD:
        *scaler = 0;
        *unit = 7;
        return (int32_t)state->measurement_period_3;
    default:
        return 0;
    }
}

static int param_reset(nd_idis_register_state_t *state, param_kind_t kind)
{
    switch (kind) {
    case PARAM_CLOCK_SHIFT:
        if (state->clock == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        state->clock->time_shift_limit = 60;
        return ND_OK;
    case PARAM_PQ_SAG:
    case PARAM_PQ_SWELL:
    case PARAM_PQ_CUT:
    case PARAM_PQ_SAG_TIME:
    case PARAM_PQ_SWELL_TIME:
    case PARAM_PQ_CUT_TIME:
        if (state->pq == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        if (kind == PARAM_PQ_SAG) {
            state->pq->config.voltage_sag = 0;
        } else if (kind == PARAM_PQ_SWELL) {
            state->pq->config.voltage_swell = 0;
        } else if (kind == PARAM_PQ_CUT) {
            state->pq->config.voltage_cut = 0;
        } else if (kind == PARAM_PQ_SAG_TIME) {
            state->pq->config.sag_debounce_seconds = 0;
        } else if (kind == PARAM_PQ_SWELL_TIME) {
            state->pq->config.swell_debounce_seconds = 0;
        } else {
            state->pq->config.cut_debounce_seconds = 0;
        }
        return ND_OK;
    case PARAM_POWER_DURATION:
        if (state->power == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        state->power->last_duration = 0;
        return ND_OK;
    case PARAM_POWER_THRESHOLD:
        if (state->power == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        state->power->threshold_seconds = 0;
        return ND_OK;
    case PARAM_MEASUREMENT_PERIOD:
        state->measurement_period_3 = 0;
        return ND_OK;
    default:
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
}

static int pq_stat_phase(const nd_obis_t *ln)
{
    for (uint8_t phase = 0; phase < 3; phase++) {
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_MAGNITUDE[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_MAGNITUDE[phase])) {
            return (int)phase;
        }
    }
    return -1;
}

static int pq_stat_for(const nd_obis_t *ln, const nd_pq_state_t *pq,
                       int32_t *value, int8_t *scaler, uint8_t *unit,
                       int *has_unit)
{
    for (uint8_t phase = 0; phase < 3; phase++) {
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_COUNT[phase])) {
            *value = pq->sag_phase_count[phase];
            *has_unit = 0;
            return 0;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_DURATION[phase])) {
            *value = (int32_t)pq->sag_phase_duration[phase];
            *scaler = 0;
            *unit = 7;
            *has_unit = 1;
            return 0;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_MAGNITUDE[phase])) {
            *value = pq->sag_phase_magnitude[phase];
            *scaler = -1;
            *unit = 35;
            *has_unit = 1;
            return 0;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_COUNT[phase])) {
            *value = pq->swell_phase_count[phase];
            *has_unit = 0;
            return 0;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_DURATION[phase])) {
            *value = (int32_t)pq->swell_phase_duration[phase];
            *scaler = 0;
            *unit = 7;
            *has_unit = 1;
            return 0;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_MAGNITUDE[phase])) {
            *value = pq->swell_phase_magnitude[phase];
            *scaler = -1;
            *unit = 35;
            *has_unit = 1;
            return 0;
        }
    }
    return -1;
}

static int put_logical_name(nd_axdr_writer_t *out, const nd_obis_t *ln)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    return nd_axdr_writer_ok(out) ? ND_OK : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int get_param(nd_idis_register_state_t *state, param_kind_t kind,
                     uint8_t attribute_id, nd_axdr_writer_t *out)
{
    if ((kind == PARAM_CLOCK_SHIFT && state->clock == NULL)
        || ((kind == PARAM_POWER_DURATION || kind == PARAM_POWER_THRESHOLD)
            && state->power == NULL)
        || ((kind == PARAM_PQ_SAG || kind == PARAM_PQ_SWELL
             || kind == PARAM_PQ_CUT || kind == PARAM_PQ_SAG_TIME
             || kind == PARAM_PQ_SWELL_TIME || kind == PARAM_PQ_CUT_TIME)
            && state->pq == NULL)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    int8_t scaler;
    uint8_t unit;
    int32_t value = param_value(state, kind, &scaler, &unit);
    if (attribute_id == 2) {
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, (uint16_t)value);
    } else if (attribute_id == 3) {
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)scaler);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, unit);
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t wire_unit(nd_measurement_unit_t unit)
{
    switch (unit) {
    case ND_MEASUREMENT_UNIT_VOLT:
        return 35;
    case ND_MEASUREMENT_UNIT_AMPERE:
        return 33;
    case ND_MEASUREMENT_UNIT_WATT:
        return 27;
    case ND_MEASUREMENT_UNIT_VAR:
        return 29;
    case ND_MEASUREMENT_UNIT_VA:
        return 28;
    case ND_MEASUREMENT_UNIT_HERTZ:
        return 44;
    default:
        return 0xFF;
    }
}

static int average_channel_for(const nd_obis_t *ln)
{
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_VOLTAGE_L1)) {
        return 0;
    }
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_VOLTAGE_L2)) {
        return 1;
    }
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_VOLTAGE_L3)) {
        return 2;
    }
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_CURRENT_L1)) {
        return 3;
    }
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_CURRENT_L2)) {
        return 4;
    }
    if (nd_obis_eq(ln, &ND_OBIS_AVERAGE_CURRENT_L3)) {
        return 5;
    }
    return -1;
}

static int measurement_for(const nd_obis_t *ln, nd_measure_quantity_t *quantity,
                           uint8_t *phase)
{
    if (nd_obis_eq(ln, &ND_OBIS_VOLTAGE_L1)) {
        *quantity = ND_MEASURE_VOLTAGE;
        *phase = 1;
    } else if (nd_obis_eq(ln, &ND_OBIS_VOLTAGE_L2)) {
        *quantity = ND_MEASURE_VOLTAGE;
        *phase = 2;
    } else if (nd_obis_eq(ln, &ND_OBIS_VOLTAGE_L3)) {
        *quantity = ND_MEASURE_VOLTAGE;
        *phase = 3;
    } else if (nd_obis_eq(ln, &ND_OBIS_CURRENT_L1)) {
        *quantity = ND_MEASURE_CURRENT;
        *phase = 1;
    } else if (nd_obis_eq(ln, &ND_OBIS_CURRENT_L2)) {
        *quantity = ND_MEASURE_CURRENT;
        *phase = 2;
    } else if (nd_obis_eq(ln, &ND_OBIS_CURRENT_L3)) {
        *quantity = ND_MEASURE_CURRENT;
        *phase = 3;
    } else if (nd_obis_eq(ln, &ND_OBIS_ACTIVE_POWER_IMPORT)) {
        *quantity = ND_MEASURE_ACTIVE_POWER;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_ACTIVE_POWER_EXPORT)) {
        *quantity = ND_MEASURE_ACTIVE_POWER_EXPORT;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_REACTIVE_POWER_IMPORT)) {
        *quantity = ND_MEASURE_REACTIVE_POWER_IMPORT;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_REACTIVE_POWER_EXPORT)) {
        *quantity = ND_MEASURE_REACTIVE_POWER_EXPORT;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_APPARENT_POWER_IMPORT)) {
        *quantity = ND_MEASURE_APPARENT_POWER_IMPORT;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_APPARENT_POWER_EXPORT)) {
        *quantity = ND_MEASURE_APPARENT_POWER_EXPORT;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_ACTIVE_POWER_COMBINED)) {
        *quantity = ND_MEASURE_TOTAL_ACTIVE_POWER;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_ACTIVE_POWER_NET)) {
        *quantity = ND_MEASURE_ACTIVE_POWER_NET;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_CURRENT_SUM)) {
        *quantity = ND_MEASURE_CURRENT_SUM;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_MIN)) {
        *quantity = ND_MEASURE_POWER_FACTOR_MIN;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_AVG)) {
        *quantity = ND_MEASURE_POWER_FACTOR_AVG;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR)) {
        *quantity = ND_MEASURE_POWER_FACTOR;
        *phase = 0;
    } else if (nd_obis_eq(ln, &ND_OBIS_FREQUENCY)) {
        *quantity = ND_MEASURE_FREQUENCY;
        *phase = 0;
    } else {
        return -1;
    }
    return 0;
}

int nd_ic_pq_stat_matches(const nd_obis_t *ln)
{
    if (ln == NULL) {
        return 0;
    }
    for (uint8_t phase = 0; phase < 3; phase++) {
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_MAGNITUDE[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_MAGNITUDE[phase])) {
            return 1;
        }
    }
    return 0;
}

int nd_ic_register_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id,
                       const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out)
{
    nd_idis_register_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    param_kind_t param = param_kind_for(ln);
    if (param != PARAM_NONE) {
        if (attribute_id == 1) {
            return put_logical_name(out, ln);
        }
        return get_param(state, param, attribute_id, out);
    }
    if (state->pq != NULL) {
        int pq_phase = pq_stat_phase(ln);
        if (state->phases != 0 && pq_phase >= state->phases) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        int32_t value = 0;
        int8_t scaler = 0;
        uint8_t unit = 0;
        int has_unit = 0;
        if (pq_stat_for(ln, state->pq, &value, &scaler, &unit, &has_unit)
            == 0) {
            if (attribute_id == 1) {
                return put_logical_name(out, ln);
            }
            if (attribute_id == 2) {
                nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
                nd_axdr_put_u16(out, (uint16_t)value);
            } else if (attribute_id == 3 && has_unit) {
                nd_axdr_put_structure_header(out, 2);
                nd_axdr_put_tag(out, ND_AXDR_INTEGER);
                nd_axdr_put_u8(out, (uint8_t)scaler);
                nd_axdr_put_tag(out, ND_AXDR_ENUM);
                nd_axdr_put_u8(out, unit);
            } else {
                return ND_ERR_ATTRIBUTE_UNDEFINED;
            }
            return nd_axdr_writer_ok(out) ? ND_OK
                                          : ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    }
    int average_channel = average_channel_for(ln);
    if (average_channel >= 0) {
        uint8_t average_phase = average_channel < 3
            ? (uint8_t)(average_channel + 1)
            : (uint8_t)(average_channel - 2);
        if (state->phases != 0 && average_phase > state->phases) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        if (attribute_id == 1) {
            return put_logical_name(out, ln);
        }
        if (state->average == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        nd_measurement_value_t value;
        if (nd_idis_average_get(state->average, (uint8_t)average_channel,
                                &value) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, (uint32_t)value.value);
        } else if (attribute_id == 3) {
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_INTEGER);
            nd_axdr_put_u8(out, (uint8_t)value.scaler);
            nd_axdr_put_tag(out, ND_AXDR_ENUM);
            nd_axdr_put_u8(out, wire_unit(value.unit));
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_idis_energy_map_t map;
    if (nd_idis_energy_map(ln, &map) == 0) {
        if (attribute_id == 1) {
            return put_logical_name(out, ln);
        }
        if (state->energy == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        int64_t value = nd_energy_set_read_raw(state->energy, map.first,
                                               map.tariff);
        if (map.combine != 0) {
            int64_t second = nd_energy_set_read_raw(state->energy, map.second,
                                                    map.tariff);
            value += map.combine * second;
        }
        if (map.divisor != 0) {
            value /= map.divisor;
        }
        if (attribute_id == 2) {
            if (map.signed_value) {
                nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG);
                nd_axdr_put_u32(out, (uint32_t)(int32_t)value);
            } else {
                nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
                nd_axdr_put_u32(out, (uint32_t)value);
            }
        } else if (attribute_id == 3) {
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_INTEGER);
            nd_axdr_put_u8(out, 0);
            nd_axdr_put_tag(out, ND_AXDR_ENUM);
            nd_axdr_put_u8(out, map.unit);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (state->measurement == NULL || state->measurement->read == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_measure_quantity_t quantity;
    uint8_t phase;
    if (measurement_for(ln, &quantity, &phase) != 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (state->phases != 0 && phase > state->phases) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        return put_logical_name(out, ln);
    }
    if (state->measurement == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_measurement_value_t value;
    if (state->measurement->read(state->measurement->ctx, quantity, phase,
                                 &value) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    switch (attribute_id) {
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, (uint32_t)value.value);
        break;
    case 3:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, (uint8_t)value.scaler);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, wire_unit(value.unit));
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, value.status);
        break;
    case 5: {
        uint8_t time[12] = {0};
        nd_dlms_datetime_t now;
        if (state->clock != NULL && state->clock->port != NULL
            && state->clock->port->read != NULL
            && state->clock->port->read(state->clock->port->ctx, &now) == 0) {
            time[0] = (uint8_t)(now.year >> 8);
            time[1] = (uint8_t)now.year;
            time[2] = now.month;
            time[3] = now.day;
            time[5] = now.hour;
            time[6] = now.minute;
            time[7] = now.second;
        }
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, sizeof(time));
        nd_axdr_put_octets(out, time, sizeof(time));
        break;
    }
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_register_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_register_state_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    param_kind_t kind = param_kind_for(ln);
    if (kind == PARAM_NONE) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id != 2) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    uint32_t value;
    if (read_unsigned_value(in, &value) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (kind == PARAM_CLOCK_SHIFT) {
        if (state->clock == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        if (value > 3600) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->clock->time_shift_limit = (uint16_t)value;
        return ND_OK;
    }
    if (kind == PARAM_MEASUREMENT_PERIOD) {
        state->measurement_period_3 = value;
        return ND_OK;
    }
    if (kind == PARAM_POWER_THRESHOLD) {
        if (state->power == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        state->power->threshold_seconds = (uint16_t)value;
        return ND_OK;
    }
    if (kind == PARAM_POWER_DURATION) {
        return ND_ERR_ACCESS_DENIED;
    }
    if (state->pq == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (value > 65535) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (kind == PARAM_PQ_SAG) {
        state->pq->config.voltage_sag = (int32_t)value;
    } else if (kind == PARAM_PQ_SWELL) {
        state->pq->config.voltage_swell = (int32_t)value;
    } else if (kind == PARAM_PQ_CUT) {
        state->pq->config.voltage_cut = (int32_t)value;
    } else {
        if (value > 3600) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (kind == PARAM_PQ_SAG_TIME) {
            state->pq->config.sag_debounce_seconds = (uint16_t)value;
        } else if (kind == PARAM_PQ_SWELL_TIME) {
            state->pq->config.swell_debounce_seconds = (uint16_t)value;
        } else {
            state->pq->config.cut_debounce_seconds = (uint16_t)value;
        }
    }
    return ND_OK;
}

static void reset_pq_phase(nd_pq_state_t *pq, const nd_obis_t *ln)
{
    for (uint8_t phase = 0; phase < 3; phase++) {
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SAG_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SAG_MAGNITUDE[phase])) {
            pq->sag_phase_count[phase] = 0;
            pq->sag_phase_duration[phase] = 0;
            pq->sag_phase_magnitude[phase] = 0;
            pq->sag_phase_timer[phase] = 0;
            pq->sag_phase_active[phase] = 0;
            return;
        }
        if (nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_COUNT[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_DURATION[phase])
            || nd_obis_eq(ln, &ND_OBIS_PQ_SWELL_MAGNITUDE[phase])) {
            pq->swell_phase_count[phase] = 0;
            pq->swell_phase_duration[phase] = 0;
            pq->swell_phase_magnitude[phase] = 0;
            pq->swell_phase_timer[phase] = 0;
            pq->swell_phase_active[phase] = 0;
            return;
        }
    }
}

int nd_ic_register_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out)
{
    nd_idis_register_state_t *state = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    if (state == NULL || ln == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    param_kind_t param = param_kind_for(ln);
    if (param != PARAM_NONE) {
        return param_reset(state, param);
    }
    if (state->pq != NULL) {
        int pq_phase = pq_stat_phase(ln);
        if (state->phases != 0 && pq_phase >= state->phases) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        int32_t value = 0;
        int8_t scaler = 0;
        uint8_t unit = 0;
        int has_unit = 0;
        if (pq_stat_for(ln, state->pq, &value, &scaler, &unit, &has_unit) == 0) {
            reset_pq_phase(state->pq, ln);
            return ND_OK;
        }
    }
    int average_channel = average_channel_for(ln);
    if (average_channel >= 0) {
        uint8_t average_phase = average_channel < 3
            ? (uint8_t)(average_channel + 1)
            : (uint8_t)(average_channel - 2);
        if (state->phases != 0 && average_phase > state->phases) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        return ND_OK; /* instantaneous average has no accumulator */
    }
    nd_measure_quantity_t quantity;
    uint8_t phase;
    if (measurement_for(ln, &quantity, &phase) == 0) {
        if (state->phases != 0 && phase > state->phases) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        return ND_OK; /* instantaneous value has no accumulator */
    }
    nd_idis_energy_map_t map;
    if (nd_idis_energy_map(ln, &map) == 0) {
        if (state->energy == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        nd_energy_set_reset(state->energy, map.first);
        if (map.combine != 0) {
            nd_energy_set_reset(state->energy, map.second);
        }
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}
