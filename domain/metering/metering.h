#ifndef ND_DOMAIN_METERING_METERING_H
#define ND_DOMAIN_METERING_METERING_H

#include <stdint.h>

#include "domain/common/allocator.h"
#include "domain/metering/energy.h"

/*
 * Aggregate root of the metering bounded context. The definition stays in
 * metering.c. Callers hold an opaque handle and never touch its layout.
 */
typedef struct metering_state metering_state_t;

typedef struct {
    uint32_t meter_const;
    uint8_t cal_word;
    uint32_t save_step;
} metering_config_t;

typedef struct {
    nd_energy_channel_t forward;
    nd_energy_channel_t reverse;
    nd_energy_amount_t month;
} nd_metering_snapshot_t;

metering_state_t *metering_create(nd_allocator_t *allocator,
                                  const metering_config_t *config);

void metering_destroy(metering_state_t *state, nd_allocator_t *allocator);

nd_energy_update_t metering_add_pulses(metering_state_t *state,
                                       nd_energy_direction_t direction,
                                       uint8_t tariff, uint32_t pulses);

int64_t metering_read(const metering_state_t *state,
                      nd_energy_direction_t direction, uint8_t tariff,
                      nd_energy_resolution_t resolution);

int64_t metering_read_combined(const metering_state_t *state, uint8_t tariff,
                               nd_energy_resolution_t resolution);

void metering_clear(metering_state_t *state);

void metering_set_total(metering_state_t *state, nd_energy_direction_t direction,
                        uint32_t integer);

void metering_export(const metering_state_t *state,
                     nd_metering_snapshot_t *snapshot);

void metering_import(metering_state_t *state,
                     const nd_metering_snapshot_t *snapshot);

#endif
