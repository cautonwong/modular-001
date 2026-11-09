#include "domain/metering/energy.h"

#include <string.h>

static nd_energy_channel_t *channel_of(nd_energy_book_t *book,
                                       nd_energy_direction_t direction)
{
    return direction == ND_ENERGY_REVERSE ? &book->reverse : &book->forward;
}

static const nd_energy_channel_t *channel_of_const(
    const nd_energy_book_t *book, nd_energy_direction_t direction)
{
    return direction == ND_ENERGY_REVERSE ? &book->reverse : &book->forward;
}

static uint8_t valid_tariff(uint8_t tariff)
{
    return (tariff == ND_TARIFF_TOTAL || tariff >= ND_TARIFF_SLOTS) ? 1 : tariff;
}

static int64_t amount_in_0_0001(const nd_energy_book_t *book,
                                const nd_energy_amount_t *amount)
{
    int64_t value = (int64_t)amount->integer * 100;
    if (book->meter_const != 0) {
        value += (int64_t)amount->fraction * 10000 / book->meter_const;
    }
    return value;
}

static uint32_t carry_into(nd_energy_amount_t *amount, uint32_t pulses,
                           uint32_t step)
{
    uint32_t total = (uint32_t)amount->fraction + pulses;
    uint32_t carry = total / step;

    amount->fraction = (uint16_t)(total % step);
    amount->integer += carry;
    return carry;
}

static int save_crossed(uint32_t start, uint32_t carry, uint32_t save_step)
{
    if (save_step == 0) {
        return 0;
    }
    for (uint32_t index = 1; index <= carry; index++) {
        if ((start + index) % save_step == 0) {
            return 1;
        }
    }
    return 0;
}

void nd_energy_book_init(nd_energy_book_t *book, uint32_t meter_const,
                         uint8_t cal_word, uint32_t save_step)
{
    memset(book, 0, sizeof(*book));
    book->meter_const = meter_const;
    book->cal_word = cal_word;
    book->save_step = save_step;
}

nd_energy_update_t nd_energy_add_pulses(nd_energy_book_t *book,
                                        nd_energy_direction_t direction,
                                        uint8_t tariff, uint32_t pulses)
{
    nd_energy_update_t changed = {0};
    uint32_t step = book->meter_const / 100;
    nd_energy_channel_t *channel;

    if (step == 0 || pulses == 0) {
        return changed;
    }

    tariff = valid_tariff(tariff);
    channel = channel_of(book, direction);

    changed.energy_changed = 1;
    uint32_t total_start = channel->slots[ND_TARIFF_TOTAL].integer;

    carry_into(&channel->slots[tariff], pulses, step);
    uint32_t total_carry = carry_into(&channel->slots[ND_TARIFF_TOTAL], pulses, step);

    if (save_crossed(total_start, total_carry, book->save_step)) {
        changed.energy_save_due = 1;
    }

    uint8_t month_bit = direction == ND_ENERGY_FORWARD ? ND_CAL_ADD_POS : ND_CAL_ADD_NEG;
    if (book->cal_word & month_bit) {
        uint32_t month_start = book->month.integer;
        uint32_t month_carry = carry_into(&book->month, pulses, step);

        changed.month_changed = 1;
        if (save_crossed(month_start, month_carry, book->save_step)) {
            changed.month_save_due = 1;
        }
    }

    return changed;
}

int64_t nd_energy_read_amount(const nd_energy_book_t *book,
                              const nd_energy_amount_t *amount,
                              nd_energy_resolution_t resolution)
{
    int64_t value = amount_in_0_0001(book, amount);

    if (resolution == ND_ENERGY_RES_0_01_KWH) {
        value /= 100;
    }
    return value;
}

int64_t nd_energy_read(const nd_energy_book_t *book,
                       nd_energy_direction_t direction, uint8_t tariff,
                       nd_energy_resolution_t resolution)
{
    if (tariff >= ND_TARIFF_SLOTS) {
        return 0;
    }
    return nd_energy_read_amount(
        book, &channel_of_const(book, direction)->slots[tariff], resolution);
}

int64_t nd_energy_read_combined(const nd_energy_book_t *book, uint8_t tariff,
                                nd_energy_resolution_t resolution)
{
    int64_t value = 0;

    if (tariff >= ND_TARIFF_SLOTS) {
        return 0;
    }
    if (book->cal_word & ND_CAL_ADD_POS) {
        value += amount_in_0_0001(book, &book->forward.slots[tariff]);
    }
    if (book->cal_word & ND_CAL_DEC_POS) {
        value -= amount_in_0_0001(book, &book->forward.slots[tariff]);
    }
    if (book->cal_word & ND_CAL_ADD_NEG) {
        value += amount_in_0_0001(book, &book->reverse.slots[tariff]);
    }
    if (book->cal_word & ND_CAL_DEC_NEG) {
        value -= amount_in_0_0001(book, &book->reverse.slots[tariff]);
    }
    if (resolution == ND_ENERGY_RES_0_01_KWH) {
        value /= 100;
    }
    return value;
}

void nd_energy_clear(nd_energy_book_t *book)
{
    memset(&book->forward, 0, sizeof(book->forward));
    memset(&book->reverse, 0, sizeof(book->reverse));
    memset(&book->month, 0, sizeof(book->month));
}

void nd_energy_set_total(nd_energy_book_t *book, nd_energy_direction_t direction,
                         uint32_t integer)
{
    nd_energy_channel_t *channel = channel_of(book, direction);

    memset(channel, 0, sizeof(*channel));
    channel->slots[ND_TARIFF_TOTAL].integer = integer;
    channel->slots[1].integer = integer;
    memset(&book->month, 0, sizeof(book->month));
}
