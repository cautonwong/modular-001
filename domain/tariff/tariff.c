#include "domain/tariff/tariff.h"

#include <string.h>

static uint8_t clamp_day_table(const nd_tariff_table_t *table, uint8_t value)
{
    if (value == 0 || value > ND_TARIFF_MAX_DAY_TABLES) {
        return 1;
    }
    if (value > table->day_table_count) {
        return 1;
    }
    return value;
}

static uint8_t clamp_fee(const nd_tariff_table_t *table, uint8_t value)
{
    if (value == 0 || value > ND_TARIFF_MAX_FEES) {
        return 1;
    }
    if (value > table->fee_count) {
        return 1;
    }
    return value;
}

void nd_tariff_table_init(nd_tariff_table_t *table)
{
    memset(table, 0, sizeof(*table));
}

uint8_t nd_tariff_find_interval(const nd_tariff_interval_t *intervals,
                                uint8_t count, uint16_t now)
{
    if (count == 0) {
        return 0;
    }

    for (uint8_t index = 0; index < count; index++) {
        uint16_t start = intervals[index].start;
        uint16_t next = intervals[(index + 1) % count].start;

        if (start < next) {
            if (now >= start && now < next) {
                return index;
            }
        } else if (start == next) {
            if (now >= start) {
                return index;
            }
        } else if (now >= start || now < next) {
            return index;
        }
    }
    return (uint8_t)(count - 1);
}

uint8_t nd_tariff_select_day_table(const nd_tariff_table_t *table,
                                   const nd_datetime_t *now)
{
    uint8_t holidays = table->holiday_count;
    if (holidays > ND_TARIFF_MAX_HOLIDAYS) {
        holidays = ND_TARIFF_MAX_HOLIDAYS;
    }
    for (uint8_t index = 0; index < holidays; index++) {
        const nd_tariff_holiday_t *holiday = &table->holidays[index];
        if (holiday->year == now->year && holiday->month == now->month
            && holiday->day == now->day) {
            return clamp_day_table(table, holiday->day_table);
        }
    }

    uint8_t weekday = nd_calendar_weekday(now->day, now->month, now->year);
    uint8_t weekday_bit = (uint8_t)(0x80u >> weekday);
    if ((table->workday_mask & weekday_bit) == 0) {
        return clamp_day_table(table, table->rest_day_table);
    }

    if (table->season_count == 0) {
        return 1;
    }

    uint8_t seasons = table->season_count;
    if (seasons > ND_TARIFF_MAX_SEASONS) {
        seasons = ND_TARIFF_MAX_SEASONS;
    }
    uint16_t key = (uint16_t)((now->month << 8) | now->day);
    uint8_t index = nd_tariff_find_interval(table->seasons, seasons, key);
    return clamp_day_table(table, table->seasons[index].value);
}

uint8_t nd_tariff_select_fee(const nd_tariff_table_t *table, uint8_t day_table,
                             uint8_t hour, uint8_t minute)
{
    if (day_table == 0) {
        day_table = 1;
    }
    if (day_table > ND_TARIFF_MAX_DAY_TABLES) {
        day_table = ND_TARIFF_MAX_DAY_TABLES;
    }

    uint8_t slot = (uint8_t)(day_table - 1);
    uint8_t periods = table->period_count[slot];
    if (periods == 0) {
        return 1;
    }
    if (periods > ND_TARIFF_MAX_PERIODS) {
        periods = ND_TARIFF_MAX_PERIODS;
    }

    uint16_t key = (uint16_t)((hour << 8) | minute);
    uint8_t index = nd_tariff_find_interval(table->periods[slot], periods, key);
    return clamp_fee(table, table->periods[slot][index].value);
}
