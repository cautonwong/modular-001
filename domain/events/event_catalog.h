#ifndef ND_EVENT_CATALOG_H
#define ND_EVENT_CATALOG_H

#include <stdint.h>

/*
 * Legacy event id to record OAD mapping.
 *
 * mode is the legacy scheme value: 0-7 for freeze schemes, 10 for event
 * start and 11 for event end. It is intentionally a byte so the catalog
 * does not expose the target's enum scheme_mode.
 */
typedef struct
{
    uint32_t oad;
    uint8_t mode;
} nd_event_catalog_entry_t;

/* Returns 1 and fills entry when the id has a catalog entry, else 0. */
int nd_event_catalog_lookup(uint16_t record_id, nd_event_catalog_entry_t *entry);

#endif /* ND_EVENT_CATALOG_H */
