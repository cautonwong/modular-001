#ifndef ND_IDIS_CALIBRATION_H
#define ND_IDIS_CALIBRATION_H

#include <stdint.h>

#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_registry.h"

/*
 * Factory scale calibration written by the production tool.
 *
 * The record is 16 bytes, fixed by that tool: magic plus three int32 factors.
 * It is usable only when the magic matches and voltage, current and power are
 * each greater than zero. The record has no checksum field, so a slot whose
 * magic does not match cannot be told apart from unwritten media.
 *
 * 0xE400 is the authoritative slot. 0xE000 is registry entry 255, and the
 * registry trailer for a 255-entry blob lands there too, so the production
 * tool must write 0xE400. A tool that keeps writing 0xE000 corrupts registry
 * entries 255-256, which the firmware cannot prevent. Boot reads 0xE000 only
 * while 0xE400 has no usable record, copies a valid legacy record to 0xE400
 * (a failed rewrite is retried on the next boot), then never reads the old
 * slot again because it belongs to the registry blob.
 */
#define ND_IDIS_CALIBRATION_MAGIC 0x43414C31u
#define ND_IDIS_CALIBRATION_LEGACY_OFFSET 0xE000u
#define ND_IDIS_CALIBRATION_OFFSET 0xE400u

typedef struct {
    uint32_t magic;
    int32_t voltage;
    int32_t current;
    int32_t power;
} nd_idis_calibration_record_t;

#define ND_IDIS_CALIBRATION_RECORD_SIZE 16u
_Static_assert(sizeof(nd_idis_calibration_record_t)
                   == ND_IDIS_CALIBRATION_RECORD_SIZE,
               "calibration record format is fixed by the production tool");

/* The EEPROM map is a product contract, so every constant is pinned to its
 * measured value. A one-sided edit of any boundary fails the build instead of
 * silently overlapping the next region. */
_Static_assert(ND_IDIS_REGISTRY_EEPROM_OFFSET == 0xD800u,
               "registry slot moved");
_Static_assert(ND_IDIS_CALIBRATION_OFFSET == 0xE400u,
               "calibration slot moved");
_Static_assert(ND_IDIS_ENERGY_BASE_OFFSET == 0xE800u,
               "energy base slot moved");
_Static_assert(ND_IDIS_ENERGY_BASE_ALT_OFFSET == 0xEC00u,
               "energy alternate slot moved");
_Static_assert(ND_IDIS_REGISTRY_EEPROM_OFFSET + ND_IDIS_REGISTRY_HEADER_SIZE
                       + ND_IDIS_REGISTRY_MAX * ND_IDIS_REGISTRY_ENTRY_SIZE
                       + ND_IDIS_REGISTRY_TRAILER_SIZE
                   <= ND_IDIS_CALIBRATION_OFFSET,
               "calibration must start after the registry maximum blob");
_Static_assert(ND_IDIS_CALIBRATION_OFFSET + ND_IDIS_CALIBRATION_RECORD_SIZE
                   <= ND_IDIS_ENERGY_BASE_OFFSET,
               "calibration must end before the energy base");
_Static_assert(ND_IDIS_ENERGY_BASE_OFFSET + ND_IDIS_ENERGY_BASE_BYTES
                   == ND_IDIS_ENERGY_BASE_ALT_OFFSET,
               "energy snapshot slots must be adjacent");
_Static_assert(ND_IDIS_ENERGY_BASE_ALT_OFFSET + ND_IDIS_ENERGY_BASE_BYTES
                   <= 0x10000u,
               "energy snapshots must fit the 16-bit EEPROM address space");

typedef enum {
    /* No usable record: the caller keeps the compiled defaults. */
    ND_IDIS_CALIBRATION_NONE = 0,
    /* The record was read from the current slot. */
    ND_IDIS_CALIBRATION_LOADED,
    /* The record was adopted from the legacy slot and rewritten once. */
    ND_IDIS_CALIBRATION_ADOPTED,
    /* A slot carries the magic but a factor is not positive. The caller keeps
     * the compiled defaults and reports a fault. */
    ND_IDIS_CALIBRATION_FAULT,
    /* A well-formed registry blob covers the legacy slot, so the factory
     * record was overwritten. The caller keeps the compiled defaults and
     * reports a fault. */
    ND_IDIS_CALIBRATION_LOST,
} nd_idis_calibration_status_t;

/* COSEM event code boot reports when the record is unusable: 14, NV memory
 * error. */
#define ND_IDIS_CALIBRATION_FAULT_EVENT 14u

/*
 * Loads the calibration and performs the one-time move out of the registry
 * range. Adoption is idempotent: once the current slot validates, the legacy
 * slot is never read or written again.
 *
 * NONE means a normal uncalibrated meter: blank media, or a legacy slot
 * overwritten by something other than a registry blob, which is
 * indistinguishable from blank. LOST means the registry header shows its
 * blob covers 0xE000; that is the documented deployment risk from a
 * production tool still writing the old offset.
 */
nd_idis_calibration_status_t nd_idis_calibration_load(
    nd_idis_calibration_record_t *record, const nd_store_t *store);

/* The event boot raises for this status, or 0 when no event is needed. */
uint16_t nd_idis_calibration_fault_event(nd_idis_calibration_status_t status);

#endif
