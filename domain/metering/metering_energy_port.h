#ifndef ND_METERING_ENERGY_PORT_H
#define ND_METERING_ENERGY_PORT_H

#include <stdint.h>
#include "domain/metering/energy.h"

/*
 * Opaque energy ledger handle. The target owns the mapping to the legacy
 * meter base block; domain and application code never see that layout.
 */
typedef struct nd_energy_ledger nd_energy_ledger_t;

nd_energy_update_t metering_energy_add(nd_energy_ledger_t *ledger,
                                       nd_energy_direction_t direction,
                                       uint32_t pulses);
void metering_energy_clear(nd_energy_ledger_t *ledger);
void metering_energy_set_total(nd_energy_ledger_t *ledger,
                               nd_energy_direction_t direction,
                               uint32_t integer);
int64_t metering_energy_read(nd_energy_ledger_t *ledger,
                             nd_energy_direction_t direction, uint8_t tariff,
                             nd_energy_resolution_t resolution);
int64_t metering_energy_read_combined(nd_energy_ledger_t *ledger,
                                      uint8_t tariff,
                                      nd_energy_resolution_t resolution);

#endif
