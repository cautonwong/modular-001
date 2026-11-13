#include "domain/metering/measurement_average.h"

#include <string.h>

void nd_measurement_average_init(nd_measurement_average_t *average)
{
    if (average != NULL) {
        memset(average, 0, sizeof(*average));
    }
}

int nd_measurement_average_add(nd_measurement_average_t *average,
                               const nd_measurement_value_t *value)
{
    if (average == NULL || value == NULL) {
        return -1;
    }
    if (average->count != 0
        && (average->scaler != value->scaler || average->unit != value->unit
            || average->phase != value->phase)) {
        return -1;
    }
    average->sum += value->value;
    average->count++;
    average->scaler = value->scaler;
    average->unit = value->unit;
    average->phase = value->phase;
    average->status |= value->status;
    return 0;
}

int nd_measurement_average_value(const nd_measurement_average_t *average,
                                 nd_measurement_value_t *out)
{
    if (average == NULL || out == NULL || average->count == 0) {
        return -1;
    }
    *out = (nd_measurement_value_t){
        .value = average->sum / average->count,
        .scaler = average->scaler,
        .unit = average->unit,
        .phase = average->phase,
        .status = average->status,
    };
    return 0;
}

int nd_measurement_average_reset(nd_measurement_average_t *average)
{
    if (average == NULL) {
        return -1;
    }
    memset(average, 0, sizeof(*average));
    return 0;
}
