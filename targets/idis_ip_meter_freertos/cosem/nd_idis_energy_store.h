#ifndef ND_IDIS_ENERGY_STORE_H
#define ND_IDIS_ENERGY_STORE_H

#include <stdint.h>

#include "domain/common/store.h"
#include "domain/metering/energy.h"
#include "domain/metering/energy_set.h"
#include "domain/metering/pulse_store.h"

/* EEPROM map for the two IDIS targets: adjacent 1 KiB snapshot slots. */
#define ND_IDIS_ENERGY_BASE_OFFSET 0xE800u
#define ND_IDIS_ENERGY_BASE_ALT_OFFSET 0xEC00u
#define ND_IDIS_ENERGY_BASE_BYTES 1024u

/* The target adapter between the energy book and the pulse store port. The
 * book is the base blob; the journal holds the pulses since the last commit.
 * A NULL store keeps the pre-port whole-record behavior for products that
 * cannot afford the journal engine. */

int nd_idis_energy_load(nd_energy_set_t *set, nd_pulse_store_t *store,
                        nd_store_t *legacy, uint32_t legacy_offset);

/* Periodic save. An accepted pulse is recorded in the journal, so the journal
 * path writes nothing; the legacy path still writes when the record differs. */
int nd_idis_energy_save(const nd_energy_set_t *set, nd_pulse_store_t *store,
                        nd_store_t *legacy, uint32_t legacy_offset);

/* Fold the book into a new base and reset the journal. Called when the journal
 * cannot hold the book, not on a timer: FULL, an append-error retry, a
 * register clear/set-total, and power-off with appends since the last commit. */
int nd_idis_energy_commit(const nd_energy_set_t *set, nd_pulse_store_t *store,
                          nd_store_t *legacy, uint32_t legacy_offset);

/* Snapshot upgrade only: when base() is EMPTY or ERROR, read the un-CRC'd
 * legacy record once and commit it as the new base. Returns 0 when a record
 * was adopted, 1 when there was nothing to adopt, -1 on a device error. */
int nd_idis_energy_adopt(nd_energy_set_t *set, nd_pulse_store_t *store,
                         nd_store_t *legacy, uint32_t legacy_offset);

/* Append one physical pulse and add it to the book. On FULL the current book
 * is committed and the same pulse is retried. Returns -1 when the pulse was
 * not durably accepted; it is still counted, so commit at the next safe
 * point. */
int nd_idis_energy_pulse(nd_energy_set_t *set, nd_pulse_store_t *store,
                         nd_energy_direction_t direction, uint8_t tariff);

/* The meter's documented top load (docs/design/energy-pulse-store.md §7). A
 * power sample above it is a calibration fault, not energy. */
#define ND_IDIS_ENERGY_MAX_POWER_W 41000u

/* Convert one active-power sample in watts into whole pulses at meter_const.
 * The sub-pulse remainder stays in *fraction (micro-pulses). A sample above
 * ND_IDIS_ENERGY_MAX_POWER_W consumes nothing, so an absurd calibration
 * cannot flood the journal; the accepted rate stays at the meter constant. */
uint32_t nd_idis_energy_pulses(uint64_t *fraction, int64_t watts,
                               uint32_t meter_const);

/* The un-CRC'd whole-record format, kept for adoption and downgrade. */
int nd_idis_energy_load_legacy(nd_energy_set_t *set, nd_store_t *store,
                               uint32_t offset);
int nd_idis_energy_save_legacy(const nd_energy_set_t *set, nd_store_t *store,
                               uint32_t offset);

#endif
