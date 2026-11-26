#include "domain/tariff/tariff.h"

#include <stddef.h>

#define SEASON_PERIOD 0x1200
#define PERIOD_MINUTES 1440
#define PERIOD_MIN_GAP 15

static uint8_t seasons_bound(uint8_t day_table_count)
{
    return day_table_count > ND_TARIFF_MAX_DAY_TABLES
               ? ND_TARIFF_MAX_DAY_TABLES
               : day_table_count;
}

static nd_tariff_error_t validate_season_entry(const nd_tariff_interval_t *entry,
                                               uint8_t bound)
{
    uint8_t month = (uint8_t)(entry->start >> 8);
    uint8_t day = (uint8_t)(entry->start & 0xFFu);

    if (month == 0 || month > 12) {
        return ND_TARIFF_ERR_DATE;
    }
    if (day == 0 || day > nd_calendar_days_in_month(month, 2001)) {
        return ND_TARIFF_ERR_DATE;
    }
    if (entry->value == 0 || entry->value > bound) {
        return ND_TARIFF_ERR_VALUE;
    }
    return ND_TARIFF_VALID;
}

nd_tariff_error_t nd_tariff_validate_seasons(
    const nd_tariff_interval_t *seasons, uint8_t count,
    uint8_t day_table_count)
{
    uint8_t bound = seasons_bound(day_table_count);
    uint8_t wrapped = 0;

    if (count == 0) {
        return ND_TARIFF_ERR_EMPTY;
    }
    if (count > ND_TARIFF_MAX_SEASONS) {
        return ND_TARIFF_ERR_COUNT;
    }
    for (uint8_t index = 0; index < count; index++) {
        nd_tariff_error_t error = validate_season_entry(&seasons[index], bound);
        if (error != ND_TARIFF_VALID) {
            return error;
        }
    }
    for (uint8_t index = 0; index < count; index++) {
        const nd_tariff_interval_t *current = &seasons[index];
        const nd_tariff_interval_t *next = &seasons[(index + 1) % count];
        uint16_t gap;

        if (next->start == current->start) {
            if (next->value != current->value) {
                return ND_TARIFF_ERR_ORDER;
            }
            continue;
        }
        if (next->start < current->start) {
            if (wrapped) {
                return ND_TARIFF_ERR_ORDER;
            }
            wrapped = 1;
            gap = (uint16_t)(SEASON_PERIOD - current->start + next->start);
            if (gap < 1) {
                return ND_TARIFF_ERR_ORDER;
            }
            continue;
        }
    }
    return ND_TARIFF_VALID;
}

static uint8_t fee_bound(uint8_t fee_count)
{
    return fee_count > ND_TARIFF_MAX_FEES ? ND_TARIFF_MAX_FEES : fee_count;
}

static nd_tariff_error_t validate_period_entry(const nd_tariff_interval_t *entry,
                                               uint8_t bound)
{
    uint8_t hour = (uint8_t)(entry->start >> 8);
    uint8_t minute = (uint8_t)(entry->start & 0xFFu);

    if (hour > 23 || minute > 59) {
        return ND_TARIFF_ERR_TIME;
    }
    if (entry->value == 0 || entry->value > bound) {
        return ND_TARIFF_ERR_VALUE;
    }
    return ND_TARIFF_VALID;
}

nd_tariff_error_t nd_tariff_validate_periods(
    const nd_tariff_interval_t *periods, uint8_t count, uint8_t fee_count)
{
    uint8_t bound = fee_bound(fee_count);
    uint8_t wrapped = 0;

    if (count == 0) {
        return ND_TARIFF_ERR_EMPTY;
    }
    if (count > ND_TARIFF_MAX_PERIODS) {
        return ND_TARIFF_ERR_COUNT;
    }
    for (uint8_t index = 0; index < count; index++) {
        nd_tariff_error_t error = validate_period_entry(&periods[index], bound);
        if (error != ND_TARIFF_VALID) {
            return error;
        }
    }
    for (uint8_t index = 0; index < count; index++) {
        uint16_t current = periods[index].start;
        uint16_t next = periods[(index + 1) % count].start;
        uint16_t now = (uint16_t)((current >> 8) * 60 + (current & 0xFFu));
        uint16_t later = (uint16_t)((next >> 8) * 60 + (next & 0xFFu));
        uint16_t gap = (uint16_t)(later - now);

        if (next == current) {
            if (periods[(index + 1) % count].value != periods[index].value) {
                return ND_TARIFF_ERR_ORDER;
            }
            continue;
        }
        if (later < now) {
            if (wrapped) {
                return ND_TARIFF_ERR_ORDER;
            }
            wrapped = 1;
            gap = (uint16_t)(PERIOD_MINUTES - now + later);
            if (gap < PERIOD_MIN_GAP) {
                return ND_TARIFF_ERR_SPACING;
            }
            continue;
        }
        if (gap < PERIOD_MIN_GAP) {
            return ND_TARIFF_ERR_SPACING;
        }
    }
    return ND_TARIFF_VALID;
}

nd_tariff_error_t nd_tariff_table_validate(const nd_tariff_table_t *table)
{
    nd_tariff_error_t error;

    if (table == NULL) {
        return ND_TARIFF_ERR_COUNT;
    }
    if (table->holiday_count > ND_TARIFF_MAX_HOLIDAYS
        || table->season_count > ND_TARIFF_MAX_SEASONS
        || table->day_table_count > ND_TARIFF_MAX_DAY_TABLES
        || table->fee_count > ND_TARIFF_MAX_FEES) {
        return ND_TARIFF_ERR_COUNT;
    }
    if (table->season_count > 0) {
        error = nd_tariff_validate_seasons(table->seasons, table->season_count,
                                           table->day_table_count);
        if (error != ND_TARIFF_VALID) {
            return error;
        }
    }
    for (uint8_t index = 0; index < table->holiday_count; index++) {
        const nd_tariff_holiday_t *holiday = &table->holidays[index];
        if (holiday->month == 0 || holiday->month > 12
            || holiday->day == 0
            || holiday->day > nd_calendar_days_in_month(holiday->month,
                                                        holiday->year)) {
            return ND_TARIFF_ERR_DATE;
        }
        if (holiday->day_table == 0
            || holiday->day_table > ND_TARIFF_MAX_DAY_TABLES) {
            return ND_TARIFF_ERR_VALUE;
        }
    }
    if (table->rest_day_table > ND_TARIFF_MAX_DAY_TABLES) {
        return ND_TARIFF_ERR_VALUE;
    }
    for (uint8_t slot = 0; slot < table->day_table_count; slot++) {
        uint8_t count = table->period_count[slot];
        if (count > ND_TARIFF_MAX_PERIODS) {
            return ND_TARIFF_ERR_COUNT;
        }
        if (count == 0) {
            continue;
        }
        error = nd_tariff_validate_periods(table->periods[slot], count,
                                           table->fee_count);
        if (error != ND_TARIFF_VALID) {
            return error;
        }
    }
    return ND_TARIFF_VALID;
}
