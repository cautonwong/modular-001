#include "infra/channel/fifo_channel.h"

#include <stdint.h>

#include "infra/kernel/fifo.h"

static int fifo_channel_read(void *ctx, uint8_t *buffer, size_t length)
{
    nd_fifo_channel_t *storage = ctx;

    if (length > UINT16_MAX) {
        length = UINT16_MAX;
    }
    return (int)fifoBuf_getData(storage->fifo, buffer, (uint16_t)length);
}

static int fifo_channel_write(void *ctx, const uint8_t *buffer, size_t length)
{
    nd_fifo_channel_t *storage = ctx;

    if (length > UINT16_MAX) {
        length = UINT16_MAX;
    }
    return (int)fifoBuf_putData(storage->fifo, buffer, (uint16_t)length);
}

static int fifo_channel_flush(void *ctx)
{
    (void)ctx;
    return 0;
}

void nd_fifo_channel_init(nd_channel_t *channel, nd_fifo_channel_t *storage,
                          struct fifo_t *fifo)
{
    if ((channel == NULL) || (storage == NULL)) {
        return;
    }

    storage->fifo = fifo;
    channel->ctx = storage;
    channel->read = fifo_channel_read;
    channel->write = fifo_channel_write;
    channel->flush = fifo_channel_flush;
}

struct fifo_t *nd_fifo_channel_fifo(const nd_channel_t *channel)
{
    if ((channel == NULL) || (channel->read != fifo_channel_read)) {
        return NULL;
    }
    return ((nd_fifo_channel_t *)channel->ctx)->fifo;
}
