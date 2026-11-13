#include "domain/events/rtc.h"

void nd_rtc_default_datetime(nd_datetime_t *out)
{
    out->second = 8;
    out->minute = 8;
    out->hour = 8;
    out->day = 8;
    out->month = 9;
    out->year = 2022;
}

uint8_t nd_rtc_snapshot_due(const nd_datetime_t *now)
{
    return (uint8_t)(now->hour == 12 && now->minute == 1 && now->second == 35);
}

uint8_t nd_rtc_poweron_fault(uint8_t now_valid, uint32_t now_seconds,
                             uint32_t powerdown_seconds)
{
    if (!now_valid || now_seconds < powerdown_seconds) {
        return 1;
    }
    return (uint8_t)(now_seconds - powerdown_seconds
                     > ND_RTC_POWERON_MAX_GAP_SECONDS);
}
