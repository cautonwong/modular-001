#ifndef ND_PROTOCOL_DL698_H
#define ND_PROTOCOL_DL698_H

#include <stddef.h>
#include <stdint.h>

#include "components/services/channel/nd_channel.h"
#include "application/adapters/dl698/fr_698_message.h"

struct instance_t;
struct context_698_t;
struct fifo_t;

typedef struct nd_dl698 nd_dl698_t;

typedef struct {
    struct instance_t *instance;
    struct context_698_t *context;
    uint8_t pid;
    uint32_t (*now_ms)(struct instance_t *instance);
    nd_channel_t *input;
    nd_channel_t *output;
} nd_dl698_config_t;

nd_dl698_t *nd_dl698_create(const nd_dl698_config_t *config);
void nd_dl698_destroy(nd_dl698_t *self);
int nd_dl698_poll(nd_dl698_t *self);
int nd_dl698_send(nd_dl698_t *self, nd_dl698_msg_t *mesg);

#endif
