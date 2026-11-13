#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_energy.h"

static int channel_for_group(uint8_t group, nd_idis_energy_map_t *map)
{
    switch (group) {
    case 1:
        map->first = ND_ENERGY_CHANNEL_ACTIVE_IMPORT;
        map->unit = 30; /* kWh */
        return 0;
    case 2:
        map->first = ND_ENERGY_CHANNEL_ACTIVE_EXPORT;
        map->unit = 30;
        return 0;
    case 3:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q1;
        map->second = ND_ENERGY_CHANNEL_REACTIVE_Q2;
        map->combine = 1;
        map->unit = 31; /* kvarh */
        return 0;
    case 4:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q3;
        map->second = ND_ENERGY_CHANNEL_REACTIVE_Q4;
        map->combine = 1;
        map->unit = 31;
        return 0;
    case 5:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q1;
        map->unit = 31;
        return 0;
    case 6:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q2;
        map->unit = 31;
        return 0;
    case 7:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q3;
        map->unit = 31;
        return 0;
    case 8:
        map->first = ND_ENERGY_CHANNEL_REACTIVE_Q4;
        map->unit = 31;
        return 0;
    case 9:
        map->first = ND_ENERGY_CHANNEL_APPARENT_IMPORT;
        map->unit = 32; /* kVAh */
        return 0;
    case 10:
        map->first = ND_ENERGY_CHANNEL_APPARENT_EXPORT;
        map->unit = 32;
        return 0;
    case 15:
        map->first = ND_ENERGY_CHANNEL_ACTIVE_IMPORT;
        map->second = ND_ENERGY_CHANNEL_ACTIVE_EXPORT;
        map->combine = 1;
        map->unit = 30;
        return 0;
    case 16:
        map->first = ND_ENERGY_CHANNEL_ACTIVE_IMPORT;
        map->second = ND_ENERGY_CHANNEL_ACTIVE_EXPORT;
        map->combine = -1;
        map->unit = 30;
        return 0;
    default:
        return -1;
    }
}

int nd_idis_energy_map(const nd_obis_t *ln, nd_idis_energy_map_t *map)
{
    if (ln == NULL || map == NULL || ln->a != 1 || ln->b != 0
        || ln->d != 8 || ln->f != 255 || ln->e > 13) {
        return -1;
    }
    *map = (nd_idis_energy_map_t){0};
    if (channel_for_group(ln->c, map) != 0) {
        return -1;
    }
    map->tariff = ln->e;
    map->divisor = 10000;
    map->signed_value = ln->c == 16;
    return 0;
}
