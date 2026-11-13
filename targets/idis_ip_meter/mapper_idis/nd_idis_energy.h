#ifndef ND_IDIS_ENERGY_H
#define ND_IDIS_ENERGY_H

#include "components/protocol/dlms/include/nd_axdr.h"
#include "domain/metering/energy_set.h"

typedef struct {
    nd_energy_channel_id_t first;
    nd_energy_channel_id_t second;
    int8_t combine; /* 0 = first only, 1 = first+second, -1 = first-second */
    uint8_t tariff;
    uint16_t divisor; /* raw 1e-4 kWh -> wire unit */
    uint8_t signed_value;
    uint8_t unit;
} nd_idis_energy_map_t;

int nd_idis_energy_map(const nd_obis_t *ln, nd_idis_energy_map_t *map);

#endif
