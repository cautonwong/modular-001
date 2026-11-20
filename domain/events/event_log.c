#include "domain/events/event_log.h"

#include <string.h>

struct nd_event_log {
    nd_event_record_t entries[ND_EVENT_LOG_MAX];
    uint32_t counters[ND_EVENT_KIND_COUNT];
    uint16_t head;
    uint16_t count;
};

_Static_assert(sizeof(struct nd_event_log) <= ND_EVENT_LOG_STORAGE_BYTES,
               "nd_event_log_storage_t is too small");
_Static_assert(_Alignof(struct nd_event_log) <= _Alignof(nd_event_log_storage_t),
               "nd_event_log_storage_t is under-aligned");

void nd_event_log_init(nd_event_log_t *log)
{
    if (log != NULL) {
        memset(log, 0, sizeof(*log));
    }
}

int nd_event_log_append(nd_event_log_t *log,
                        const nd_event_record_t *record)
{
    if (log == NULL || record == NULL || record->kind >= ND_EVENT_KIND_COUNT) {
        return -1;
    }
    log->entries[log->head] = *record;
    log->head = (uint16_t)((log->head + 1) % ND_EVENT_LOG_MAX);
    if (log->count < ND_EVENT_LOG_MAX) {
        log->count++;
    }
    log->counters[record->kind]++;
    return 0;
}

uint16_t nd_event_log_count(const nd_event_log_t *log)
{
    return log == NULL ? 0 : log->count;
}

uint16_t nd_event_log_head(const nd_event_log_t *log)
{
    return log == NULL ? 0 : log->head;
}

const nd_event_record_t *nd_event_log_at(const nd_event_log_t *log,
                                         uint16_t index)
{
    if (log == NULL || index >= log->count) {
        return NULL;
    }
    uint16_t oldest = (uint16_t)((log->head + ND_EVENT_LOG_MAX - log->count)
                                 % ND_EVENT_LOG_MAX);
    return &log->entries[(oldest + index) % ND_EVENT_LOG_MAX];
}

const nd_event_record_t *nd_event_log_latest(const nd_event_log_t *log)
{
    if (log == NULL || log->count == 0) {
        return NULL;
    }
    return nd_event_log_at(log, (uint16_t)(log->count - 1));
}

uint32_t nd_event_log_counter(const nd_event_log_t *log,
                              nd_event_kind_t kind)
{
    if (log == NULL || kind >= ND_EVENT_KIND_COUNT) {
        return 0;
    }
    return log->counters[kind];
}

int nd_event_log_reset(nd_event_log_t *log)
{
    if (log == NULL) {
        return -1;
    }
    memset(log, 0, sizeof(*log));
    return 0;
}

void nd_event_log_export(const nd_event_log_t *log, nd_event_record_t *entries)
{
    if (log == NULL || entries == NULL) {
        return;
    }
    memcpy(entries, log->entries, sizeof(log->entries));
}

int nd_event_log_import(nd_event_log_t *log, const nd_event_record_t *entries,
                        uint16_t count, uint16_t head)
{
    if (log == NULL || entries == NULL) {
        return -1;
    }
    log->count = count <= ND_EVENT_LOG_MAX ? count : ND_EVENT_LOG_MAX;
    log->head = head < ND_EVENT_LOG_MAX ? head : 0;
    memcpy(log->entries, entries, sizeof(log->entries));
    return 0;
}
