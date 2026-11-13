#ifndef ND_DOMAIN_METERING_MEASUREMENT_AVERAGE_H
#define ND_DOMAIN_METERING_MEASUREMENT_AVERAGE_H

#include <stdint.h>

#include "domain/metering/measurement.h"

/*
 * Fixed-point averaging for IDIS instantaneous/average values and PQ profile
 * captures. The caller supplies already-scaled samples; the accumulator keeps
 * the sample scaler/unit so the result is directly usable.
 */
typedef struct {
    int64_t sum;
    uint32_t count;
    int8_t scaler;
    nd_measurement_unit_t unit;
    uint8_t phase;
    uint8_t status;
} nd_measurement_average_t;

void nd_measurement_average_init(nd_measurement_average_t *average);
int nd_measurement_average_add(nd_measurement_average_t *average,
                               const nd_measurement_value_t *value);
int nd_measurement_average_value(const nd_measurement_average_t *average,
                                 nd_measurement_value_t *out);
int nd_measurement_average_reset(nd_measurement_average_t *average);

#endif
