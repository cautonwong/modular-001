#ifndef ND_DOMAIN_EVENTS_EVENT_LOG_H
#define ND_DOMAIN_EVENTS_EVENT_LOG_H

#include <stdint.h>

#include "domain/events/event_record.h"

#define ND_EVENT_LOG_MAX 16

typedef struct {
    nd_event_record_t entries[ND_EVENT_LOG_MAX];
    uint32_t counters[ND_EVENT_KIND_COUNT];
    uint16_t head;
    uint16_t count;
} nd_event_log_t;

void nd_event_log_init(nd_event_log_t *log);
int nd_event_log_append(nd_event_log_t *log,
                        const nd_event_record_t *record);
uint16_t nd_event_log_count(const nd_event_log_t *log);
const nd_event_record_t *nd_event_log_at(const nd_event_log_t *log,
                                         uint16_t index);
const nd_event_record_t *nd_event_log_latest(const nd_event_log_t *log);
uint32_t nd_event_log_counter(const nd_event_log_t *log,
                              nd_event_kind_t kind);
int nd_event_log_reset(nd_event_log_t *log);

#endif
