#ifndef ND_TARGET_STATEGRID_METERING_ENERGY_ADAPTER_H
#define ND_TARGET_STATEGRID_METERING_ENERGY_ADAPTER_H

#include "targets/stategrid_meter/app/stategrid_context.h"

#include "domain/metering/metering_energy_port.h"
struct mbase_control_block_t;

/*
 * Target-side constructors for the opaque ledger. They let legacy callers
 * keep their existing instance or meter-base handle while the port itself
 * exposes only nd_energy_ledger_t.
 */
nd_energy_ledger_t *metering_energy_ledger_from_instance(
    stategrid_context_t *instance);
nd_energy_ledger_t *metering_energy_ledger_from_mbase(
    struct mbase_control_block_t *mbase_cb);

#endif
