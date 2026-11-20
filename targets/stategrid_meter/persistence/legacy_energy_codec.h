#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_CODEC_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "domain/metering/metering.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

/*
 * Checksums and the reserved padding word belong to the persistence layer and
 * are left untouched by encode/decode. seal() fills them so a downgraded
 * reader accepts the record; valid() is the adoption boundary for a record
 * that was written by the legacy firmware.
 */
void legacy_energy_encode(const nd_metering_snapshot_t *snapshot,
                          legacy_energy_record_t *record);
void legacy_energy_decode(const legacy_energy_record_t *record,
                          nd_metering_snapshot_t *snapshot);

uint32_t legacy_energy_checksum(const uint8_t *data, size_t length);
void legacy_energy_seal(legacy_energy_record_t *record);
int legacy_energy_valid(const legacy_energy_record_t *record);
/* Check_EC_recover validated the energy and month items separately; adoption
 * must accept one half while the other is torn. */
int legacy_energy_valid_energy(const legacy_energy_record_t *record);
int legacy_energy_valid_month(const legacy_energy_record_t *record);

#endif
