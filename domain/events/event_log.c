#include "domain/events/event_log.h"

#include <string.h>

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
