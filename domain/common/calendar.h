#ifndef ND_DOMAIN_COMMON_CALENDAR_H
#define ND_DOMAIN_COMMON_CALENDAR_H

#include <stdint.h>

#define ND_RTC_HEX_LEN 7
#define ND_RTC_BCD_LEN 6

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month; /* 1..12 */
    uint16_t year; /* full year, legacy math uses the low two digits */
} nd_datetime_t;

uint8_t nd_calendar_bcd(uint8_t value);

uint8_t nd_calendar_days_in_month(uint8_t month, uint16_t year);

int nd_calendar_date_valid(const nd_datetime_t *datetime);

int nd_calendar_time_valid(const nd_datetime_t *datetime);

uint32_t nd_calendar_total_day(uint8_t day, uint8_t month, uint16_t year);

uint32_t nd_calendar_total_minutes(const nd_datetime_t *datetime);

uint8_t nd_calendar_weekday(uint8_t day, uint8_t month, uint16_t year);

void nd_calendar_to_rtc_hex(const nd_datetime_t *datetime, uint8_t out[ND_RTC_HEX_LEN]);

void nd_calendar_to_rtc_bcd(const nd_datetime_t *datetime, uint8_t out[ND_RTC_BCD_LEN]);

#endif
