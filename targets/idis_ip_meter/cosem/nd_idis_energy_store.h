#ifndef ND_IDIS_ENERGY_STORE_H
#define ND_IDIS_ENERGY_STORE_H

#include "domain/common/store.h"
#include "domain/metering/energy_set.h"

int nd_idis_energy_save(const nd_energy_set_t *set, nd_store_t *store,
                        uint32_t offset);
int nd_idis_energy_load(nd_energy_set_t *set, nd_store_t *store,
                        uint32_t offset);

#endif
