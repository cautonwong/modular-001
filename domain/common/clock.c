#include "domain/common/clock.h"

#include <string.h>

static int time_key(const nd_clock_datetime_t *rule)
{
    return ((int)rule->month * 32 + rule->day) * 86400
        + (int)rule->hour * 3600 + (int)rule->minute * 60 + rule->second;
}

static int local_key(const nd_datetime_t *time)
{
    return ((int)time->month * 32 + time->day) * 86400
        + (int)time->hour * 3600 + (int)time->minute * 60 + time->second;
}

static int rule_matches(const nd_clock_datetime_t *rule,
                        const nd_datetime_t *time)
{
    if (!(rule->wildcard_mask & ND_CLOCK_WILD_MONTH)
        && rule->month != time->month) {
        return 0;
    }
    if (!(rule->wildcard_mask & ND_CLOCK_WILD_DAY) && rule->day != time->day) {
        return 0;
    }
    if (!(rule->wildcard_mask & ND_CLOCK_WILD_HOUR)
        && rule->hour != time->hour) {
        return 0;
    }
    if (!(rule->wildcard_mask & ND_CLOCK_WILD_MINUTE)
        && rule->minute != time->minute) {
        return 0;
    }
    if (!(rule->wildcard_mask & ND_CLOCK_WILD_SECOND)
        && rule->second != time->second) {
        return 0;
    }
    return 1;
}

void nd_clock_config_init(nd_clock_config_t *config)
{
    if (config != NULL) {
        memset(config, 0, sizeof(*config));
        config->clock_base = 1;
    }
}

int nd_clock_dst_active(const nd_clock_config_t *config,
                        const nd_datetime_t *local_time)
{
    if (config == NULL || local_time == NULL || !config->daylight_enabled) {
        return 0;
    }
    if (rule_matches(&config->daylight_begin, local_time)) {
        return 1;
    }
    if (rule_matches(&config->daylight_end, local_time)) {
        return 0;
    }
    int begin = time_key(&config->daylight_begin);
    int end = time_key(&config->daylight_end);
    int now = local_key(local_time);
    if (begin == end) {
        return 0;
    }
    if (begin < end) {
        return now >= begin && now < end;
    }
    return now >= begin || now < end;
}

int nd_clock_effective_deviation(const nd_clock_config_t *config,
                                 const nd_datetime_t *local_time,
                                 int16_t *deviation)
{
    if (config == NULL || local_time == NULL || deviation == NULL) {
        return -1;
    }
    *deviation = config->time_zone;
    if (nd_clock_dst_active(config, local_time)) {
        *deviation = (int16_t)(*deviation + config->daylight_deviation);
    }
    return 0;
}

int nd_clock_adjust_to_quarter(nd_datetime_t *time)
{
    if (time == NULL) {
        return -1;
    }
    uint16_t minutes = (uint16_t)(time->minute + (time->second >= 30 ? 1 : 0));
    uint16_t rounded = (uint16_t)(((minutes + 7u) / 15u) * 15u);
    time->second = 0;
    if (rounded >= 60) {
        time->minute = (uint8_t)(rounded - 60);
        if (nd_calendar_add_seconds(time, 3600, time) != 0) {
            return -1;
        }
    } else {
        time->minute = (uint8_t)rounded;
    }
    return 0;
}

int nd_clock_adjust_to_minute(nd_datetime_t *time)
{
    if (time == NULL) {
        return -1;
    }
    if (time->second >= 30
        && nd_calendar_add_seconds(time, 60, time) != 0) {
        return -1;
    }
    time->second = 0;
    return 0;
}

int nd_clock_adjust_to_period(nd_datetime_t *time, uint16_t period_seconds)
{
    if (time == NULL || period_seconds == 0) {
        return -1;
    }
    uint32_t seconds = (uint32_t)time->hour * 3600u
        + (uint32_t)time->minute * 60u + time->second;
    uint32_t rem = seconds % period_seconds;
    if (rem >= period_seconds / 2u) {
        seconds += period_seconds - rem;
    } else {
        seconds -= rem;
    }
    nd_datetime_t base = *time;
    base.hour = 0;
    base.minute = 0;
    base.second = 0;
    return nd_calendar_add_seconds(&base, (int32_t)seconds, time);
}

int nd_clock_shift(nd_datetime_t *time, int32_t seconds)
{
    return nd_calendar_add_seconds(time, seconds, time);
}
