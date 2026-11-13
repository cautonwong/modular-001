#include "targets/stategrid_meter/app/tariff_lookup.h"

#include "targets/stategrid_meter/persistence/legacy_tariff_parser.h"

/* shortcut: one tariff table per firmware. Move it into the target
 * composition root when the target owns instance state. */
static nd_tariff_table_t tariff_table;
static uint8_t last_period;

int tariff_lookup_load(const nd_parameter_store_t *store)
{
    last_period = 0;
    return legacy_tariff_load(store, &tariff_table);
}

uint8_t tariff_lookup_day_table(const nd_datetime_t *now)
{
    return nd_tariff_select_day_table(&tariff_table, now);
}

uint8_t tariff_lookup_fee(uint8_t day_table, const nd_datetime_t *now,
                          uint8_t *period_changed)
{
    uint16_t start;
    uint16_t now_key = (uint16_t)((now->hour << 8) | now->minute);
    uint8_t period = nd_tariff_select_period(&tariff_table, day_table,
                                             now->hour, now->minute, &start);

    if (period_changed != NULL) {
        *period_changed = (uint8_t)(period != last_period || now_key == start);
    }
    last_period = period;

    return nd_tariff_select_fee(&tariff_table, day_table, now->hour, now->minute);
}
