#ifndef ND_MBUS_H
#define ND_MBUS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    int (*send)(void *ctx, const uint8_t *frame, size_t length);
    int (*recv)(void *ctx, uint8_t *frame, size_t capacity,
                uint32_t timeout_ms);
} nd_mbus_link_t;

typedef struct {
    uint8_t primary_address;
    uint32_t identification_number;
    uint16_t manufacturer_id;
    uint8_t version;
    uint8_t device_type;
} nd_mbus_identity_t;

#endif
