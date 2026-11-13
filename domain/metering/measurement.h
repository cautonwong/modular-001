#ifndef ND_DOMAIN_METERING_MEASUREMENT_H
#define ND_DOMAIN_METERING_MEASUREMENT_H

#include <stdint.h>

/*
 * Semantic measurement values. Hardware adapters fill these; COSEM classes
 * only map an OBIS to a quantity/phase and encode the value.
 */

typedef enum {
    ND_MEASUREMENT_UNIT_NONE = 0,
    ND_MEASUREMENT_UNIT_VOLT,
    ND_MEASUREMENT_UNIT_AMPERE,
    ND_MEASUREMENT_UNIT_WATT,
    ND_MEASUREMENT_UNIT_VAR,
    ND_MEASUREMENT_UNIT_VA,
    ND_MEASUREMENT_UNIT_HERTZ
} nd_measurement_unit_t;

#define ND_MEASURE_PHASE_TOTAL 0
#define ND_MEASURE_PHASE_L1 1
#define ND_MEASURE_PHASE_L2 2
#define ND_MEASURE_PHASE_L3 3

typedef struct {
    int64_t value;
    int8_t scaler;
    nd_measurement_unit_t unit;
    uint8_t phase;
    uint8_t status;
} nd_measurement_value_t;

#endif
