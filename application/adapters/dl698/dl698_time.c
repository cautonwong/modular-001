#include "application/adapters/dl698/dl698_time.h"

#include "domain/common/calendar.h"

#define EPOCH_MINUTES (24u * 60u)

static uint8_t field_ok(uint16_t value, uint16_t min, uint16_t max, uint8_t wildcard)
{
    if (wildcard && value == 0xFF) {
        return 1;
    }
    return (uint8_t)(value >= min && value <= max);
}

uint8_t dl_data_time_check(uint8_t *date_time, uint8_t type)
{
    uint16_t year = ((uint16_t)date_time[0] << 8) | date_time[1];
    uint8_t month = date_time[2];
    uint8_t day = date_time[3];
    uint8_t wildcard = (uint8_t)((type & TIME_WILDCARD) != 0);

    if (!(wildcard && year == 0xFFFF) && (year < 2000 || year > 2099)) {
        return 0;
    }
    if (!field_ok(month, 1, 12, wildcard)) {
        return 0;
    }
    if (!field_ok(day, 1, 31, wildcard)) {
        return 0;
    }
    if (!(wildcard && day == 0xFF) && month != 0xFF && year != 0xFFFF
        && day > nd_calendar_days_in_month(month, year)) {
        return 0;
    }
    if (!field_ok(date_time[4], 0, 23, wildcard)) {
        return 0;
    }
    if (!field_ok(date_time[5], 0, 59, wildcard)) {
        return 0;
    }
    return field_ok(date_time[6], 0, 59, wildcard);
}

uint32_t dl_timeTag_interval(uint8_t *date, uint8_t type)
{
    static const uint32_t unit_seconds[6] = {
        1, 60, 3600, 86400, 86400ul * 30, 86400ul * 365
    };
    uint8_t unit = date[0];
    uint32_t interval = ((uint16_t)date[1] << 8) | date[2];

    (void)type;
    if (unit >= sizeof(unit_seconds) / sizeof(unit_seconds[0])) {
        return 0;
    }
    return interval * unit_seconds[unit];
}

uint32_t dl_datetime_to_second(uint8_t *date, uint8_t type)
{
    nd_datetime_t datetime;

    (void)type;
    datetime.year = ((uint16_t)date[0] << 8) | date[1];
    datetime.month = date[2];
    datetime.day = date[3];
    datetime.hour = date[4];
    datetime.minute = date[5];
    datetime.second = date[6];

    return (nd_calendar_total_minutes(&datetime) - EPOCH_MINUTES) * 60u
           + datetime.second;
}
