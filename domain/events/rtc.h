#ifndef ND_DOMAIN_EVENTS_RTC_H
#define ND_DOMAIN_EVENTS_RTC_H

#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_RTC_POWERON_MAX_GAP_SECONDS 86400000u

void nd_rtc_default_datetime(nd_datetime_t *out);

uint8_t nd_rtc_snapshot_due(const nd_datetime_t *now);

uint8_t nd_rtc_poweron_fault(uint8_t now_valid, uint32_t now_seconds,
                             uint32_t powerdown_seconds);

#endif
