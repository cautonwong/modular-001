#ifndef ND_APPLICATION_PORTS_PLATFORM_H
#define ND_APPLICATION_PORTS_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct nd_platform_port {
    void *ctx;
    void (*watchdog_kick)(void *ctx);
    void (*event_output_set)(void *ctx, bool enable);
    bool (*device_error)(void *ctx);
    /* Called once the device error has been reported, so a latched fault may
     * clear only after the reporter observed it. */
    void (*device_error_reported)(void *ctx);
} nd_platform_port_t;

void nd_platform_watchdog_kick(const nd_platform_port_t *port);
void nd_platform_event_output_set(const nd_platform_port_t *port, bool enable);
/* True while the board reports a memory (EEPROM) fault. */
bool nd_platform_device_error(const nd_platform_port_t *port);
void nd_platform_device_error_reported(const nd_platform_port_t *port);

#endif
