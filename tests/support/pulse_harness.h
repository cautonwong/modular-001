#ifndef ND_TESTS_SUPPORT_PULSE_HARNESS_H
#define ND_TESTS_SUPPORT_PULSE_HARNESS_H

#include <stdint.h>

#include "domain/metering/energy_set.h"
#include "domain/metering/pulse_store.h"

/* The consumer half of the pulse tests: a 1:1 pulse count (meter_const
 * 10000 makes one pulse one raw unit) and boot recovery from base + replay. */
void pulse_set_init(nd_energy_set_t *set);
int pulse_recover(nd_pulse_store_t *store, nd_energy_set_t *set);
void pulse_add(nd_energy_set_t *set, nd_energy_direction_t direction,
               uint8_t tariff);
uint32_t pulse_total(const nd_energy_set_t *set);
uint32_t pulse_slot_count(const nd_energy_set_t *set,
                          nd_energy_direction_t direction, uint8_t tariff);

#endif
