#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_STORE_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_STORE_H

#include <stdint.h>

#include "domain/common/store.h"
#include "domain/metering/metering.h"

int legacy_energy_load(const nd_store_t *store, uint32_t block_offset,
                       metering_state_t *state);

int legacy_energy_save(const nd_store_t *store, uint32_t block_offset,
                       const metering_state_t *state);

#endif
