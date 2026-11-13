#ifndef ND_INFRA_BOARD_RELAY_CONFIG_H
#define ND_INFRA_BOARD_RELAY_CONFIG_H

#include <stdint.h>

/* Relay feedback must stay stable for this many one-second samples. */
#define Relay_Ck_Time 200

uint8_t Magnetic_hold_relay_Init(void);

#endif
