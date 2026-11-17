#ifndef ND_PROTOCOL_DL645_H
#define ND_PROTOCOL_DL645_H

#include <stddef.h>
#include <stdint.h>

#include "components/services/channel/nd_channel.h"
struct context_645_t;

typedef struct nd_dl645 nd_dl645_t;

typedef struct {
    void *instance;
    struct context_645_t *context;
    uint32_t (*now_ms)(void *instance);
    uint8_t pid;
    nd_channel_t *input;
    nd_channel_t *output;
} nd_dl645_config_t;

nd_dl645_t *nd_dl645_create(const nd_dl645_config_t *config);
void nd_dl645_destroy(nd_dl645_t *self);
int nd_dl645_poll(nd_dl645_t *self);

#endif
