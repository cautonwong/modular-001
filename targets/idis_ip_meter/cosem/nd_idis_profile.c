#include "targets/idis_ip_meter/cosem/nd_idis_profile.h"

#include <string.h>

#include "domain/freeze/capture.h"
#include "domain/freeze/schedule.h"
#include "targets/idis_ip_meter/cosem/nd_idis_capture.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

int nd_idis_profile_init(nd_idis_profile_state_t *profile, nd_store_t *store,
                         uint32_t base_offset, uint16_t capacity,
                         uint16_t slot_size, const nd_obis_t *ln,
                         const nd_clock_port_t *clock,
                         const nd_capture_object_t *captures,
                         uint8_t capture_count, uint32_t capture_period,
                         uint8_t compressed, uint8_t delta)
{
    if (profile == NULL || store == NULL || ln == NULL
        || capture_count > ND_IDIS_PROFILE_MAX_CAPTURES
        || (captures == NULL && capture_count != 0)) {
        return -1;
    }
    memset(profile, 0, sizeof(*profile));
    profile->ln = *ln;
    profile->clock = clock;
    profile->capture_count = capture_count;
    profile->capture_period = capture_period;
    profile->compressed = compressed != 0;
    profile->delta = delta != 0;
    for (uint8_t index = 0; index < capture_count; index++) {
        profile->captures[index] = captures[index];
    }
    return nd_profile_store_init(&profile->store, store, base_offset, capacity,
                                 slot_size);
}

void nd_idis_profile_set_status(nd_idis_profile_state_t *profile,
                                uint8_t status)
{
    if (profile != NULL) {
        profile->status = status;
    }
}

static int is_status_capture(const nd_capture_object_t *capture)
{
    return capture->class_id == ND_IC_DATA
        && capture->attribute_id == 2
        && (nd_obis_eq(&capture->ln, &ND_OBIS_PROFILE_STATUS_1)
            || nd_obis_eq(&capture->ln, &ND_OBIS_PROFILE_STATUS_2)
            || nd_obis_eq(&capture->ln, &ND_OBIS_PROFILE_STATUS_PQ));
}

int nd_idis_profile_capture(nd_idis_profile_state_t *profile,
                            const nd_cosem_port_t *port, uint8_t client_sap,
                            const nd_clock_value_t *now)
{
    if (profile == NULL || port == NULL) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }

    nd_capture_value_t values[ND_IDIS_PROFILE_MAX_CAPTURES] = {0};
    for (uint8_t index = 0; index < profile->capture_count; index++) {
        const nd_capture_object_t *capture = &profile->captures[index];
        if (is_status_capture(capture)) {
            values[index].type = ND_CAPTURE_UNSIGNED;
            values[index].value.unsigned_value = profile->status;
            continue;
        }
        uint8_t temp[32];
        nd_axdr_writer_t raw;
        nd_axdr_writer_init(&raw, temp, sizeof(temp));
        int result = port->get(port->ctx, client_sap, capture->class_id,
                               &capture->ln, capture->attribute_id, NULL,
                               &raw);
        /*
         * shortcut: capture objects that the target cannot currently serve
         * (M-Bus, limiter, power failure duration, optional registers) store
         * null-data instead of failing the whole capture.
         */
        if (result != ND_OK || !nd_axdr_writer_ok(&raw)) {
            values[index].type = ND_CAPTURE_NONE;
            continue;
        }
        nd_axdr_reader_t reader;
        nd_axdr_reader_init(&reader, temp, raw.len);
        if (nd_idis_capture_read(capture, &reader, &values[index]) != 0) {
            /* Unsupported value type: keep the column as null-data. */
            values[index].type = ND_CAPTURE_NONE;
        }
    }
    nd_clock_value_t zero = {0};
    const nd_clock_value_t *time = now != NULL ? now : &zero;
    nd_capture_snapshot_t snapshot = {
        .capture_time = *time,
        .status = profile->status,
        .count = profile->capture_count,
        .values = values,
    };
    nd_axdr_writer_t writer;
    uint8_t record[ND_IDIS_PROFILE_RECORD_MAX];
    nd_axdr_writer_init(&writer, record, sizeof(record));
    for (uint8_t index = 0; index < profile->capture_count; index++) {
        if (nd_idis_capture_write(&profile->captures[index], &values[index],
                                  &writer) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
    }
    if (writer.len == 0 || !nd_axdr_writer_ok(&writer)) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_profile_store_append(&profile->store, &snapshot.capture_time.time,
                                snapshot.status, record,
                                writer.len) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (profile->after_capture != NULL) {
        profile->after_capture(profile->after_capture_ctx);
    }
    return ND_OK;
}
int nd_idis_profile_on_second(nd_idis_profile_state_t *profile,
                              const nd_cosem_port_t *port, uint8_t client_sap,
                              const nd_clock_value_t *now)
{
    if (profile == NULL || now == NULL || profile->capture_period == 0) {
        return 0;
    }
    if (profile->capture_period % 60u == 0) {
        nd_freeze_period_t period = {
            .valid = 1,
            .interval = (uint16_t)(profile->capture_period / 60u),
        };
        if (!nd_freeze_period_due(&now->time, &period, 1)) {
            return 0;
        }
    } else {
        uint32_t seconds = (uint32_t)now->time.hour * 3600u
                         + (uint32_t)now->time.minute * 60u + now->time.second;
        if (seconds % profile->capture_period != 0) {
            return 0;
        }
    }
    return nd_idis_profile_capture(profile, port, client_sap, now) == ND_OK
               ? 1
               : -1;
}
