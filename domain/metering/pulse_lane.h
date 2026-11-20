#ifndef ND_DOMAIN_METERING_PULSE_LANE_H
#define ND_DOMAIN_METERING_PULSE_LANE_H

#include <stdint.h>

#include "domain/metering/energy.h"

/* One encoded lane per direction and tariff. The store treats a lane as one
 * byte; the timeline is reconstructed by the tariff marker protocol. */
#define ND_PULSE_DIRECTIONS 2u
#define ND_PULSE_TARIFFS 12u
#define ND_PULSE_LANES (ND_PULSE_DIRECTIONS * ND_PULSE_TARIFFS)

/* Same slot fold as the energy book: total and out-of-range tariffs are
 * metered in tariff 1. */
static inline uint8_t nd_energy_pulse_tariff(uint8_t tariff)
{
    return (tariff == ND_TARIFF_TOTAL || tariff >= ND_TARIFF_SLOTS) ? 1u
                                                                   : tariff;
}

static inline uint8_t nd_energy_pulse_lane(nd_energy_direction_t direction,
                                           uint8_t tariff)
{
    uint8_t slot = (uint8_t)(nd_energy_pulse_tariff(tariff) - 1u);
    uint8_t side = direction == ND_ENERGY_REVERSE ? 1u : 0u;

    return (uint8_t)(slot * ND_PULSE_DIRECTIONS + side);
}

static inline nd_energy_direction_t nd_energy_pulse_lane_direction(uint8_t lane)
{
    return (lane % ND_PULSE_DIRECTIONS) != 0u ? ND_ENERGY_REVERSE
                                              : ND_ENERGY_FORWARD;
}

static inline uint8_t nd_energy_pulse_lane_tariff(uint8_t lane)
{
    return (uint8_t)(lane / ND_PULSE_DIRECTIONS + 1u);
}

#endif
