#ifndef ND_DOMAIN_EVENTS_EVENT_RECORD_H
#define ND_DOMAIN_EVENTS_EVENT_RECORD_H

#include <stdint.h>

#include "domain/common/calendar.h"

/*
 * Semantic event emitted by a domain detector. IDIS event codes and alarm
 * bits are mapped in the target; the domain never stores either.
 */
typedef enum {
    ND_EVENT_NONE = 0,
    ND_EVENT_POWER_OFF,
    ND_EVENT_POWER_UP,
    ND_EVENT_COVER_OPEN,
    ND_EVENT_OVER_CURRENT,
    ND_EVENT_NEUTRAL_CURRENT,
    ND_EVENT_CLOCK,
    ND_EVENT_BATTERY,
    ND_EVENT_ERROR,
    ND_EVENT_DISCONNECT,
    ND_EVENT_VOLTAGE,
    ND_EVENT_FRAUD,
    ND_EVENT_COMMUNICATION,
    ND_EVENT_LOAD_CONTROL,
    ND_EVENT_MBUS,
    ND_EVENT_VOLTAGE_SAG,
    ND_EVENT_VOLTAGE_SWELL,
    ND_EVENT_VOLTAGE_MISSING,
    ND_EVENT_FREQUENCY_LOW,
    ND_EVENT_FREQUENCY_HIGH,
    ND_EVENT_VOLTAGE_UNBALANCE,
    ND_EVENT_GENERIC,
    ND_EVENT_KIND_COUNT
} nd_event_kind_t;

typedef struct {
    nd_event_kind_t kind;
    nd_datetime_t time;
    uint8_t source;
    uint8_t severity;
    uint32_t duration_seconds;
    int32_t value;
} nd_event_record_t;

#endif
