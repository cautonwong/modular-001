#ifndef ND_DOMAIN_COMMON_CLOCK_H
#define ND_DOMAIN_COMMON_CLOCK_H

#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_CLOCK_WILD_MONTH 0x01u
#define ND_CLOCK_WILD_DAY 0x02u
#define ND_CLOCK_WILD_HOUR 0x04u
#define ND_CLOCK_WILD_MINUTE 0x08u
#define ND_CLOCK_WILD_SECOND 0x10u

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t wildcard_mask;
} nd_clock_datetime_t;

typedef struct {
    int16_t time_zone; /* normal local time deviation from UTC, minutes */
    int16_t daylight_deviation; /* extra minutes while DST is active */
    uint8_t daylight_enabled;
    uint8_t clock_base; /* 0 undefined, 1 crystal, 2/3 mains, 4 GPS, 5 radio */
    nd_clock_datetime_t daylight_begin;
    nd_clock_datetime_t daylight_end;
} nd_clock_config_t;

void nd_clock_config_init(nd_clock_config_t *config);
int nd_clock_dst_active(const nd_clock_config_t *config,
                        const nd_datetime_t *local_time);
int nd_clock_effective_deviation(const nd_clock_config_t *config,
                                 const nd_datetime_t *local_time,
                                 int16_t *deviation);
int nd_clock_adjust_to_quarter(nd_datetime_t *time);
int nd_clock_adjust_to_minute(nd_datetime_t *time);
int nd_clock_adjust_to_period(nd_datetime_t *time, uint16_t period_seconds);
int nd_clock_shift(nd_datetime_t *time, int32_t seconds);

#endif
