#ifndef ND_SERVICES_CHANNEL_H
#define ND_SERVICES_CHANNEL_H

#include <stddef.h>
#include <stdint.h>

/*
 * A non-blocking byte channel. read and write return the number of bytes
 * transferred, 0 when nothing is available or accepted, and a negative value
 * on error. A partial write is allowed; the caller keeps the remaining bytes.
 */
typedef struct nd_channel {
    void *ctx;
    int (*read)(void *ctx, uint8_t *buffer, size_t length);
    int (*write)(void *ctx, const uint8_t *buffer, size_t length);
    int (*flush)(void *ctx);
} nd_channel_t;

int nd_channel_read(const nd_channel_t *channel, uint8_t *buffer,
                    size_t length);
int nd_channel_write(const nd_channel_t *channel, const uint8_t *buffer,
                     size_t length);
int nd_channel_flush(const nd_channel_t *channel);

#endif
