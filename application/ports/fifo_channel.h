#ifndef ND_FIFO_CHANNEL_PORT_H
#define ND_FIFO_CHANNEL_PORT_H

#include "components/services/channel/nd_channel.h"

struct fifo_t;

/*
 * FIFO-backed channel storage. The interface is declared with the consumer
 * so protocol components do not include the infra implementation header.
 */
typedef struct {
    struct fifo_t *fifo;
} nd_fifo_channel_t;

void nd_fifo_channel_init(nd_channel_t *channel, nd_fifo_channel_t *storage,
                          struct fifo_t *fifo);

struct fifo_t *nd_fifo_channel_fifo(const nd_channel_t *channel);

#endif
