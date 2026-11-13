#ifndef ND_IDIS_BAREMETAL_H
#define ND_IDIS_BAREMETAL_H

#include <stdint.h>

/* Bare-metal exception entry and the 10 ms tick counter. */
uint32_t baremetal_uptime_seconds(void);

void SysTick_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);

#endif
