#ifndef ND_SERVICES_MAINTENANCE_H
#define ND_SERVICES_MAINTENANCE_H

#include <stdint.h>

/*
 * One framed maintenance command from the local optical port:
 *   0x68, length 5, command 1, confirmation (4 bytes, big endian), XOR of
 *   length..confirmation.
 * The consumer decides whether the confirmation releases the factory hold.
 * Unknown bytes are ignored until the next 0x68.
 */
#define ND_MAINTENANCE_START 0x68u
#define ND_MAINTENANCE_LENGTH 5u
#define ND_MAINTENANCE_RELEASE 0x01u

typedef int (*nd_maintenance_release_t)(void *ctx, uint32_t confirmation);

typedef struct {
    uint8_t state;
    uint8_t checksum;
    uint32_t confirmation;
} nd_maintenance_t;

void nd_maintenance_init(nd_maintenance_t *maintenance);

/* Feeds one byte. Returns 1 when a complete release frame was accepted, 0
 * while a frame is incomplete, and -1 when a frame or the release failed. */
int nd_maintenance_feed(nd_maintenance_t *maintenance, uint8_t byte,
                        nd_maintenance_release_t release, void *ctx);

/* Drops a partial frame. The consumer calls this after each drain of the
 * optical FIFO, which makes one drain the inter-byte timeout: late bytes
 * cannot complete a prefix left over from an earlier cycle. */
void nd_maintenance_abort(nd_maintenance_t *maintenance);

#endif
