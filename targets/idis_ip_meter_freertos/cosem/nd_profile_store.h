#ifndef ND_PROFILE_STORE_H
#define ND_PROFILE_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/calendar.h"
#include "domain/common/store.h"

typedef struct {
    nd_store_t *store;
    uint32_t base_offset;
    uint16_t capacity;
    uint16_t slot_size;
    uint16_t head;
    uint16_t entries;
    uint16_t generation;
    uint16_t next_sequence;
    uint16_t sector_slots;
    uint8_t appends_since_meta;
} nd_profile_store_t;

int nd_profile_store_init(nd_profile_store_t *profile, nd_store_t *store,
                          uint32_t base_offset, uint16_t capacity,
                          uint16_t slot_size);
int nd_profile_store_append(nd_profile_store_t *profile,
                            const nd_datetime_t *time, uint8_t status,
                            const uint8_t *record, size_t length);
int nd_profile_store_read(const nd_profile_store_t *profile, uint32_t index,
                          uint8_t *record, size_t capacity, size_t *length);
int nd_profile_store_reset(nd_profile_store_t *profile);
int nd_profile_store_sync(nd_profile_store_t *profile);
void nd_profile_store_set_sector_slots(nd_profile_store_t *profile,
                                       uint16_t slots);
uint16_t nd_profile_store_entries(const nd_profile_store_t *profile);
uint16_t nd_profile_store_capacity(const nd_profile_store_t *profile);

#endif
