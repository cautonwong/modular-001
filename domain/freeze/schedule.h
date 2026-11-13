#ifndef ND_DOMAIN_FREEZE_SCHEDULE_H
#define ND_DOMAIN_FREEZE_SCHEDULE_H

#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_FREEZE_MINUTE_SCHEMES 8
#define ND_FREEZE_BILL_DATES 3

/* One entry of the stored freeze period table: valid flag plus the interval
 * in the caller's unit (minutes for minute, hours for hour, days for day). */
typedef struct {
    uint8_t valid;
    uint16_t interval;
} nd_freeze_period_t;

/* A settlement day: day of month plus hour of day. 0x63, 0x99 and 0xFF mark
 * an unused slot. */
typedef struct {
    uint8_t day;
    uint8_t hour;
} nd_freeze_bill_date_t;

uint8_t nd_freeze_bill_date_valid(const nd_freeze_bill_date_t *bill);

typedef struct {
    uint16_t remaining;
    uint8_t armed;
} nd_freeze_instant_t;

/* Due on the minute boundary every interval*unit_minutes of wall clock since
 * 2000-01-01 00:00. */
uint8_t nd_freeze_period_due(const nd_datetime_t *now,
                             const nd_freeze_period_t *period,
                             uint16_t unit_minutes);

/* Due at 00:00:00 on the first day of the month. */
uint8_t nd_freeze_month_due(const nd_datetime_t *now);

/* Due at the scheduled day and hour, once per scheduled hour. The caller
 * passes the last recorded settlement time. */
uint8_t nd_freeze_bill_due(const nd_datetime_t *now,
                           const nd_datetime_t *last_bill,
                           const nd_freeze_bill_date_t *bill);

void nd_freeze_instant_arm(nd_freeze_instant_t *state, uint16_t delay_seconds);

/* Counts down one step per call and returns 1 on the call that reaches zero,
 * matching the legacy trigger field. */
uint8_t nd_freeze_instant_step(nd_freeze_instant_t *state);

#endif
