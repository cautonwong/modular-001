#ifndef ND_IDIS_EVENT_DESC_H
#define ND_IDIS_EVENT_DESC_H

#include "domain/events/event_record.h"

#define ND_EVENT_SOURCE_OTHER 0
#define ND_EVENT_SOURCE_SUPPLY 1
#define ND_EVENT_SOURCE_COVER 2
#define ND_EVENT_SOURCE_CLOCK 3
#define ND_EVENT_SOURCE_BATTERY 4
#define ND_EVENT_SOURCE_MEMORY 5
#define ND_EVENT_SOURCE_LOAD 6
#define ND_EVENT_SOURCE_NETWORK 7
#define ND_EVENT_SOURCE_MBUS 8
#define ND_EVENT_SOURCE_SECURITY 9
#define ND_EVENT_SOURCE_VOLTAGE 10
#define ND_EVENT_SOURCE_TARIFF 11
#define ND_EVENT_SOURCE_DISCONNECT 12
#define ND_EVENT_SOURCE_FIRMWARE 13

#define ND_EVENT_SEVERITY_INFO 0
#define ND_EVENT_SEVERITY_WARN 1
#define ND_EVENT_SEVERITY_ERROR 2
#define ND_EVENT_SEVERITY_CRITICAL 3

typedef struct {
    uint16_t code;
    nd_event_kind_t kind;
    uint8_t source;
    uint8_t severity;
} nd_idis_event_descriptor_t;

const nd_idis_event_descriptor_t *nd_idis_event_describe(uint16_t code);

#endif
