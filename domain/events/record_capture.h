#ifndef ND_DOMAIN_EVENTS_RECORD_CAPTURE_H
#define ND_DOMAIN_EVENTS_RECORD_CAPTURE_H

#include <stdint.h>

typedef enum {
    ND_RECORD_CAPTURE_SET_TIME = 0,
    ND_RECORD_CAPTURE_PROGRAM,
    ND_RECORD_CAPTURE_DEFAULT
} nd_record_capture_kind_t;

typedef enum {
    ND_RECORD_KIND_DEFAULT = 0,
    ND_RECORD_KIND_METER_WR,
    ND_RECORD_KIND_METER_WR_645,
    ND_RECORD_KIND_METER_WR_R,
    ND_RECORD_KIND_METER_WR_645_R,
    ND_RECORD_KIND_SET_TIME
} nd_record_kind_t;

#define ND_RECORD_CAPTURE_START_MODE 10
#define ND_RECORD_CAPTURE_END_MODE 11

uint32_t nd_record_capture_oad(nd_record_capture_kind_t kind, uint32_t oad,
                               uint8_t program_first_is_time_set);

uint8_t nd_record_capture_visible(uint8_t mode, uint32_t oad,
                                  nd_record_kind_t record_kind);

#endif
