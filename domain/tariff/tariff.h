#ifndef ND_DOMAIN_TARIFF_TARIFF_H
#define ND_DOMAIN_TARIFF_TARIFF_H

#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_TARIFF_MAX_SEASONS 14
#define ND_TARIFF_MAX_DAY_TABLES 8
#define ND_TARIFF_MAX_PERIODS 14
#define ND_TARIFF_MAX_HOLIDAYS 20
#define ND_TARIFF_MAX_FEES 12

typedef struct {
    uint16_t start; /* month<<8|day for seasons, hour<<8|minute for periods */
    uint8_t value;  /* day table number or fee number */
} nd_tariff_interval_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t day_table;
} nd_tariff_holiday_t;

typedef struct {
    uint8_t holiday_count;
    nd_tariff_holiday_t holidays[ND_TARIFF_MAX_HOLIDAYS];

    uint8_t workday_mask; /* bit 7 is Sunday; zero bit marks a rest day */
    uint8_t rest_day_table;

    uint8_t season_count;
    nd_tariff_interval_t seasons[ND_TARIFF_MAX_SEASONS];

    uint8_t day_table_count;
    uint8_t fee_count;
    uint8_t period_count[ND_TARIFF_MAX_DAY_TABLES];
    nd_tariff_interval_t periods[ND_TARIFF_MAX_DAY_TABLES][ND_TARIFF_MAX_PERIODS];
} nd_tariff_table_t;

void nd_tariff_table_init(nd_tariff_table_t *table);

uint8_t nd_tariff_find_interval(const nd_tariff_interval_t *intervals,
                                uint8_t count, uint16_t now);

uint8_t nd_tariff_select_day_table(const nd_tariff_table_t *table,
                                   const nd_datetime_t *now);

uint8_t nd_tariff_select_fee(const nd_tariff_table_t *table, uint8_t day_table,
                             uint8_t hour, uint8_t minute);

#endif
