#ifndef ND_PROFILE_STORE_H
#define ND_PROFILE_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/calendar.h"
#include "domain/common/store.h"

/* init returns this when the region holds a valid pre-v4 image. The store is
 * then held: appends are refused so the old image is never overwritten. The
 * caller runs nd_idis_profiles_release_legacy or nd_profile_store_reset. */
#define ND_PROFILE_STORE_LEGACY_IMAGE (-2)
#define ND_PROFILE_STORE_LEGACY_NONE 0xFFFFFFFFu

/* Called after every store read during init. The boot scan reads hundreds of
 * KiB over bit-bang SPI, so the target feeds its watchdog here. */
typedef struct {
    void *ctx;
    void (*feed)(void *ctx);
} nd_profile_watchdog_t;

/* Init policy that is not part of the persisted layout. */
typedef struct {
    /* Pre-v4 base, probed only while the v4 ring is empty. */
    uint32_t legacy_base_offset; /* ND_PROFILE_STORE_LEGACY_NONE = no probe */
    const nd_profile_watchdog_t *watchdog; /* NULL = no feed */
} nd_profile_store_opts_t;

typedef struct {
    nd_store_t *store;
    uint32_t base_offset;   /* sector-aligned; slot 0 starts here */
    uint16_t capacity;      /* live records kept */
    uint16_t slot_size;     /* bytes per slot, at most 256 */
    uint16_t sector_slots;  /* slots per erase sector; 0 = no sector erase */
    uint16_t head;          /* next write slot, 0 .. phys_slots-1 */
    uint16_t entries;       /* live records, 0 .. capacity */
    uint16_t next_sequence; /* sequence of the next record */
    nd_profile_watchdog_t watchdog;
    uint8_t legacy; /* pre-v4 image retained; append is refused until reset */
} nd_profile_store_t;

/* Ring position frozen for one response. read_at derives the slot from these
 * and rejects a record whose stored sequence moved after the snapshot. A GET
 * takes one for its count and write passes; class-7 methods 3 and 4 take one
 * for their row reads. */
typedef struct {
    uint16_t head;
    uint16_t entries;
    uint16_t next_sequence;
} nd_profile_store_snapshot_t;

int nd_profile_store_init(nd_profile_store_t *profile, nd_store_t *store,
                          uint32_t base_offset, uint16_t capacity,
                          uint16_t slot_size, uint32_t sector_size,
                          const nd_profile_store_opts_t *opts);
int nd_profile_store_append(nd_profile_store_t *profile,
                            const nd_datetime_t *time, uint8_t status,
                            const uint8_t *record, size_t length);
void nd_profile_store_snapshot(const nd_profile_store_t *profile,
                               nd_profile_store_snapshot_t *snapshot);
int nd_profile_store_read_at(const nd_profile_store_t *profile,
                             const nd_profile_store_snapshot_t *snapshot,
                             uint32_t index, uint8_t *record, size_t capacity,
                             size_t *length);
int nd_profile_store_read(const nd_profile_store_t *profile, uint32_t index,
                          uint8_t *record, size_t capacity, size_t *length);
/* Resets this ring and clears this store's hold. Releasing one store while
 * another still holds lets this ring's next sector erase destroy an
 * overlapping pre-v4 image; object-model callers use nd_idis_profiles_reset,
 * and only nd_idis_profiles_release_legacy discards all holds. */
int nd_profile_store_reset(nd_profile_store_t *profile);
/* Fail closed: refuse appends so a retained pre-v4 image cannot be
 * overwritten. Cleared only by a successful reset. */
void nd_profile_store_hold(nd_profile_store_t *profile);
uint16_t nd_profile_store_entries(const nd_profile_store_t *profile);
uint16_t nd_profile_store_capacity(const nd_profile_store_t *profile);

#endif
