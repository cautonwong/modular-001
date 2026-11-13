#include "domain/tariff/activity_calendar.h"

#include <string.h>

static int name_equal(const nd_calendar_name_t *left,
                      const nd_calendar_name_t *right)
{
    return left->len == right->len
        && memcmp(left->data, right->data, left->len) == 0;
}

static int datetime_key(const nd_calendar_datetime_t *value,
                        const nd_datetime_t *now)
{
    if (value->year != now->year) {
        return value->year < now->year ? -1 : 1;
    }
    if (value->month != now->month) {
        return value->month < now->month ? -1 : 1;
    }
    if (value->day != now->day) {
        return value->day < now->day ? -1 : 1;
    }
    if (value->hour != now->hour) {
        return value->hour < now->hour ? -1 : 1;
    }
    if (value->minute != now->minute) {
        return value->minute < now->minute ? -1 : 1;
    }
    if (value->second != now->second) {
        return value->second < now->second ? -1 : 1;
    }
    return 0;
}

int nd_calendar_match_datetime(const nd_calendar_datetime_t *pattern,
                               const nd_datetime_t *now)
{
    if (pattern == NULL || now == NULL) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_YEAR)
        && pattern->year != now->year) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_MONTH)
        && pattern->month != now->month) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_DAY)
        && pattern->day != now->day) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_HOUR)
        && pattern->hour != now->hour) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_MINUTE)
        && pattern->minute != now->minute) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_SECOND)
        && pattern->second != now->second) {
        return 0;
    }
    return 1;
}

int nd_calendar_match_date(const nd_calendar_datetime_t *pattern,
                           const nd_datetime_t *now)
{
    if (pattern == NULL || now == NULL) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_YEAR)
        && pattern->year != now->year) {
        return 0;
    }
    if (!(pattern->wildcard_mask & ND_CALENDAR_WILD_MONTH)
        && pattern->month != now->month) {
        return 0;
    }
    return (pattern->wildcard_mask & ND_CALENDAR_WILD_DAY)
        || pattern->day == now->day;
}

const nd_calendar_season_t *nd_calendar_active_season(
    const nd_activity_calendar_t *calendar, const nd_datetime_t *now)
{
    if (calendar == NULL || now == NULL) {
        return NULL;
    }
    const nd_calendar_bank_t *bank = &calendar->active;
    const nd_calendar_season_t *selected = NULL;
    for (uint8_t index = 0; index < bank->season_count; index++) {
        const nd_calendar_season_t *season = &bank->seasons[index];
        if (datetime_key(&season->start, now) <= 0) {
            selected = season;
        }
    }
    return selected;
}

const nd_calendar_week_t *nd_calendar_find_week(const nd_calendar_bank_t *bank,
                                                const nd_calendar_name_t *name)
{
    if (bank == NULL || name == NULL) {
        return NULL;
    }
    for (uint8_t index = 0; index < bank->week_count; index++) {
        if (name_equal(&bank->weeks[index].name, name)) {
            return &bank->weeks[index];
        }
    }
    return NULL;
}

uint8_t nd_calendar_day_id(const nd_calendar_bank_t *bank,
                           const nd_datetime_t *now)
{
    if (bank == NULL || now == NULL) {
        return 0;
    }
    for (uint8_t index = 0; index < bank->special_day_count; index++) {
        if (nd_calendar_match_date(&bank->special_days[index].day, now)) {
            return bank->special_days[index].day_id;
        }
    }
    nd_activity_calendar_t view = {.active = *bank};
    const nd_calendar_season_t *season = nd_calendar_active_season(&view, now);
    if (season == NULL) {
        return 0;
    }
    const nd_calendar_week_t *week = nd_calendar_find_week(bank, &season->week);
    if (week == NULL) {
        return 0;
    }
    uint8_t weekday = nd_calendar_weekday(now->day, now->month, now->year);
    uint8_t index = (uint8_t)((weekday + 6u) % 7u);
    return week->day_ids[index];
}

const nd_calendar_day_t *nd_calendar_find_day(const nd_calendar_bank_t *bank,
                                              uint8_t day_id)
{
    if (bank == NULL) {
        return NULL;
    }
    for (uint8_t index = 0; index < bank->day_count; index++) {
        if (bank->days[index].day_id == day_id) {
            return &bank->days[index];
        }
    }
    return NULL;
}

const nd_calendar_action_t *nd_calendar_select_action(
    const nd_calendar_day_t *day, const nd_datetime_t *now)
{
    if (day == NULL || now == NULL) {
        return NULL;
    }
    const nd_calendar_action_t *selected = NULL;
    for (uint8_t index = 0; index < day->action_count; index++) {
        const nd_time_of_day_t *start = &day->actions[index].start;
        if (start->hour > now->hour
            || (start->hour == now->hour && start->minute > now->minute)
            || (start->hour == now->hour && start->minute == now->minute
                && start->second > now->second)) {
            break;
        }
        selected = &day->actions[index];
    }
    return selected;
}

int nd_calendar_set_passive(nd_activity_calendar_t *calendar,
                            const nd_calendar_bank_t *passive,
                            const nd_calendar_datetime_t *activate_time)
{
    if (calendar == NULL || passive == NULL || activate_time == NULL) {
        return -1;
    }
    calendar->passive = *passive;
    calendar->activate_time = *activate_time;
    calendar->has_passive = 1;
    return 0;
}

int nd_calendar_activate(nd_activity_calendar_t *calendar)
{
    if (calendar == NULL || !calendar->has_passive) {
        return -1;
    }
    calendar->active = calendar->passive;
    calendar->has_passive = 0;
    return 0;
}

int nd_calendar_activation_due(const nd_activity_calendar_t *calendar,
                               const nd_datetime_t *now)
{
    if (calendar == NULL || now == NULL || !calendar->has_passive) {
        return 0;
    }
    return nd_calendar_match_datetime(&calendar->activate_time, now)
        && datetime_key(&calendar->activate_time, now) <= 0;
}
