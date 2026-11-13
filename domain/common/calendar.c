#include <stddef.h>
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

void nd_calendar_from_total_day(uint32_t total_day, nd_datetime_t *out)
{
    uint32_t day = total_day > 0 ? total_day - 1 : 0;
    uint16_t year = 2000;
    uint8_t month = 1;

    while (day >= (uint32_t)(is_leap(year) ? 366 : 365)) {
        day -= is_leap(year) ? 366 : 365;
        year++;
    }
    while (day >= nd_calendar_days_in_month(month, year)) {
        day -= nd_calendar_days_in_month(month, year);
        month++;
    }

    out->second = 0;
    out->minute = 0;
    out->hour = 0;
    out->day = (uint8_t)(day + 1);
    out->month = month;
    out->year = year;
}

uint32_t nd_calendar_total_minutes(const nd_datetime_t *datetime)
{
    uint32_t days = nd_calendar_total_day(datetime->day, datetime->month, datetime->year);
    return datetime->minute + (uint32_t)datetime->hour * 60 + days * 60 * 24;
}

int nd_calendar_add_seconds(const nd_datetime_t *datetime, int32_t seconds,
                            nd_datetime_t *out)
{
    if (datetime == NULL || out == NULL) {
        return -1;
    }
    int64_t total = (int64_t)nd_calendar_total_day(datetime->day, datetime->month,
                                                   datetime->year) * 86400
        + (int64_t)datetime->hour * 3600 + (int64_t)datetime->minute * 60
        + datetime->second + seconds;
    if (total < 86400) {
        return -1;
    }
    uint32_t day = (uint32_t)(total / 86400);
    uint32_t rest = (uint32_t)(total % 86400);
    nd_calendar_from_total_day(day, out);
    out->hour = (uint8_t)(rest / 3600);
    rest %= 3600;
    out->minute = (uint8_t)(rest / 60);
    out->second = (uint8_t)(rest % 60);
    return 0;
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
