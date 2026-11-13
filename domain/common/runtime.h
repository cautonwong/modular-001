#ifndef ND_COMMON_RUNTIME_H
#define ND_COMMON_RUNTIME_H

#include <stdint.h>

typedef struct nd_runtime_port {
    void *ctx;
    void (*delay_us)(void *ctx, uint32_t us);
    void (*watchdog_clear)(void *ctx);
} nd_runtime_port_t;

void nd_runtime_bind(const nd_runtime_port_t *port);
void nd_runtime_delay_us(uint32_t us);
void nd_runtime_watchdog_clear(void);

#endif
