#include "components/services/channel/nd_channel.h"

int nd_channel_read(const nd_channel_t *channel, uint8_t *buffer,
                    size_t length)
{
    if ((channel == NULL) || (channel->read == NULL)
        || ((buffer == NULL) && (length > 0))) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    return channel->read(channel->ctx, buffer, length);
}

int nd_channel_write(const nd_channel_t *channel, const uint8_t *buffer,
                     size_t length)
{
    if ((channel == NULL) || (channel->write == NULL)
        || ((buffer == NULL) && (length > 0))) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    return channel->write(channel->ctx, buffer, length);
}

int nd_channel_flush(const nd_channel_t *channel)
{
    if ((channel == NULL) || (channel->flush == NULL)) {
        return -1;
    }
    return channel->flush(channel->ctx);
}
