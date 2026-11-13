#ifndef ND_CLOCK_PORT_H
#define ND_CLOCK_PORT_H

#include "components/protocol/dlms/include/nd_axdr.h"

typedef struct {
    void *ctx;
    int (*read)(void *ctx, nd_dlms_datetime_t *now);
    int (*write)(void *ctx, const nd_dlms_datetime_t *now);
} nd_clock_port_t;

#endif
