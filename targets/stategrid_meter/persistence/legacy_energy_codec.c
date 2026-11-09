#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"

_Static_assert(ND_TARIFF_SLOTS == ND_LEGACY_TARIFF_SLOTS, "tariff slot count must match legacy");

void legacy_energy_encode(const nd_metering_snapshot_t *snapshot,
                          legacy_energy_record_t *record)
{
    for (uint8_t index = 0; index < ND_TARIFF_SLOTS; index++) {
        record->active_p[index] = snapshot->forward.slots[index].integer;
        record->active_n[index] = snapshot->reverse.slots[index].integer;
        record->active_p_dec[index] = snapshot->forward.slots[index].fraction;
        record->active_n_dec[index] = snapshot->reverse.slots[index].fraction;
    }
    record->month_energy = snapshot->month.integer;
    record->month_energy_dec = snapshot->month.fraction;
}

void legacy_energy_decode(const legacy_energy_record_t *record,
                          nd_metering_snapshot_t *snapshot)
{
    for (uint8_t index = 0; index < ND_TARIFF_SLOTS; index++) {
        snapshot->forward.slots[index].integer = record->active_p[index];
        snapshot->reverse.slots[index].integer = record->active_n[index];
        snapshot->forward.slots[index].fraction = record->active_p_dec[index];
        snapshot->reverse.slots[index].fraction = record->active_n_dec[index];
    }
    snapshot->month.integer = record->month_energy;
    snapshot->month.fraction = record->month_energy_dec;
}
