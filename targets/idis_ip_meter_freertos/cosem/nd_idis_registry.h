#ifndef ND_IDIS_REGISTRY_H
#define ND_IDIS_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#include "components/protocol/dlms/include/nd_axdr.h"
#include "domain/common/store.h"

/*
 * External object registry, path C in docs/idis-object-registry.md.
 *
 * The blob lives in non-volatile storage and is loaded into RAM at boot:
 *
 *   0..3   "NDRG"
 *   4      format version (1)
 *   5      entry size (8)
 *   6..7   entry count, big-endian
 *   8..    count * { class_id u8, version u8, logical name 6 }
 *   last 2 checksum of every preceding byte, big-endian
 */
#define ND_IDIS_REGISTRY_MAX 360
#define ND_IDIS_REGISTRY_ENTRY_SIZE 8
#define ND_IDIS_REGISTRY_HEADER_SIZE 8
#define ND_IDIS_REGISTRY_TRAILER_SIZE 2
#define ND_IDIS_REGISTRY_EEPROM_OFFSET 55296u

_Static_assert(ND_IDIS_REGISTRY_MAX <= UINT16_MAX,
               "registry count must fit the blob header");
_Static_assert(ND_IDIS_REGISTRY_EEPROM_OFFSET + ND_IDIS_REGISTRY_HEADER_SIZE
                   + ND_IDIS_REGISTRY_MAX * ND_IDIS_REGISTRY_ENTRY_SIZE
                   + ND_IDIS_REGISTRY_TRAILER_SIZE
               <= 0x10000u,
               "registry must fit a 16-bit EEPROM address space");

typedef struct {
    uint8_t class_id;
    uint8_t version;
    nd_obis_t ln;
} nd_idis_registry_entry_t;

typedef struct {
    uint16_t count;
    nd_idis_registry_entry_t entries[ND_IDIS_REGISTRY_MAX];
} nd_idis_registry_t;

void nd_idis_registry_clear(nd_idis_registry_t *registry);
int nd_idis_registry_add(nd_idis_registry_t *registry, uint8_t class_id,
                         uint8_t version, const nd_obis_t *ln);
int nd_idis_registry_remove(nd_idis_registry_t *registry, uint8_t class_id,
                            const nd_obis_t *ln);

/* Returns 0 and fills the registry, or -1 after clearing it when the blob is
 * absent, truncated, malformed or fails its checksum. */
int nd_idis_registry_load(nd_idis_registry_t *registry, const nd_store_t *store,
                          uint32_t offset);

/* Returns 1 when a well-formed blob header at offset describes a blob whose
 * written bytes, header and trailer included, reach target. Reads the header
 * only, so a corrupt tail does not change the answer. */
int nd_idis_registry_covers(const nd_store_t *store, uint32_t offset,
                            uint32_t target);

uint32_t nd_idis_registry_blob_size(uint16_t count);

#endif
