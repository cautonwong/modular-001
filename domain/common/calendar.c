#include "domain/common/calendar.h"

static const uint8_t month_days[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const uint16_t month_start[13] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static int is_leap(uint16_t year)
{
    return (year & 0x03) == 0;
}

uint8_t nd_calendar_bcd(uint8_t value)
{
    if (value > 99) {
        return 0;
    }
    return (uint8_t)((value / 10) * 0x10 + value % 10);
}

uint8_t nd_calendar_from_bcd(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0F));
}

uint8_t nd_calendar_days_in_month(uint8_t month, uint16_t year)
{
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && is_leap(year)) {
        return 29;
    }
    return month_days[month];
}

int nd_calendar_date_valid(const nd_datetime_t *datetime)
{
    if (datetime->day < 1 || datetime->day > 31) {
        return 0;
    }
    if (datetime->month < 1 || datetime->month > 12) {
        return 0;
    }
    return datetime->day <= nd_calendar_days_in_month(datetime->month, datetime->year);
}

int nd_calendar_time_valid(const nd_datetime_t *datetime)
{
    return datetime->hour <= 23 && datetime->minute <= 59 && datetime->second <= 59;
}

uint32_t nd_calendar_total_day(uint8_t day, uint8_t month, uint16_t year)
{
    uint16_t short_year = year % 100;
    uint32_t result = (uint32_t)day + month_start[month] + (uint32_t)short_year * 365
                      + (short_year >> 2) + 1;

    if (is_leap(year) && month <= 2) {
        result--;
    }
    return result;
}

uint32_t nd_calendar_total_minutes(const nd_datetime_t *datetime)
{
    uint32_t days = nd_calendar_total_day(datetime->day, datetime->month, datetime->year);
    return datetime->minute + (uint32_t)datetime->hour * 60 + days * 60 * 24;
}

uint8_t nd_calendar_weekday(uint8_t day, uint8_t month, uint16_t year)
{
    return (uint8_t)((nd_calendar_total_day(day, month, year) + 5) % 7);
}

void nd_calendar_to_rtc_hex(const nd_datetime_t *datetime, uint8_t out[ND_RTC_HEX_LEN])
{
    out[0] = datetime->second;
    out[1] = datetime->minute;
    out[2] = datetime->hour;
    out[3] = datetime->day;
    out[4] = datetime->month;
    out[5] = (uint8_t)(datetime->year & 0xFF);
    out[6] = (uint8_t)((datetime->year >> 8) & 0xFF);
}

void nd_calendar_to_rtc_bcd(const nd_datetime_t *datetime, uint8_t out[ND_RTC_BCD_LEN])
{
    out[0] = nd_calendar_bcd(datetime->second);
    out[1] = nd_calendar_bcd(datetime->minute);
    out[2] = nd_calendar_bcd(datetime->hour);
    out[3] = nd_calendar_bcd(datetime->day);
    out[4] = nd_calendar_bcd(datetime->month);

    uint16_t year = datetime->year > 2000 ? datetime->year - 2000 : 0;
    out[5] = nd_calendar_bcd((uint8_t)year);
}
