#ifndef ND_DOMAIN_LOAD_CONTROL_DISCONNECT_H
#define ND_DOMAIN_LOAD_CONTROL_DISCONNECT_H

#include <stdint.h>

#include "domain/common/calendar.h"

typedef enum {
    ND_DISCONNECT_DISCONNECTED = 0,
    ND_DISCONNECT_CONNECTED = 1,
    ND_DISCONNECT_READY_FOR_RECONNECTION = 2
} nd_disconnect_state_t;

typedef enum {
    ND_DISCONNECT_REMOTE_DISCONNECT,
    ND_DISCONNECT_REMOTE_RECONNECT,
    ND_DISCONNECT_MANUAL_DISCONNECT,
    ND_DISCONNECT_MANUAL_RECONNECT,
    ND_DISCONNECT_LOCAL_DISCONNECT,
    ND_DISCONNECT_LOCAL_RECONNECT
} nd_disconnect_trigger_t;

typedef struct {
    nd_disconnect_state_t control_state;
    uint8_t output_state;
    uint8_t control_mode; /* 0..6, Blue Book Table 25 */
    nd_disconnect_trigger_t last_trigger;
    nd_datetime_t last_change;
} nd_disconnect_snapshot_t;


void nd_disconnect_init(nd_disconnect_snapshot_t *snapshot, uint8_t control_mode,
                        const nd_datetime_t *now);
int nd_disconnect_request(nd_disconnect_snapshot_t *snapshot,
                          nd_disconnect_trigger_t trigger,
                          const nd_datetime_t *now);
int nd_disconnect_set_output(nd_disconnect_snapshot_t *snapshot,
                             uint8_t output_state);

#endif
