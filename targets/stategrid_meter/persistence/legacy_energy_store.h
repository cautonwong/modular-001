#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_STORE_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_STORE_H

#include <stdint.h>

#include "domain/common/store.h"
#include "domain/metering/metering.h"
#include "domain/metering/pulse_store.h"

int legacy_energy_load(const nd_store_t *store, uint32_t block_offset,
                       metering_state_t *state);

int legacy_energy_save(const nd_store_t *store, uint32_t block_offset,
                       const metering_state_t *state);

/*
 * Legacy adoption sources. `poweroff` is the 172-byte record the old firmware
 * wrote at power-off (EC1); `flag` is the ec_save_success word that says that
 * record was not consumed yet. `backup` reads the EC2 items by their resource
 * index (the parameter manager serves `backup_energy` and `backup_month`).
 * A NULL backup means the caller has no parameter-block access.
 */
typedef struct {
    const nd_store_t *eeprom;
    uint32_t poweroff_offset;
    uint32_t flag_offset;
    const nd_store_t *backup;
    uint32_t backup_energy;
    uint32_t backup_month;
} legacy_energy_sources_t;

/* Restore from the snapshot store. Returns 0 on a clean restore, 1 when a
 * torn-region recovery used the older EC1 energy half because the EC2 energy
 * read failed persistently (the caller must raise storage_fault; the total may
 * regress to the last power-off value), and -1 when the store is unknown or a
 * first-boot adoption cannot read EC2, so the caller retries without adopting.
 * EMPTY adopts the legacy sources once and commits them as generation 1.
 * ERROR (a completed scan with no valid slot) takes the same explicit recovery
 * path, because the snapshot's torn region is a known state its explicit
 * commit may replace; the legacy energy and month items are adopted
 * independently, mirroring Check_EC_recover. IO_ERROR (a device read fault)
 * returns -1 without adopting, so a transient error retries instead of writing
 * the legacy value over a real base. OK replays accepted pulses on top of the
 * base. */
int legacy_energy_restore(nd_pulse_store_t *store,
                          const legacy_energy_sources_t *sources,
                          metering_state_t *state);

/* Commit the current book as one whole 172-byte record. */
int legacy_energy_persist(nd_pulse_store_t *store,
                          const metering_state_t *state);

#endif
