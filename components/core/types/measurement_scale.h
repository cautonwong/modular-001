#ifndef MEASUREMENT_SCALE_H
#define MEASUREMENT_SCALE_H

#include <stdint.h>

/*
 * Saturate to int32 before widening. A direct float-to-int64 cast makes GCC
 * funnel through __aeabi_f2lz, which pulls libgcc's double soft-float into
 * the Cortex-M0 image. The bounds are exact powers of two, so the float
 * comparisons are exact; corrupt calibration or raw data saturates instead of
 * becoming an unbounded integer.
 */
static inline int64_t measurement_scale_clamp(float value)
{
    if (!(value >= -2147483648.0f)) {
        return INT32_MIN;
    }
    if (value > 2147483520.0f) {
        return INT32_MAX;
    }
    return (int64_t)(int32_t)value;
}

#endif
