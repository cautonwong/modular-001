#ifndef ND_COMMON_TIMER_H
#define ND_COMMON_TIMER_H

#include <stdint.h>

#define ND_TIMER_MULTIFUNCTION_PORT 6

typedef struct nd_timer_port {
    void *ctx;
    void (*start_one)(void *ctx, uint8_t timer_id, uint32_t count);
} nd_timer_port_t;

void nd_timer_bind(const nd_timer_port_t *port);
void nd_timer_start_one(uint8_t timer_id, uint32_t count);

#endif
