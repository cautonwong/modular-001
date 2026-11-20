#include "domain/common/store.h"

int nd_store_read(const nd_store_t *store, uint32_t offset,
                  uint8_t *buffer, size_t length)
{
    if (store == NULL || store->read == NULL) {
        return -1;
    }
    return store->read(store->ctx, offset, buffer, length);
}
