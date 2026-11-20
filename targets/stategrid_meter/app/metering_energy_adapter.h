#ifndef ND_TARGET_STATEGRID_METERING_ENERGY_ADAPTER_H
#define ND_TARGET_STATEGRID_METERING_ENERGY_ADAPTER_H

#include "targets/stategrid_meter/app/stategrid_context.h"

#include "domain/metering/metering_energy_port.h"
struct mbase_control_block_t;

/*
 * Product defaults for the energy snapshot. The region was measured free at
 * boot (allocator high water 0xAF0A) and is above it; the boot guard in the
 * composition root fails when a future block crosses 0xC000. Two 4-byte
 * aligned slots of 172-byte blob + 12-byte header + CRC follow from the
 * offset. A product that has a different EEPROM budget overrides these in its
 * composition root, not in the persisted layout.
 */
#define STG_ENERGY_SNAPSHOT_OFFSET 0xC000u
#define STG_ENERGY_SNAPSHOT_BLOB_BYTES 172u
#define STG_ENERGY_LEGACY_BLOCK_OFFSET 0x450Au
#define STG_ENERGY_LEGACY_RECORD_OFFSET (STG_ENERGY_LEGACY_BLOCK_OFFSET + 3u)

/*
 * Target-side constructors for the opaque ledger. They let legacy callers
 * keep their existing instance or meter-base handle while the port itself
 * exposes only nd_energy_ledger_t. The store is bound once by the composition
 * root through the instance; from_mbase keeps that binding for read-only
 * callers.
 */
nd_energy_ledger_t *metering_energy_ledger_from_instance(
    stategrid_context_t *instance);
nd_energy_ledger_t *metering_energy_ledger_from_mbase(
    struct mbase_control_block_t *mbase_cb);

/* Commit the book before a power-off save. Returns 0 when durable or when no
 * store is bound; -1 on a storage fault. */
int metering_energy_commit(stategrid_context_t *instance);

/* 1 while the durable base has not been restored. The legacy mirror must not
 * be overwritten in that state. */
int metering_energy_restore_pending(stategrid_context_t *instance);

/* The storage fault latches until the device-error reporter observes it:
 * Energy_Meter runs before report_2015state_judge in the same second, so a
 * recovery must not erase the fault before it is reported. */
void metering_energy_storage_fault_reported(stategrid_context_t *instance);

/* Host tests drop the in-process book to model a reset; firmware gets the same
 * effect from startup clearing file scope. */
void metering_energy_adapter_reset(void);

#endif
