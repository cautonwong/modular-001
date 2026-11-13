#ifndef ND_DOMAIN_FREEZE_CAPTURE_H
#define ND_DOMAIN_FREEZE_CAPTURE_H

#include <stdint.h>

#include "domain/common/clock_value.h"

#define ND_CAPTURE_BYTES_MAX 16

typedef enum {
    ND_CAPTURE_NONE = 0,
    ND_CAPTURE_SIGNED,
    ND_CAPTURE_UNSIGNED,
    ND_CAPTURE_CLOCK,
    ND_CAPTURE_MEASUREMENT,
    ND_CAPTURE_BYTES
} nd_capture_type_t;

typedef struct nd_capture_value nd_capture_value_t;

/*
 * One captured value, in capture-object order. The target fills the variant;
 * the profile store only serializes it.
 */
struct nd_capture_value {
    nd_capture_type_t type;
    uint8_t status;
    union {
        int64_t signed_value;
        uint64_t unsigned_value;
        nd_clock_value_t clock;
        struct {
            int8_t scaler;
            uint8_t unit;
        } measurement;
        struct {
            uint8_t len;
            uint8_t data[ND_CAPTURE_BYTES_MAX];
        } bytes;
    } value;
};

typedef struct {
    nd_clock_value_t capture_time;
    uint8_t status;
    uint8_t count;
    const nd_capture_value_t *values;
} nd_capture_snapshot_t;

#endif
