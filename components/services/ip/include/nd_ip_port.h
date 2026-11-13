#ifndef ND_IP_PORT_H
#define ND_IP_PORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t address[16];
    uint16_t port;
    uint8_t family; /* 4 or 6 */
} nd_ip_endpoint_t;

typedef struct {
    void *ctx;
    int (*open)(void *ctx, const nd_ip_endpoint_t *local);
    int (*close)(void *ctx);
    int (*send)(void *ctx, const nd_ip_endpoint_t *peer,
                const uint8_t *data, size_t length);
    int (*recv)(void *ctx, nd_ip_endpoint_t *peer, uint8_t *data,
                size_t capacity, uint32_t timeout_ms);
} nd_ip_port_t;

#endif
