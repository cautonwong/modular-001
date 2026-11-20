#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"

#include <stddef.h>

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

/* Mirrors do_checksum(): 0xA5A5 plus every byte, truncated to 16 bits. */
uint32_t legacy_energy_checksum(const uint8_t *data, size_t length)
{
    uint16_t checksum = 0xA5A5u;

    for (size_t index = 0; index < length; index++) {
        checksum = (uint16_t)(checksum + data[index]);
    }
    return checksum;
}

void legacy_energy_seal(legacy_energy_record_t *record)
{
    record->reserved = 0;
    record->energy_checksum =
        legacy_energy_checksum((const uint8_t *)record,
                               offsetof(legacy_energy_record_t, energy_checksum));
    record->use_energy_checksum =
        legacy_energy_checksum((const uint8_t *)&record->month_energy_dec,
                               offsetof(legacy_energy_record_t,
                                        use_energy_checksum)
                                   - offsetof(legacy_energy_record_t,
                                              month_energy_dec));
}

int legacy_energy_valid_energy(const legacy_energy_record_t *record)
{
    if (record == NULL) {
        return 0;
    }
    return record->energy_checksum
        == legacy_energy_checksum((const uint8_t *)record,
                                  offsetof(legacy_energy_record_t,
                                           energy_checksum));
}

int legacy_energy_valid_month(const legacy_energy_record_t *record)
{
    if (record == NULL) {
        return 0;
    }
    return record->use_energy_checksum
        == legacy_energy_checksum((const uint8_t *)&record->month_energy_dec,
                                  offsetof(legacy_energy_record_t,
                                           use_energy_checksum)
                                      - offsetof(legacy_energy_record_t,
                                                 month_energy_dec));
}

int legacy_energy_valid(const legacy_energy_record_t *record)
{
    return legacy_energy_valid_energy(record)
        && legacy_energy_valid_month(record);
}
