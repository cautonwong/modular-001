#include "targets/idis_ip_meter_freertos/cosem/nd_idis_registry.h"

#include <string.h>

_Static_assert(sizeof(nd_idis_registry_entry_t) == ND_IDIS_REGISTRY_ENTRY_SIZE,
               "registry entry must match the blob entry size");
_Static_assert(sizeof(nd_idis_registry_t)
                   == sizeof(uint16_t)
                          + ND_IDIS_REGISTRY_MAX * ND_IDIS_REGISTRY_ENTRY_SIZE,
               "registry RAM image must stay compact");

#define ND_IDIS_REGISTRY_FORMAT 1
#define ND_IDIS_REGISTRY_CHUNK 32
#define ND_IDIS_REGISTRY_MAGIC "NDRG"

static uint16_t checksum_update(uint16_t sum, const uint8_t *data, size_t length)
{
    for (size_t index = 0; index < length; index++) {
        sum = (uint16_t)(sum + data[index]);
    }
    return sum;
}

void nd_idis_registry_clear(nd_idis_registry_t *registry)
{
    if (registry != NULL) {
        registry->count = 0;
    }
}

int nd_idis_registry_add(nd_idis_registry_t *registry, uint8_t class_id,
                         uint8_t version, const nd_obis_t *ln)
{
    if (registry == NULL || ln == NULL || class_id == 0
        || registry->count >= ND_IDIS_REGISTRY_MAX) {
        return -1;
    }
    nd_idis_registry_entry_t *entry = &registry->entries[registry->count];
    entry->class_id = class_id;
    entry->version = version;
    entry->ln = *ln;
    registry->count++;
    return 0;
}

int nd_idis_registry_remove(nd_idis_registry_t *registry, uint8_t class_id,
                            const nd_obis_t *ln)
{
    if (registry == NULL || ln == NULL) {
        return -1;
    }
    for (uint16_t index = 0; index < registry->count; index++) {
        nd_idis_registry_entry_t *entry = &registry->entries[index];
        if (entry->class_id == class_id
            && memcmp(&entry->ln, ln, sizeof(*ln)) == 0) {
            for (uint16_t move = index; move + 1u < registry->count; move++) {
                registry->entries[move] = registry->entries[move + 1];
            }
            registry->count--;
            return 0;
        }
    }
    return -1;
}

uint32_t nd_idis_registry_blob_size(uint16_t count)
{
    return ND_IDIS_REGISTRY_HEADER_SIZE + ND_IDIS_REGISTRY_TRAILER_SIZE
           + (uint32_t)count * ND_IDIS_REGISTRY_ENTRY_SIZE;
}

int nd_idis_registry_covers(const nd_store_t *store, uint32_t offset,
                            uint32_t target)
{
    if (store == NULL) {
        return 0;
    }
    uint8_t header[ND_IDIS_REGISTRY_HEADER_SIZE];
    if (nd_store_read(store, offset, header, sizeof(header)) != 0
        || memcmp(header, ND_IDIS_REGISTRY_MAGIC, 4) != 0
        || header[4] != ND_IDIS_REGISTRY_FORMAT
        || header[5] != ND_IDIS_REGISTRY_ENTRY_SIZE) {
        return 0;
    }
    uint16_t count = (uint16_t)((header[6] << 8) | header[7]);
    if (count > ND_IDIS_REGISTRY_MAX) {
        return 0;
    }
    return offset <= target
           && target < offset + nd_idis_registry_blob_size(count);
}

int nd_idis_registry_load(nd_idis_registry_t *registry, const nd_store_t *store,
                          uint32_t offset)
{
    if (registry == NULL || store == NULL) {
        return -1;
    }
    nd_idis_registry_clear(registry);

    uint8_t header[ND_IDIS_REGISTRY_HEADER_SIZE];
    if (nd_store_read(store, offset, header, sizeof(header)) != 0) {
        return -1;
    }
    if (memcmp(header, ND_IDIS_REGISTRY_MAGIC, 4) != 0
        || header[4] != ND_IDIS_REGISTRY_FORMAT
        || header[5] != ND_IDIS_REGISTRY_ENTRY_SIZE) {
        return -1;
    }
    uint16_t count = (uint16_t)((header[6] << 8) | header[7]);
    if (count > ND_IDIS_REGISTRY_MAX) {
        return -1;
    }

    uint16_t sum = checksum_update(0, header, sizeof(header));
    uint32_t position = offset + ND_IDIS_REGISTRY_HEADER_SIZE;
    uint16_t loaded = 0;
    while (loaded < count) {
        uint16_t batch = (uint16_t)(count - loaded);
        if (batch > ND_IDIS_REGISTRY_CHUNK) {
            batch = ND_IDIS_REGISTRY_CHUNK;
        }
        uint8_t chunk[ND_IDIS_REGISTRY_CHUNK * ND_IDIS_REGISTRY_ENTRY_SIZE];
        size_t length = (size_t)batch * ND_IDIS_REGISTRY_ENTRY_SIZE;
        if (nd_store_read(store, position, chunk, length) != 0) {
            return -1;
        }
        sum = checksum_update(sum, chunk, length);
        for (uint16_t index = 0; index < batch; index++) {
            const uint8_t *raw = &chunk[index * ND_IDIS_REGISTRY_ENTRY_SIZE];
            nd_idis_registry_entry_t *entry = &registry->entries[loaded + index];
            if (raw[0] == 0) {
                registry->count = 0;
                return -1;
            }
            entry->class_id = raw[0];
            entry->version = raw[1];
            memcpy(&entry->ln, raw + 2, sizeof(entry->ln));
        }
        loaded = (uint16_t)(loaded + batch);
        position += (uint32_t)length;
    }
    registry->count = count;

    uint8_t trailer[ND_IDIS_REGISTRY_TRAILER_SIZE];
    if (nd_store_read(store, position, trailer, sizeof(trailer)) != 0) {
        return -1;
    }
    uint16_t stored = (uint16_t)((trailer[0] << 8) | trailer[1]);
    if (stored != sum) {
        registry->count = 0;
        return -1;
    }
    return 0;
}
