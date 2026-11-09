#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"

_Static_assert(ND_TARIFF_SLOTS == ND_LEGACY_TARIFF_SLOTS, "tariff slot count must match legacy");

void legacy_energy_encode(const nd_energy_book_t *book, legacy_energy_record_t *record)
{
    for (uint8_t index = 0; index < ND_TARIFF_SLOTS; index++) {
        record->active_p[index] = book->forward.slots[index].integer;
        record->active_n[index] = book->reverse.slots[index].integer;
        record->active_p_dec[index] = book->forward.slots[index].fraction;
        record->active_n_dec[index] = book->reverse.slots[index].fraction;
    }
    record->month_energy = book->month.integer;
    record->month_energy_dec = book->month.fraction;
}

void legacy_energy_decode(const legacy_energy_record_t *record, nd_energy_book_t *book)
{
    for (uint8_t index = 0; index < ND_TARIFF_SLOTS; index++) {
        book->forward.slots[index].integer = record->active_p[index];
        book->reverse.slots[index].integer = record->active_n[index];
        book->forward.slots[index].fraction = record->active_p_dec[index];
        book->reverse.slots[index].fraction = record->active_n_dec[index];
    }
    book->month.integer = record->month_energy;
    book->month.fraction = record->month_energy_dec;
}
