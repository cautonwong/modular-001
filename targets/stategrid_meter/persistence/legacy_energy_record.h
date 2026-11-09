#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_RECORD_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_RECORD_H

#include <stddef.h>
#include <stdint.h>

/*
 * Frozen copy of the persisted energy slice from the legacy aggregate root.
 * Offsets were measured with tools/legacy_probe.c against the original
 * nidavellir/include/applications.h and instances/stategrid_meter/port.h.
 * Do not reorder fields. Existing meters are read with these bytes.
 */
#define ND_LEGACY_TARIFF_SLOTS 13

#pragma pack(push, 4)
typedef struct {
    uint32_t active_p[ND_LEGACY_TARIFF_SLOTS];
    uint32_t active_n[ND_LEGACY_TARIFF_SLOTS];
    uint16_t active_p_dec[ND_LEGACY_TARIFF_SLOTS];
    uint16_t active_n_dec[ND_LEGACY_TARIFF_SLOTS];
    uint32_t energy_checksum;
    uint16_t month_energy_dec;
    uint16_t reserved; /* padding in the legacy struct, preserved verbatim */
    uint32_t month_energy;
    uint32_t use_energy_checksum;
} legacy_energy_record_t;
#pragma pack(pop)

_Static_assert(offsetof(legacy_energy_record_t, active_p) == 0, "legacy active_p offset");
_Static_assert(offsetof(legacy_energy_record_t, active_n) == 52, "legacy active_n offset");
_Static_assert(offsetof(legacy_energy_record_t, active_p_dec) == 104, "legacy active_p_dec offset");
_Static_assert(offsetof(legacy_energy_record_t, active_n_dec) == 130, "legacy active_n_dec offset");
_Static_assert(offsetof(legacy_energy_record_t, energy_checksum) == 156, "legacy checksum offset");
_Static_assert(offsetof(legacy_energy_record_t, month_energy_dec) == 160, "legacy month dec offset");
_Static_assert(offsetof(legacy_energy_record_t, month_energy) == 164, "legacy month offset");
_Static_assert(offsetof(legacy_energy_record_t, use_energy_checksum) == 168, "legacy use checksum offset");
_Static_assert(sizeof(legacy_energy_record_t) == 172, "legacy energy slice size");

/* Byte offset of active_p inside the legacy persistent block. */
#define ND_LEGACY_ENERGY_OFFSET_IN_BLOCK 3

#endif
