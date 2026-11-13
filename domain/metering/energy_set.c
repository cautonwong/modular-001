#include "domain/metering/energy_set.h"

#include <string.h>

static nd_energy_channel_t *channel_for_add(nd_energy_set_t *set,
                                            nd_energy_channel_id_t channel)
{
    switch (channel) {
    case ND_ENERGY_CHANNEL_ACTIVE_IMPORT:
        return &set->active_import;
    case ND_ENERGY_CHANNEL_ACTIVE_EXPORT:
        return &set->active_export;
    case ND_ENERGY_CHANNEL_REACTIVE_Q1:
        return &set->reactive_q1;
    case ND_ENERGY_CHANNEL_REACTIVE_Q2:
        return &set->reactive_q2;
    case ND_ENERGY_CHANNEL_REACTIVE_Q3:
        return &set->reactive_q3;
    case ND_ENERGY_CHANNEL_REACTIVE_Q4:
        return &set->reactive_q4;
    case ND_ENERGY_CHANNEL_APPARENT_IMPORT:
        return &set->apparent_import;
    case ND_ENERGY_CHANNEL_APPARENT_EXPORT:
        return &set->apparent_export;
    default:
        return NULL;
    }
}

static const nd_energy_channel_t *channel_for_read(
    const nd_energy_set_t *set, nd_energy_channel_id_t channel)
{
    switch (channel) {
    case ND_ENERGY_CHANNEL_ACTIVE_IMPORT:
        return &set->active_import;
    case ND_ENERGY_CHANNEL_ACTIVE_EXPORT:
        return &set->active_export;
    case ND_ENERGY_CHANNEL_REACTIVE_Q1:
        return &set->reactive_q1;
    case ND_ENERGY_CHANNEL_REACTIVE_Q2:
        return &set->reactive_q2;
    case ND_ENERGY_CHANNEL_REACTIVE_Q3:
        return &set->reactive_q3;
    case ND_ENERGY_CHANNEL_REACTIVE_Q4:
        return &set->reactive_q4;
    case ND_ENERGY_CHANNEL_APPARENT_IMPORT:
        return &set->apparent_import;
    case ND_ENERGY_CHANNEL_APPARENT_EXPORT:
        return &set->apparent_export;
    default:
        return NULL;
    }
}

void nd_energy_set_init(nd_energy_set_t *set, uint32_t meter_const)
{
    if (set == NULL) {
        return;
    }
    memset(set, 0, sizeof(*set));
    set->meter_const = meter_const;
}

int nd_energy_set_add_pulses(nd_energy_set_t *set,
                             nd_energy_channel_id_t channel, uint8_t tariff,
                             uint32_t pulses)
{
    if (set == NULL) {
        return -1;
    }
    nd_energy_channel_t *target = channel_for_add(set, channel);
    if (target == NULL) {
        return -1;
    }
    nd_energy_channel_add_pulses(target, tariff, pulses, set->meter_const);
    return 0;
}

int64_t nd_energy_set_read(const nd_energy_set_t *set,
                           nd_energy_channel_id_t channel, uint8_t tariff,
                           nd_energy_resolution_t resolution)
{
    if (set == NULL) {
        return 0;
    }
    const nd_energy_channel_t *target = channel_for_read(set, channel);
    if (target == NULL) {
        return 0;
    }
    return nd_energy_channel_read(target, tariff, set->meter_const, resolution);
}

void nd_energy_set_reset(nd_energy_set_t *set, nd_energy_channel_id_t channel)
{
    if (set == NULL) {
        return;
    }
    switch (channel) {
    case ND_ENERGY_CHANNEL_ACTIVE_IMPORT:
        memset(&set->active_import, 0, sizeof(set->active_import));
        break;
    case ND_ENERGY_CHANNEL_ACTIVE_EXPORT:
        memset(&set->active_export, 0, sizeof(set->active_export));
        break;
    case ND_ENERGY_CHANNEL_REACTIVE_Q1:
        memset(&set->reactive_q1, 0, sizeof(set->reactive_q1));
        break;
    case ND_ENERGY_CHANNEL_REACTIVE_Q2:
        memset(&set->reactive_q2, 0, sizeof(set->reactive_q2));
        break;
    case ND_ENERGY_CHANNEL_REACTIVE_Q3:
        memset(&set->reactive_q3, 0, sizeof(set->reactive_q3));
        break;
    case ND_ENERGY_CHANNEL_REACTIVE_Q4:
        memset(&set->reactive_q4, 0, sizeof(set->reactive_q4));
        break;
    case ND_ENERGY_CHANNEL_APPARENT_IMPORT:
        memset(&set->apparent_import, 0, sizeof(set->apparent_import));
        break;
    case ND_ENERGY_CHANNEL_APPARENT_EXPORT:
        memset(&set->apparent_export, 0, sizeof(set->apparent_export));
        break;
    default:
        break;
    }
}

int64_t nd_energy_set_read_raw(const nd_energy_set_t *set,
                               nd_energy_channel_id_t channel,
                               uint8_t tariff)
{
    if (set == NULL) {
        return 0;
    }
    const nd_energy_channel_t *target = channel_for_read(set, channel);
    if (target == NULL) {
        return 0;
    }
    return nd_energy_channel_read_raw(target, tariff, set->meter_const);
}
