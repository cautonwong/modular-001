#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_disconnect.h"

#include "targets/idis_ip_meter_freertos/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event.h"

#define ND_IDIS_DISCONNECT_MAGIC 0x44495331u

typedef struct {
    uint32_t magic;
    nd_disconnect_snapshot_t snapshot;
} nd_idis_disconnect_record_t;

static uint8_t wire_state(nd_disconnect_state_t state)
{
    switch (state) {
    case ND_DISCONNECT_DISCONNECTED:
        return 0;
    case ND_DISCONNECT_CONNECTED:
        return 1;
    case ND_DISCONNECT_READY_FOR_RECONNECTION:
        return 2;
    default:
        return 0;
    }
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


static uint16_t trigger_event_code(nd_disconnect_trigger_t trigger)
{
    switch (trigger) {
    case ND_DISCONNECT_REMOTE_DISCONNECT:
        return 62;
    case ND_DISCONNECT_REMOTE_RECONNECT:
        return 63;
    case ND_DISCONNECT_MANUAL_DISCONNECT:
        return 60;
    case ND_DISCONNECT_MANUAL_RECONNECT:
        return 61;
    case ND_DISCONNECT_LOCAL_DISCONNECT:
        return 64;
    case ND_DISCONNECT_LOCAL_RECONNECT:
        return 69;
    default:
        return 0;
    }
}

static int request_and_log(nd_idis_objects_t *objects,
                           nd_disconnect_snapshot_t *snapshot,
                           nd_disconnect_trigger_t trigger,
                           const nd_datetime_t *now)
{
    if (nd_disconnect_request(snapshot, trigger, now) != 0) {
        return -1;
    }
    uint16_t code = trigger_event_code(trigger);
    if (code != 0) {
        (void)nd_idis_event_raise_kind(&objects->event, ND_EVENT_DISCONNECT,
                                       code, 0, 1, 0, 0);
    }
    return 0;
}

static nd_disconnect_snapshot_t *snapshot_for(nd_idis_objects_t *objects,
                                              const nd_obis_t *ln)
{
    if (objects == NULL || ln == NULL) {
        return NULL;
    }
    if (nd_obis_eq(ln, &ND_OBIS_DISCONNECT_CONTROL)) {
        return &objects->disconnect;
    }
    if (nd_obis_eq(ln, &ND_OBIS_LOAD_CONTROL_1)) {
        return &objects->load_control_1;
    }
    if (nd_obis_eq(ln, &ND_OBIS_LOAD_CONTROL_2)) {
        return &objects->load_control_2;
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_DISCONNECT_1)) {
        return &objects->mbus_disconnect[0];
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_DISCONNECT_2)) {
        return &objects->mbus_disconnect[1];
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_DISCONNECT_3)) {
        return &objects->mbus_disconnect[2];
    }
    if (nd_obis_eq(ln, &ND_OBIS_MBUS_DISCONNECT_4)) {
        return &objects->mbus_disconnect[3];
    }
    return NULL;
}

int nd_ic_disconnect_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id,
                         const nd_axdr_reader_t *selector,
                         nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)selector;
    nd_disconnect_snapshot_t *snapshot = snapshot_for(objects, ln);
    if (snapshot == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_BOOLEAN);
        nd_axdr_put_u8(out, snapshot->output_state);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, wire_state(snapshot->control_state));
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, snapshot->control_mode);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_disconnect_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    nd_disconnect_snapshot_t *snapshot = snapshot_for(objects, ln);
    if (snapshot == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return ND_ERR_TYPE_MISMATCH;
    }
    uint8_t value = nd_axdr_get_u8(in);
    if (attribute_id == 4) {
        if (value > 6) {
            return ND_ERR_TYPE_MISMATCH;
        }
        snapshot->control_mode = value;
        return ND_OK;
    }
    if (attribute_id == 3) {
        /* 2 = local disconnect (ready for reconnection), 1 = local reconnect,
         * 0 = direct remote disconnect. */
        nd_disconnect_trigger_t trigger;
        if (value == 0) {
            trigger = ND_DISCONNECT_REMOTE_DISCONNECT;
        } else if (value == 1) {
            trigger = ND_DISCONNECT_LOCAL_RECONNECT;
        } else if (value == 2) {
            trigger = ND_DISCONNECT_LOCAL_DISCONNECT;
        } else {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_datetime_t now;
        read_now(objects, &now);
        return request_and_log(objects, snapshot, trigger, &now) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

int nd_ic_disconnect_action(void *ctx, uint8_t client_sap,
                            const nd_obis_t *ln, uint8_t method_id,
                            nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)in;
    (void)out;
    nd_disconnect_snapshot_t *snapshot = snapshot_for(objects, ln);
    if (snapshot == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_datetime_t now;
    read_now(objects, &now);
    if (method_id == 1) {
        return request_and_log(objects, snapshot,
                               ND_DISCONNECT_REMOTE_DISCONNECT, &now) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 2) {
        return request_and_log(objects, snapshot,
                               ND_DISCONNECT_REMOTE_RECONNECT, &now) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_disconnect_save(const nd_disconnect_snapshot_t *snapshot,
                          nd_store_t *store, uint32_t offset)
{
    if (snapshot == NULL || store == NULL) {
        return -1;
    }
    nd_idis_disconnect_record_t record = {
        .magic = ND_IDIS_DISCONNECT_MAGIC,
        .snapshot = *snapshot,
    };
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_ic_disconnect_load(nd_disconnect_snapshot_t *snapshot,
                          nd_store_t *store, uint32_t offset)
{
    if (snapshot == NULL || store == NULL) {
        return -1;
    }
    nd_idis_disconnect_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_DISCONNECT_MAGIC) {
        return -1;
    }
    *snapshot = record.snapshot;
    return 0;
}

#define ND_IDIS_RELAYS_MAGIC 0x44495332u
#define ND_IDIS_RELAY_COUNT 6

typedef struct {
    uint32_t magic;
    nd_disconnect_snapshot_t snapshots[ND_IDIS_RELAY_COUNT];
} nd_idis_relays_record_t;

int nd_ic_relays_save(const nd_disconnect_snapshot_t snapshots[6],
                      nd_store_t *store, uint32_t offset)
{
    if (snapshots == NULL || store == NULL) {
        return -1;
    }
    nd_idis_relays_record_t record = {.magic = ND_IDIS_RELAYS_MAGIC};
    for (uint8_t index = 0; index < ND_IDIS_RELAY_COUNT; index++) {
        record.snapshots[index] = snapshots[index];
    }
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_ic_relays_load(nd_disconnect_snapshot_t snapshots[6], nd_store_t *store,
                      uint32_t offset)
{
    if (snapshots == NULL || store == NULL) {
        return -1;
    }
    nd_idis_relays_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_RELAYS_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_RELAY_COUNT; index++) {
        snapshots[index] = record.snapshots[index];
    }
    return 0;
}
