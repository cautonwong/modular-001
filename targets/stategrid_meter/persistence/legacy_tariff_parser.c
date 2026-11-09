#include "targets/stategrid_meter/persistence/legacy_tariff_parser.h"

#include "domain/common/calendar.h"
#include "targets/stategrid_meter/persistence/legacy_tariff_ids.h"

static uint8_t cap(uint8_t value, uint8_t maximum)
{
    return value > maximum ? maximum : value;
}

int legacy_tariff_load(const nd_parameter_store_t *store, nd_tariff_table_t *table)
{
    uint8_t parameters[5];
    if (nd_param_read(store, ND_LEGACY_PARAM_SEASON_DAY, parameters,
                      sizeof(parameters)) != 0) {
        return -1;
    }

    nd_tariff_table_init(table);
    table->season_count = cap(parameters[0], ND_TARIFF_MAX_SEASONS);
    table->day_table_count = cap(parameters[1], ND_TARIFF_MAX_DAY_TABLES);
    table->fee_count = cap(parameters[3], ND_TARIFF_MAX_FEES);
    table->holiday_count = cap(parameters[4], ND_TARIFF_MAX_HOLIDAYS);
    uint8_t periods = cap(parameters[2], ND_TARIFF_MAX_PERIODS);

    uint8_t weekend = 0xFF;
    uint8_t weekend_fee = 1;
    nd_param_read(store, ND_LEGACY_PARAM_WEEKEND, &weekend, 1);
    nd_param_read(store, ND_LEGACY_PARAM_WEEKEND_FEE, &weekend_fee, 1);
    table->workday_mask = weekend;
    table->rest_day_table = weekend_fee;

    for (uint8_t index = 0; index < table->season_count; index++) {
        uint8_t entry[3];
        if (nd_param_read(store, ND_LEGACY_PARAM_SEASON_TABLE0 + index, entry,
                          sizeof(entry)) != 0) {
            table->season_count = index;
            break;
        }
        table->seasons[index].start = (uint16_t)((entry[0] << 8) | entry[1]);
        table->seasons[index].value = entry[2];
    }

    for (uint8_t index = 0; index < table->holiday_count; index++) {
        uint8_t entry[6];
        if (nd_param_read(store, ND_LEGACY_PARAM_HOLIDAY_TABLE + index, entry,
                          sizeof(entry)) != 0) {
            table->holiday_count = index;
            break;
        }
        table->holidays[index].year = (uint16_t)((entry[0] << 8) | entry[1]);
        table->holidays[index].month = entry[2];
        table->holidays[index].day = entry[3];
        table->holidays[index].day_table = entry[5];
    }

    for (uint8_t number = 1; number <= table->day_table_count; number++) {
        uint8_t slot = (uint8_t)(number - 1);
        table->period_count[slot] = periods;
        if (periods == 0) {
            continue;
        }

        uint8_t entries[3 * ND_TARIFF_MAX_PERIODS];
        size_t length = (size_t)periods * 3;
        if (nd_param_read(store, ND_LEGACY_PARAM_DAY_TABLE00 + slot, entries,
                          length) != 0) {
            table->period_count[slot] = 0;
            continue;
        }

        for (uint8_t period = 0; period < periods; period++) {
            table->periods[slot][period].start =
                (uint16_t)((entries[period * 3] << 8) | entries[period * 3 + 1]);
            table->periods[slot][period].value =
                nd_calendar_from_bcd(entries[period * 3 + 2]);
        }
    }

    return 0;
}
