#ifndef ND_MEASUREMENT_PORT_H
#define ND_MEASUREMENT_PORT_H

#include "domain/metering/measurement.h"

/* Target-owned measurement channel selector, not an OBIS list. */
typedef enum {
    ND_MEASURE_VOLTAGE,
    ND_MEASURE_CURRENT,
    ND_MEASURE_ACTIVE_POWER,
    ND_MEASURE_REACTIVE_POWER,
    ND_MEASURE_APPARENT_POWER,
    ND_MEASURE_POWER_FACTOR,
    ND_MEASURE_FREQUENCY,
    ND_MEASURE_TOTAL_ACTIVE_POWER,
    ND_MEASURE_ACTIVE_POWER_EXPORT,
    ND_MEASURE_REACTIVE_POWER_IMPORT,
    ND_MEASURE_REACTIVE_POWER_EXPORT,
    ND_MEASURE_APPARENT_POWER_IMPORT,
    ND_MEASURE_APPARENT_POWER_EXPORT,
    ND_MEASURE_ACTIVE_POWER_NET,
    ND_MEASURE_CURRENT_SUM,
    ND_MEASURE_POWER_FACTOR_MIN,
    ND_MEASURE_POWER_FACTOR_AVG
} nd_measure_quantity_t;

typedef struct {
    void *ctx;
    int (*read)(void *ctx, nd_measure_quantity_t quantity, uint8_t phase,
                nd_measurement_value_t *out);
} nd_measurement_port_t;

#endif
