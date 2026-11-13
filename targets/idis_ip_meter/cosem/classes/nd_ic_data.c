#include "targets/idis_ip_meter/cosem/classes/nd_ic_data.h"

#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static void put_logical_name(nd_axdr_writer_t *out, const nd_obis_t *ln)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
}

static int product_device_index(const nd_obis_t *ln)
{
    for (uint8_t index = 0; index < 6; index++) {
        if (nd_obis_eq(ln, &ND_OBIS_DEVICE_IDS[index])) {
            return index;
        }
    }
    return -1;
}

static int mbus_device_index(const nd_obis_t *ln, uint8_t *channel,
                             uint8_t *index)
{
    for (uint8_t ch = 0; ch < 4; ch++) {
        for (uint8_t id = 0; id < 2; id++) {
            if (nd_obis_eq(ln, &ND_OBIS_MBUS_DEVICE_IDS[ch][id])) {
                *channel = ch;
                *index = id;
                return 0;
            }
        }
    }
    return -1;
}

static int event_counter_index(const nd_obis_t *ln)
{
    for (uint8_t index = 0; index < 12; index++) {
        if (nd_obis_eq(ln, &ND_OBIS_EVENT_COUNTERS[index])) {
            return index;
        }
    }
    return -1;
}

static uint32_t *u32_object(nd_idis_objects_t *objects, const nd_obis_t *ln)
{
    if (nd_obis_eq(ln, &ND_OBIS_ALARM_REGISTER_2)) {
        return &objects->event.alarm.registers[1];
    }
    if (nd_obis_eq(ln, &ND_OBIS_ALARM_FILTER_1)) {
        return &objects->event.alarm.filters[0];
    }
    if (nd_obis_eq(ln, &ND_OBIS_ALARM_FILTER_2)) {
        return &objects->event.alarm.filters[1];
    }
    if (nd_obis_eq(ln, &ND_OBIS_ALARM_DESCRIPTOR_1)) {
        return &objects->event.descriptors[0];
    }
    if (nd_obis_eq(ln, &ND_OBIS_ALARM_DESCRIPTOR_2)) {
        return &objects->event.descriptors[1];
    }
    if (nd_obis_eq(ln, &ND_OBIS_ERROR_REGISTER)) {
        return &objects->event.error_register;
    }
    return NULL;
}

static nd_idis_profile_state_t *status_profile(nd_idis_objects_t *objects,
                                                const nd_obis_t *ln)
{
    if (nd_obis_eq(ln, &ND_OBIS_PROFILE_STATUS_1)) {
        return nd_idis_profiles_find(&objects->profiles, &ND_OBIS_LOAD_PROFILE_1);
    }
    if (nd_obis_eq(ln, &ND_OBIS_PROFILE_STATUS_2)) {
        return nd_idis_profiles_find(&objects->profiles, &ND_OBIS_LOAD_PROFILE_2);
    }
    if (nd_obis_eq(ln, &ND_OBIS_PROFILE_STATUS_PQ)
        || nd_obis_eq(ln, &ND_OBIS_PROFILE_STATUS_PQ_ALT)) {
        return nd_idis_profiles_find(&objects->profiles, &ND_OBIS_PQ_PROFILE);
    }
    for (uint8_t channel = 0; channel < 4; channel++) {
        if (nd_obis_eq(ln, &ND_OBIS_PROFILE_STATUS_MBUS[channel])) {
            static const nd_obis_t loads[4] = {
                {0, 1, 24, 3, 0, 255}, {0, 2, 24, 3, 0, 255},
                {0, 3, 24, 3, 0, 255}, {0, 4, 24, 3, 0, 255},
            };
            return nd_idis_profiles_find(&objects->profiles,
                                         &loads[channel]);
        }
    }
    return NULL;
}

static int event_object_category(const nd_obis_t *ln,
                                 nd_idis_event_log_category_t *category)
{
    if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_STANDARD)) {
        *category = ND_IDIS_EVENT_LOG_STANDARD;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_FRAUD)) {
        *category = ND_IDIS_EVENT_LOG_FRAUD;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_DISCONNECTOR)) {
        *category = ND_IDIS_EVENT_LOG_DISCONNECTOR;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_MBUS)) {
        *category = ND_IDIS_EVENT_LOG_MBUS;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_PQ)) {
        *category = ND_IDIS_EVENT_LOG_PQ;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_COMM)) {
        *category = ND_IDIS_EVENT_LOG_COMMUNICATION;
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_OBJECT_COMM_DETAILS)) {
        *category = ND_IDIS_EVENT_LOG_COMMUNICATION;
    } else {
        int matched = 0;
        for (uint8_t channel = 0; channel < 4; channel++) {
            if (nd_obis_eq(ln, &ND_OBIS_MBUS_EVENT_OBJECTS[channel])) {
                *category = ND_IDIS_EVENT_LOG_MBUS;
                matched = 1;
                break;
            }
        }
        if (!matched) {
            return -1;
        }
    }
    return 0;
}

int nd_ic_data_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id,
                   const nd_axdr_reader_t *selector,
                   nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)selector;
    if (objects == NULL || ln == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    for (uint8_t index = 0; index < 2; index++) {
        const nd_obis_t *counter = index == 0 ? &ND_OBIS_SECURITY_FC_UNICAST
                                              : &ND_OBIS_SECURITY_FC_BROADCAST;
        if (nd_obis_eq(ln, counter)) {
            if (attribute_id == 1) {
                put_logical_name(out, ln);
            } else if (attribute_id == 2) {
                nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
                nd_axdr_put_u32(out, objects->security_fc_rx[index]);
            } else {
                return ND_ERR_ATTRIBUTE_UNDEFINED;
            }
            return nd_axdr_writer_ok(out) ? ND_OK
                                          : ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    }
    for (uint8_t index = 0; index < 3; index++) {
        int is_id = nd_obis_eq(ln, &ND_OBIS_FIRMWARE_IDS[index]);
        int is_sig = nd_obis_eq(ln, &ND_OBIS_FIRMWARE_SIGNATURES[index]);
        if (!is_id && !is_sig) {
            continue;
        }
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            const uint8_t *data = is_id ? objects->firmware_id[index]
                                        : objects->firmware_sig[index];
            uint8_t length = is_id ? objects->firmware_id_len[index]
                                   : objects->firmware_sig_len[index];
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, length);
            nd_axdr_put_octets(out, data, length);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    nd_idis_event_state_t *state = &objects->event;
    nd_idis_profile_state_t *profile = status_profile(objects, ln);
    nd_idis_event_log_category_t category;
    if (profile != NULL) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, profile->status);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (event_object_category(ln, &category) == 0) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, nd_idis_event_latest_code(state, category));
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_ALARM_REGISTER_1)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, nd_alarm_bits(&state->alarm, 0));
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_EVENT_COUNTER_1)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, state->counters[0]);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (event_counter_index(ln) >= 0) {
        int index = event_counter_index(ln);
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, state->counters[index]);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (u32_object(objects, ln) != NULL) {
        const uint32_t *value = u32_object(objects, ln);
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
            nd_axdr_put_u32(out, *value);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_POWER_FAILURE_COUNT)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, objects->power_failure.count);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_POWER_FAILURE_LONG_COUNT)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, objects->power_failure.long_count);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_LOCAL_TIME)
               || nd_obis_eq(ln, &ND_OBIS_LOCAL_DATE)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_dlms_datetime_t now = {0};
            if (objects->clock.port != NULL
                && objects->clock.port->read != NULL) {
                (void)objects->clock.port->read(objects->clock.port->ctx,
                                                &now);
            }
            if (nd_obis_eq(ln, &ND_OBIS_LOCAL_TIME)) {
                uint8_t time[4] = {now.hour, now.minute, now.second,
                                   now.hundredths};
                nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
                nd_axdr_put_length(out, sizeof(time));
                nd_axdr_put_octets(out, time, sizeof(time));
            } else {
                uint8_t date[5] = {(uint8_t)(now.year >> 8),
                                   (uint8_t)now.year, now.month, now.day,
                                   now.day_of_week};
                nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
                nd_axdr_put_length(out, sizeof(date));
                nd_axdr_put_octets(out, date, sizeof(date));
            }
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_CONSUMER_TEXT)
               || nd_obis_eq(ln, &ND_OBIS_CONSUMER_CODE)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            int text = nd_obis_eq(ln, &ND_OBIS_CONSUMER_TEXT);
            const uint8_t *data =
                text ? objects->consumer_text : objects->consumer_code;
            uint8_t length =
                text ? objects->consumer_text_len : objects->consumer_code_len;
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, length);
            nd_axdr_put_octets(out, data, length);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (nd_obis_eq(ln, &ND_OBIS_LDN)) {
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, objects->ldn_len);
            nd_axdr_put_octets(out, objects->ldn, objects->ldn_len);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else if (product_device_index(ln) >= 0) {
        int index = product_device_index(ln);
        if (attribute_id == 1) {
            put_logical_name(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, objects->device_id_len[index]);
            nd_axdr_put_octets(out, objects->device_id[index],
                               objects->device_id_len[index]);
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
    } else {
        uint8_t channel = 0;
        uint8_t id = 0;
        if (mbus_device_index(ln, &channel, &id) == 0) {
            if (attribute_id == 1) {
                put_logical_name(out, ln);
            } else if (attribute_id == 2) {
                nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
                nd_axdr_put_length(out,
                                   objects->mbus.device_id_len[channel][id]);
                nd_axdr_put_octets(out, objects->mbus.device_id[channel][id],
                                   objects->mbus.device_id_len[channel][id]);
            } else {
                return ND_ERR_ATTRIBUTE_UNDEFINED;
            }
        } else if (nd_obis_eq(ln, &ND_OBIS_ACTIVE_TARIFF)) {
            if (attribute_id == 1) {
                put_logical_name(out, ln);
            } else if (attribute_id == 2) {
                /* Mask name of the active Register activation - Energy. */
                const nd_register_activation_state_t *active =
                    &objects->activation.energy;
                nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
                nd_axdr_put_length(out, active->length);
                nd_axdr_put_octets(out, active->mask, active->length);
            } else {
                return ND_ERR_ATTRIBUTE_UNDEFINED;
            }
        } else {
            return ND_ERR_OBJECT_UNDEFINED;
        }
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_data_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    if (objects != NULL) {
        uint8_t channel = 0;
        uint8_t id = 0;
        if (mbus_device_index(ln, &channel, &id) == 0) {
            if (attribute_id != 2 || in == NULL
                || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint16_t length = nd_axdr_get_length(in);
            if (length > sizeof(objects->mbus.device_id[channel][id])) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, objects->mbus.device_id[channel][id],
                               length);
            if (!nd_axdr_reader_ok(in)) {
                return ND_ERR_TYPE_MISMATCH;
            }
            objects->mbus.device_id_len[channel][id] = (uint8_t)length;
            return ND_OK;
        }
    }
    if (objects != NULL
        && (nd_obis_eq(ln, &ND_OBIS_CONSUMER_TEXT)
            || nd_obis_eq(ln, &ND_OBIS_CONSUMER_CODE))) {
        int text = nd_obis_eq(ln, &ND_OBIS_CONSUMER_TEXT);
        uint8_t *data = text ? objects->consumer_text : objects->consumer_code;
        uint8_t *length = text ? &objects->consumer_text_len
                               : &objects->consumer_code_len;
        if (attribute_id != 2 || in == NULL
            || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t size = nd_axdr_get_length(in);
        if (size > 64) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_axdr_get_octets(in, data, size);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        *length = (uint8_t)size;
        return ND_OK;
    }
    uint32_t *target = objects != NULL ? u32_object(objects, ln) : NULL;
    if (target == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id != 2) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    uint8_t tag = nd_axdr_get_tag(in);
    uint32_t value = 0;
    if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        value = nd_axdr_get_u32(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_UNSIGNED) {
        value = nd_axdr_get_u8(in);
    } else {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (!nd_axdr_reader_ok(in)) {
        return ND_ERR_TYPE_MISMATCH;
    }
    *target = value;
    return ND_OK;
}

#define ND_IDIS_POWER_MAGIC 0x504F5731u

typedef struct {
    uint32_t magic;
    nd_power_failure_state_t state;
} nd_idis_power_record_t;

int nd_idis_power_save(const nd_power_failure_state_t *state,
                       nd_store_t *store, uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_power_record_t record = {.magic = ND_IDIS_POWER_MAGIC,
                                     .state = *state};
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_power_load(nd_power_failure_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_power_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_POWER_MAGIC) {
        return -1;
    }
    *state = record.state;
    return 0;
}
