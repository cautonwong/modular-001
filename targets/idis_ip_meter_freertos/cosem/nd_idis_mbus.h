#ifndef ND_IDIS_MBUS_H
#define ND_IDIS_MBUS_H

#include <stdint.h>

#include "domain/common/store.h"
#include "domain/mbus/mbus_client.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_value.h"

#define ND_IDIS_MBUS_PORTS 2
#define ND_IDIS_MBUS_PORT_SPEED_DEFAULT 3 /* 2400 baud */

#define ND_IDIS_MBUS_CAPTURE_MAX 8

typedef struct {
    uint16_t class_id;
    nd_obis_t ln;
    uint8_t attribute;
    uint16_t index;
} nd_idis_mbus_capture_t;

typedef struct {
    nd_mbus_state_t state;
    nd_idis_mbus_value_t values[ND_MBUS_CHANNELS]
                                [ND_IDIS_MBUS_VALUES_PER_CHANNEL];
    uint8_t port_speed[ND_IDIS_MBUS_PORTS];
    uint8_t device_id_len[ND_MBUS_CHANNELS][2];
    uint8_t device_id[ND_MBUS_CHANNELS][2][48];
    uint8_t capture_count[ND_MBUS_CHANNELS];
    nd_idis_mbus_capture_t captures[ND_MBUS_CHANNELS]
                                      [ND_IDIS_MBUS_CAPTURE_MAX];
    uint32_t capture_timer[ND_MBUS_CHANNELS];
} nd_idis_mbus_t;

/* Initializes clients and value objects; the link port stays unbound. */
void nd_idis_mbus_init(nd_idis_mbus_t *mbus);

/* Persists clients, port speeds and value objects (not the port pointer). */
int nd_idis_mbus_save(const nd_idis_mbus_t *mbus, nd_store_t *store,
                      uint32_t offset);
int nd_idis_mbus_load(nd_idis_mbus_t *mbus, nd_store_t *store,
                      uint32_t offset);

/* Periodic capture driven by the client's capture_period attribute. */
int nd_idis_mbus_on_second(nd_idis_mbus_t *mbus, uint8_t channel);

#endif
