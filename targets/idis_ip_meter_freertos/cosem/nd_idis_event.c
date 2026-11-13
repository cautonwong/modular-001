#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event_desc.h"

#include <stddef.h>
#include <string.h>

void nd_idis_event_init(nd_idis_event_state_t *state,
                        const nd_idis_event_rule_t *rules, uint16_t count)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    nd_alarm_init(&state->alarm, rules, count);
    nd_event_log_init(&state->log);
    state->latest_event_code = 255;
    for (size_t index = 0; index < sizeof(state->counters)
                                      / sizeof(state->counters[0]); index++) {
        state->counters[index] = 0;
    }
}

void nd_idis_event_bind_log(nd_idis_event_state_t *state,
                            nd_idis_profile_state_t *log_profile,
                            const nd_cosem_port_t *port,
                            const nd_clock_port_t *clock, uint8_t client_sap)
{
    if (state == NULL) {
        return;
    }
    state->log_profile = log_profile;
    state->category_logs[ND_IDIS_EVENT_LOG_STANDARD] = log_profile;
    state->port = port;
    state->clock = clock;
    state->client_sap = client_sap;
}

void nd_idis_event_bind_category_log(nd_idis_event_state_t *state,
                                     nd_idis_event_log_category_t category,
                                     nd_idis_profile_state_t *log_profile)
{
    if (state == NULL || category >= ND_IDIS_EVENT_LOG_CATEGORY_COUNT) {
        return;
    }
    state->category_logs[category] = log_profile;
    if (category == ND_IDIS_EVENT_LOG_STANDARD) {
        state->log_profile = log_profile;
    }
}

nd_idis_event_log_category_t
nd_idis_event_category_for_kind(nd_event_kind_t kind)
{
    switch (kind) {
    case ND_EVENT_POWER_OFF:
    case ND_EVENT_POWER_UP:
        return ND_IDIS_EVENT_LOG_POWER_FAILURE;
    case ND_EVENT_FRAUD:
        return ND_IDIS_EVENT_LOG_FRAUD;
    case ND_EVENT_DISCONNECT:
    case ND_EVENT_LOAD_CONTROL:
        return ND_IDIS_EVENT_LOG_DISCONNECTOR;
    case ND_EVENT_MBUS:
        return ND_IDIS_EVENT_LOG_MBUS;
    case ND_EVENT_VOLTAGE_SAG:
    case ND_EVENT_VOLTAGE_SWELL:
    case ND_EVENT_VOLTAGE_MISSING:
    case ND_EVENT_FREQUENCY_LOW:
    case ND_EVENT_FREQUENCY_HIGH:
    case ND_EVENT_VOLTAGE_UNBALANCE:
        return ND_IDIS_EVENT_LOG_PQ;
    case ND_EVENT_COMMUNICATION:
        return ND_IDIS_EVENT_LOG_COMMUNICATION;
    default:
        return ND_IDIS_EVENT_LOG_STANDARD;
    }
}

uint8_t nd_idis_event_latest_code(const nd_idis_event_state_t *state,
                                  nd_idis_event_log_category_t category)
{
    if (state == NULL) {
        return 255;
    }
    if (category == ND_IDIS_EVENT_LOG_STANDARD) {
        return state->latest_event_code;
    }
    uint16_t count = state->log.count;
    if (count > ND_EVENT_LOG_MAX) {
        count = ND_EVENT_LOG_MAX;
    }
    for (uint16_t offset = 0; offset < count; offset++) {
        uint16_t index = (uint16_t)((state->log.head + ND_EVENT_LOG_MAX - 1
                                     - offset)
                                    % ND_EVENT_LOG_MAX);
        if (nd_idis_event_category_for_kind(state->log.entries[index].kind)
            == category) {
            return (uint8_t)state->log_codes[index];
        }
    }
    return 255;
}

void nd_idis_event_bind_profiles(nd_idis_event_state_t *state,
                                 nd_idis_profiles_t *profiles)
{
    if (state != NULL) {
        state->profiles = profiles;
        for (uint8_t index = 0; profiles != NULL && index < profiles->count;
             index++) {
            nd_idis_profile_set_status(&profiles->items[index],
                                       state->profile_status);
        }
    }
}

static uint8_t status_bits_for_code(uint16_t event_code)
{
    switch (event_code) {
    case 1:
        return ND_PROFILE_STATUS_PDN;
    case 4:
    case 5:
        return ND_PROFILE_STATUS_CAD;
    case 6:
        return ND_PROFILE_STATUS_CIV | ND_PROFILE_STATUS_DNV;
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return ND_PROFILE_STATUS_ERR | ND_PROFILE_STATUS_DNV;
    default:
        return 0;
    }
}

static int raise_impl(nd_idis_event_state_t *state, uint16_t event_code,
                      nd_event_kind_t kind, int use_descriptor, uint8_t source,
                      uint8_t severity, uint32_t duration_seconds,
                      int32_t value)
{
    if (state == NULL) {
        return -1;
    }
    const nd_idis_event_descriptor_t *descriptor =
        nd_idis_event_describe(event_code);
    if (use_descriptor) {
        source = descriptor != NULL ? descriptor->source : 0;
        severity = descriptor != NULL ? descriptor->severity : 0;
    }
    nd_dlms_datetime_t now = {0};
    int have_now = state->clock != NULL && state->clock->read != NULL
        && state->clock->read(state->clock->ctx, &now) == 0;
    nd_event_record_t record = {
        .kind = kind != ND_EVENT_NONE
                    ? kind
                    : (descriptor != NULL ? descriptor->kind : ND_EVENT_GENERIC),
        .source = source,
        .severity = severity,
        .duration_seconds = duration_seconds,
        .time = {
            .second = now.second,
            .minute = now.minute,
            .hour = now.hour,
            .day = now.day,
            .month = now.month,
            .year = now.year,
        },
        .value = use_descriptor ? event_code : value,
    };
    (void)nd_event_log_append(&state->log, &record);
    uint16_t newest = (uint16_t)((state->log.head + ND_EVENT_LOG_MAX - 1)
                                 % ND_EVENT_LOG_MAX);
    state->log_codes[newest] = event_code;
    uint8_t bits = status_bits_for_code(event_code);
    if (bits != 0) {
        nd_profile_status_set(&state->profile_status, bits);
        if (state->log_profile != NULL) {
            nd_idis_profile_set_status(state->log_profile, state->profile_status);
        }
        if (state->profiles != NULL) {
            for (uint8_t index = 0; index < state->profiles->count; index++) {
                nd_idis_profile_set_status(&state->profiles->items[index],
                                           state->profile_status);
            }
        }
    }
    state->latest_event_code = (uint8_t)event_code;
    state->counters[0]++;
    (void)nd_alarm_raise(&state->alarm, event_code);

    if (have_now && state->port != NULL) {
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
        if (state->log_profile != NULL) {
            (void)nd_idis_profile_capture(state->log_profile, state->port,
                                          state->client_sap, &clock);
        }
        nd_idis_event_log_category_t category =
            nd_idis_event_category_for_kind(record.kind);
        if (category != ND_IDIS_EVENT_LOG_STANDARD
            && state->category_logs[category] != NULL) {
            (void)nd_idis_profile_capture(state->category_logs[category],
                                          state->port, state->client_sap,
                                          &clock);
        }
    }
    return 0;
}

int nd_idis_event_raise(nd_idis_event_state_t *state, uint16_t event_code)
{
    return raise_impl(state, event_code, ND_EVENT_NONE, 1, 0, 0, 0, 0);
}

int nd_idis_event_raise_ex(nd_idis_event_state_t *state, uint16_t event_code,
                           uint8_t source, uint8_t severity,
                           uint32_t duration_seconds, int32_t value)
{
    return raise_impl(state, event_code, ND_EVENT_NONE, 0, source, severity,
                      duration_seconds, value);
}

int nd_idis_event_raise_kind(nd_idis_event_state_t *state,
                             nd_event_kind_t kind, uint16_t event_code,
                             uint8_t source, uint8_t severity,
                             uint32_t duration_seconds, int32_t value)
{
    return raise_impl(state, event_code, kind, 0, source, severity,
                      duration_seconds, value);
}

void nd_idis_event_communication(nd_idis_event_state_t *state,
                                 nd_idis_communication_interface_t interface,
                                 int start)
{
    if (state == NULL) {
        return;
    }
    uint16_t code;
    if (interface == ND_IDIS_COMMUNICATION_REMOTE) {
        code = start ? 26 : 27;
    } else {
        code = start ? 28 : 29;
    }
    (void)nd_idis_event_raise_kind(state, ND_EVENT_COMMUNICATION, code,
                                   0, 1, 0, 0);
}

int nd_idis_event_save(const nd_idis_event_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_event_record_t record = {
        .magic = ND_IDIS_EVENT_MAGIC,
        .latest_event_code = state->latest_event_code,
        .profile_status = state->profile_status,
        .log_count = (uint8_t)state->log.count,
        .log_head = (uint8_t)state->log.head,
    };
    memcpy(record.log, state->log.entries, sizeof(record.log));
    memcpy(record.log_codes, state->log_codes, sizeof(record.log_codes));
    memcpy(record.counters, state->counters, sizeof(record.counters));
    memcpy(record.registers, state->alarm.registers,
           sizeof(record.registers));
    memcpy(record.filters, state->alarm.filters, sizeof(record.filters));
    memcpy(record.descriptors, state->descriptors,
           sizeof(record.descriptors));
    record.error_register = state->error_register;
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_event_load(nd_idis_event_state_t *state, nd_store_t *store,
                       uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_event_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_EVENT_MAGIC) {
        return -1;
    }
    state->latest_event_code = record.latest_event_code;
    state->profile_status = record.profile_status;
    state->log.count = record.log_count <= ND_EVENT_LOG_MAX ? record.log_count
                                                             : ND_EVENT_LOG_MAX;
    state->log.head = record.log_head < ND_EVENT_LOG_MAX ? record.log_head : 0;
    memcpy(state->log.entries, record.log, sizeof(record.log));
    memcpy(state->log_codes, record.log_codes, sizeof(record.log_codes));
    memcpy(state->counters, record.counters, sizeof(state->counters));
    memcpy(state->alarm.registers, record.registers,
           sizeof(state->alarm.registers));
    memcpy(state->alarm.filters, record.filters, sizeof(state->alarm.filters));
    memcpy(state->descriptors, record.descriptors,
           sizeof(state->descriptors));
    state->error_register = record.error_register;
    return 0;
}
