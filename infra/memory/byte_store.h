#ifndef ND_INFRA_MEMORY_BYTE_STORE_H
#define ND_INFRA_MEMORY_BYTE_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/store.h"

typedef struct {
    nd_store_t port;
    uint8_t *data;
    size_t size;
} nd_byte_store_t;

void nd_byte_store_init(nd_byte_store_t *store, void *data, size_t size);
nd_store_t *nd_byte_store_port(nd_byte_store_t *store);

#endif
