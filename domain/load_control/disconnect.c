#include <stddef.h>
#include "domain/load_control/disconnect.h"

static int local_disconnect_allowed(uint8_t mode)
{
    return mode == 1 || mode == 2 || mode == 5;
}

static int local_reconnect_allowed(uint8_t mode)
{
    return mode >= 2 && mode <= 6;
}

static int direct_remote_reconnect(uint8_t mode)
{
    return mode == 2 || mode == 4;
}

static int trigger_allowed(uint8_t mode, nd_disconnect_trigger_t trigger)
{
    if (mode == 0 || mode > 6) {
        return 0;
    }
    switch (trigger) {
    case ND_DISCONNECT_REMOTE_DISCONNECT:
    case ND_DISCONNECT_MANUAL_DISCONNECT:
        return 1;
    case ND_DISCONNECT_LOCAL_DISCONNECT:
        return local_disconnect_allowed(mode);
    case ND_DISCONNECT_REMOTE_RECONNECT:
    case ND_DISCONNECT_MANUAL_RECONNECT:
        return 1;
    case ND_DISCONNECT_LOCAL_RECONNECT:
        return local_reconnect_allowed(mode);
    default:
        return 0;
    }
}

static void set_state(nd_disconnect_snapshot_t *snapshot,
                      nd_disconnect_state_t state, nd_disconnect_trigger_t trigger,
                      const nd_datetime_t *now)
{
    snapshot->control_state = state;
    snapshot->output_state = state == ND_DISCONNECT_CONNECTED ? 1 : 0;
    snapshot->last_trigger = trigger;
    snapshot->last_change = *now;
}

void nd_disconnect_init(nd_disconnect_snapshot_t *snapshot, uint8_t control_mode,
                        const nd_datetime_t *now)
{
    if (snapshot == NULL) {
        return;
    }
    snapshot->control_state = ND_DISCONNECT_CONNECTED;
    snapshot->output_state = 1;
    snapshot->control_mode = control_mode <= 6 ? control_mode : 0;
    snapshot->last_trigger = ND_DISCONNECT_MANUAL_RECONNECT;
    if (now != NULL) {
        snapshot->last_change = *now;
    } else {
        snapshot->last_change = (nd_datetime_t){0};
    }
}

int nd_disconnect_request(nd_disconnect_snapshot_t *snapshot,
                          nd_disconnect_trigger_t trigger,
                          const nd_datetime_t *now)
{
    if (snapshot == NULL || now == NULL
        || !trigger_allowed(snapshot->control_mode, trigger)) {
        return -1;
    }
    switch (trigger) {
    case ND_DISCONNECT_REMOTE_DISCONNECT:
        if (snapshot->control_state == ND_DISCONNECT_CONNECTED
            || snapshot->control_state == ND_DISCONNECT_READY_FOR_RECONNECTION) {
            set_state(snapshot, ND_DISCONNECT_DISCONNECTED, trigger, now);
            return 0;
        }
        break;
    case ND_DISCONNECT_MANUAL_DISCONNECT:
    case ND_DISCONNECT_LOCAL_DISCONNECT:
        if (snapshot->control_state == ND_DISCONNECT_CONNECTED) {
            set_state(snapshot, ND_DISCONNECT_READY_FOR_RECONNECTION, trigger,
                      now);
            return 0;
        }
        break;
    case ND_DISCONNECT_REMOTE_RECONNECT:
        if (snapshot->control_state == ND_DISCONNECT_DISCONNECTED) {
            set_state(snapshot,
                      direct_remote_reconnect(snapshot->control_mode)
                          ? ND_DISCONNECT_CONNECTED
                          : ND_DISCONNECT_READY_FOR_RECONNECTION,
                      trigger, now);
            return 0;
        }
        break;
    case ND_DISCONNECT_MANUAL_RECONNECT:
    case ND_DISCONNECT_LOCAL_RECONNECT:
        if (snapshot->control_state == ND_DISCONNECT_READY_FOR_RECONNECTION) {
            set_state(snapshot, ND_DISCONNECT_CONNECTED, trigger, now);
            return 0;
        }
        break;
    default:
        break;
    }
    return -1;
}

int nd_disconnect_set_output(nd_disconnect_snapshot_t *snapshot,
                             uint8_t output_state)
{
    if (snapshot == NULL) {
        return -1;
    }
    snapshot->output_state = output_state != 0;
    return 0;
}
