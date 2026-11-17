#ifndef ND_TARGET_STATEGRID_RELAY_CONTROL_ADAPTER_H
#define ND_TARGET_STATEGRID_RELAY_CONTROL_ADAPTER_H

#include "targets/stategrid_meter/app/stategrid_context.h"

#include <stdint.h>
#include "domain/load_control/relay_control_port.h"

void save_relay_state(stategrid_context_t *instance);
uint8_t Magnetic_hold_relay_connect(void);
uint8_t Magnetic_hold_relay_disconnect(void);

#endif
