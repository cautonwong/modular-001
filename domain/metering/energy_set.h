#ifndef ND_DOMAIN_METERING_ENERGY_SET_H
#define ND_DOMAIN_METERING_ENERGY_SET_H

#include "domain/metering/energy.h"

/*
 * Physical energy channels owned by the domain. Derived values such as
 * reactive import/export or active combined totals belong to the mapping
 * layer; they are combinations of these channels.
 */
typedef struct {
    nd_energy_channel_t active_import;
    nd_energy_channel_t active_export;
    nd_energy_channel_t reactive_q1;
    nd_energy_channel_t reactive_q2;
    nd_energy_channel_t reactive_q3;
    nd_energy_channel_t reactive_q4;
    nd_energy_channel_t apparent_import;
    nd_energy_channel_t apparent_export;
    uint32_t meter_const; /* pulses per kWh, same convention as energy.h */
} nd_energy_set_t;

typedef enum {
    ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
    ND_ENERGY_CHANNEL_ACTIVE_EXPORT,
    ND_ENERGY_CHANNEL_REACTIVE_Q1,
    ND_ENERGY_CHANNEL_REACTIVE_Q2,
    ND_ENERGY_CHANNEL_REACTIVE_Q3,
    ND_ENERGY_CHANNEL_REACTIVE_Q4,
    ND_ENERGY_CHANNEL_APPARENT_IMPORT,
    ND_ENERGY_CHANNEL_APPARENT_EXPORT
} nd_energy_channel_id_t;

void nd_energy_set_init(nd_energy_set_t *set, uint32_t meter_const);
int nd_energy_set_add_pulses(nd_energy_set_t *set,
                             nd_energy_channel_id_t channel, uint8_t tariff,
                             uint32_t pulses);
int64_t nd_energy_set_read(const nd_energy_set_t *set,
                           nd_energy_channel_id_t channel, uint8_t tariff,
                           nd_energy_resolution_t resolution);
/* Base unit: 1e-4 kWh. Presentation scaling belongs to the target. */
/* Zero every tariff slot of one physical channel (register reset). */
void nd_energy_set_reset(nd_energy_set_t *set, nd_energy_channel_id_t channel);

int64_t nd_energy_set_read_raw(const nd_energy_set_t *set,
                               nd_energy_channel_id_t channel, uint8_t tariff);

#endif
