#ifndef ND_DOMAIN_TARIFF_ACTIVITY_CALENDAR_H
#define ND_DOMAIN_TARIFF_ACTIVITY_CALENDAR_H

#include <stdint.h>
#include "domain/common/calendar.h"

#define ND_CALENDAR_NAME_MAX 8

/* Wildcard bits for fields that IDIS allows to be "not specified". */
#define ND_CALENDAR_WILD_YEAR 0x01u
#define ND_CALENDAR_WILD_MONTH 0x02u
#define ND_CALENDAR_WILD_DAY 0x04u
#define ND_CALENDAR_WILD_HOUR 0x08u
#define ND_CALENDAR_WILD_MINUTE 0x10u
#define ND_CALENDAR_WILD_SECOND 0x20u

typedef struct {
    uint8_t len;
    uint8_t data[ND_CALENDAR_NAME_MAX];
} nd_calendar_name_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t wildcard_mask;
} nd_calendar_datetime_t;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t hundredths;
} nd_time_of_day_t;

typedef struct {
    nd_calendar_name_t name;
    nd_calendar_datetime_t start;
    nd_calendar_name_t week;
} nd_calendar_season_t;

typedef struct {
    nd_calendar_name_t name;
    uint8_t day_ids[7]; /* Monday..Sunday */
} nd_calendar_week_t;

typedef struct {
    nd_time_of_day_t start;
    uint8_t script_ln[6];
    uint16_t script_selector;
} nd_calendar_action_t;

typedef struct {
    uint8_t day_id;
    uint8_t action_count;
    const nd_calendar_action_t *actions;
} nd_calendar_day_t;

typedef struct {
    nd_calendar_datetime_t day;
    uint8_t day_id;
} nd_calendar_special_day_t;

/*
 * A bank is a view over target-owned storage. The domain never allocates the
 * season/week/day arrays; activation copies one bank to the other.
 */
typedef struct {
    nd_calendar_name_t name;
    uint8_t season_count;
    uint8_t week_count;
    uint8_t day_count;
    const nd_calendar_season_t *seasons;
    const nd_calendar_week_t *weeks;
    const nd_calendar_day_t *days;
    uint8_t special_day_count;
    const nd_calendar_special_day_t *special_days;
} nd_calendar_bank_t;

typedef struct {
    nd_calendar_bank_t active;
    nd_calendar_bank_t passive;
    nd_calendar_datetime_t activate_time;
    uint8_t has_passive;
} nd_activity_calendar_t;

int nd_calendar_match_datetime(const nd_calendar_datetime_t *pattern,
                               const nd_datetime_t *now);
int nd_calendar_match_date(const nd_calendar_datetime_t *pattern,
                           const nd_datetime_t *now);
const nd_calendar_season_t *nd_calendar_active_season(
    const nd_activity_calendar_t *calendar, const nd_datetime_t *now);
const nd_calendar_week_t *nd_calendar_find_week(const nd_calendar_bank_t *bank,
                                                const nd_calendar_name_t *name);
uint8_t nd_calendar_day_id(const nd_calendar_bank_t *bank,
                           const nd_datetime_t *now);
const nd_calendar_day_t *nd_calendar_find_day(const nd_calendar_bank_t *bank,
                                              uint8_t day_id);
const nd_calendar_action_t *nd_calendar_select_action(
    const nd_calendar_day_t *day, const nd_datetime_t *now);
int nd_calendar_set_passive(nd_activity_calendar_t *calendar,
                            const nd_calendar_bank_t *passive,
                            const nd_calendar_datetime_t *activate_time);
int nd_calendar_activate(nd_activity_calendar_t *calendar);
int nd_calendar_activation_due(const nd_activity_calendar_t *calendar,
                               const nd_datetime_t *now);

#endif
