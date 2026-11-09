#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_CODEC_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_CODEC_H

#include "domain/metering/energy.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

/*
 * Checksums and the reserved padding word belong to the persistence layer and
 * are left untouched by these functions.
 */
void legacy_energy_encode(const nd_energy_book_t *book, legacy_energy_record_t *record);
void legacy_energy_decode(const legacy_energy_record_t *record, nd_energy_book_t *book);

#endif
