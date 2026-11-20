#ifndef ND_DOMAIN_EVENTS_EVENT_LOG_H
#define ND_DOMAIN_EVENTS_EVENT_LOG_H

#include <stdint.h>

#include "domain/events/event_record.h"

#define ND_EVENT_LOG_MAX 16

/* Bytes an embedder reserves for the aggregate; the layout stays private. */
#define ND_EVENT_LOG_STORAGE_BYTES                                        \
    (((ND_EVENT_LOG_MAX * sizeof(nd_event_record_t)                       \
       + ND_EVENT_KIND_COUNT * sizeof(uint32_t) + 2u * sizeof(uint16_t))   \
      + sizeof(uint32_t) - 1u)                                            \
     / sizeof(uint32_t) * sizeof(uint32_t))

typedef struct nd_event_log nd_event_log_t;

typedef union {
    uint8_t bytes[ND_EVENT_LOG_STORAGE_BYTES];
    uint32_t align;
} nd_event_log_storage_t;

static inline nd_event_log_t *nd_event_log_of(nd_event_log_storage_t *storage)
{
    return (nd_event_log_t *)(void *)storage;
}

static inline const nd_event_log_t *
nd_event_log_of_const(const nd_event_log_storage_t *storage)
{
    return (const nd_event_log_t *)(const void *)storage;
}

void nd_event_log_init(nd_event_log_t *log);
int nd_event_log_append(nd_event_log_t *log,
                        const nd_event_record_t *record);
uint16_t nd_event_log_count(const nd_event_log_t *log);
uint16_t nd_event_log_head(const nd_event_log_t *log);
const nd_event_record_t *nd_event_log_at(const nd_event_log_t *log,
                                         uint16_t index);
const nd_event_record_t *nd_event_log_latest(const nd_event_log_t *log);
uint32_t nd_event_log_counter(const nd_event_log_t *log,
                              nd_event_kind_t kind);
int nd_event_log_reset(nd_event_log_t *log);
/* Copies the full physical entry array; count and head travel separately. */
void nd_event_log_export(const nd_event_log_t *log, nd_event_record_t *entries);
int nd_event_log_import(nd_event_log_t *log, const nd_event_record_t *entries,
                        uint16_t count, uint16_t head);

#endif
