#include "targets/idis_ip_meter_freertos/cosem/nd_idis_mbus.h"

#include <string.h>

#define ND_IDIS_MBUS_MAGIC 0x4D425532u

typedef struct {
    uint32_t magic;
    nd_mbus_client_t clients[ND_MBUS_CHANNELS];
    uint8_t port_speed[ND_IDIS_MBUS_PORTS];
    nd_idis_mbus_value_t values[ND_MBUS_CHANNELS]
                               [ND_IDIS_MBUS_VALUES_PER_CHANNEL];
    uint8_t device_id_len[ND_MBUS_CHANNELS][2];
    uint8_t device_id[ND_MBUS_CHANNELS][2][48];
    uint8_t capture_count[ND_MBUS_CHANNELS];
    nd_idis_mbus_capture_t captures[ND_MBUS_CHANNELS]
                                      [ND_IDIS_MBUS_CAPTURE_MAX];
    uint32_t capture_timer[ND_MBUS_CHANNELS];
} nd_idis_mbus_record_t;

void nd_idis_mbus_init(nd_idis_mbus_t *mbus)
{
    if (mbus == NULL) {
        return;
    }
    memset(mbus, 0, sizeof(*mbus));
    nd_mbus_init(&mbus->state);
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        mbus->state.clients[channel].port_index =
            channel < 2 ? 1 : 2; /* two ports, two channels each by default */
    }
    for (uint8_t port = 0; port < ND_IDIS_MBUS_PORTS; port++) {
        mbus->port_speed[port] = ND_IDIS_MBUS_PORT_SPEED_DEFAULT;
    }
}

int nd_idis_mbus_save(const nd_idis_mbus_t *mbus, nd_store_t *store,
                      uint32_t offset)
{
    if (mbus == NULL || store == NULL) {
        return -1;
    }
    nd_idis_mbus_record_t record = {.magic = ND_IDIS_MBUS_MAGIC};
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        record.clients[channel] = mbus->state.clients[channel];
        for (uint8_t value = 0; value < ND_IDIS_MBUS_VALUES_PER_CHANNEL;
             value++) {
            record.values[channel][value] = mbus->values[channel][value];
        }
    }
    for (uint8_t port = 0; port < ND_IDIS_MBUS_PORTS; port++) {
        record.port_speed[port] = mbus->port_speed[port];
    }
    memcpy(record.device_id_len, mbus->device_id_len,
           sizeof(record.device_id_len));
    memcpy(record.device_id, mbus->device_id, sizeof(record.device_id));
    memcpy(record.capture_count, mbus->capture_count,
           sizeof(record.capture_count));
    memcpy(record.captures, mbus->captures, sizeof(record.captures));
    memcpy(record.capture_timer, mbus->capture_timer,
           sizeof(record.capture_timer));
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_mbus_load(nd_idis_mbus_t *mbus, nd_store_t *store, uint32_t offset)
{
    if (mbus == NULL || store == NULL) {
        return -1;
    }
    nd_idis_mbus_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_MBUS_MAGIC) {
        return -1;
    }
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        mbus->state.clients[channel] = record.clients[channel];
        for (uint8_t value = 0; value < ND_IDIS_MBUS_VALUES_PER_CHANNEL;
             value++) {
            mbus->values[channel][value] = record.values[channel][value];
        }
    }
    for (uint8_t port = 0; port < ND_IDIS_MBUS_PORTS; port++) {
        mbus->port_speed[port] = record.port_speed[port];
    }
    memcpy(mbus->device_id_len, record.device_id_len,
           sizeof(mbus->device_id_len));
    memcpy(mbus->device_id, record.device_id, sizeof(mbus->device_id));
    memcpy(mbus->capture_count, record.capture_count,
           sizeof(mbus->capture_count));
    memcpy(mbus->captures, record.captures, sizeof(mbus->captures));
    memcpy(mbus->capture_timer, record.capture_timer,
           sizeof(mbus->capture_timer));
    return 0;
}
