#ifndef ND_APPLICATION_PORTS_PLATFORM_H
#define ND_APPLICATION_PORTS_PLATFORM_H

#include <stdbool.h>

typedef struct nd_platform_port {
    void *ctx;
    void (*watchdog_kick)(void *ctx);
    void (*event_output_set)(void *ctx, bool enable);
} nd_platform_port_t;

void nd_platform_bind(const nd_platform_port_t *port);
void nd_platform_watchdog_kick(void);
void nd_platform_event_output_set(bool enable);

#endif
