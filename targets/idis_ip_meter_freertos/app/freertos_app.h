#ifndef ND_IDIS_FREERTOS_APP_H
#define ND_IDIS_FREERTOS_APP_H

#include <stdint.h>

/* Starts the product tasks. Never returns after the scheduler starts. */
void freertos_start(void);

/* One-second product tick, called by the application task. */
void idis_app_tick(void);

uint32_t freertos_uptime_seconds(void);

/* Coalescing request for the low-priority persistence workqueue. */
void freertos_request_save(void);
void idis_save_persisted(void);

#endif
