#ifndef ND_DOMAIN_FREEZE_BACKFILL_H
#define ND_DOMAIN_FREEZE_BACKFILL_H

#include <stdint.h>

#include "domain/common/calendar.h"
#include "domain/freeze/schedule.h"

#define ND_FREEZE_DAY_CATCH_UP_MAX 7
#define ND_FREEZE_BILL_CATCH_UP_MONTHS 12

typedef struct {
    nd_datetime_t when;
    uint8_t slot;
} nd_freeze_bill_event_t;

/* Fills the freeze times missed between power-down and now, at most one
 * window of seven periods. Returns the number of timestamps written. */
uint8_t nd_freeze_day_backfill(const nd_datetime_t *now,
                               const nd_datetime_t *power_down,
                               const nd_datetime_t *last_day,
                               uint16_t period_days,
                               nd_datetime_t *out, uint8_t max);

/* Fills the settlement times missed between power-down and now, scanning at
 * most 12 months back. Returns the number of events written. */
uint8_t nd_freeze_bill_backfill(const nd_datetime_t *now,
                                const nd_datetime_t *power_down,
                                const nd_datetime_t *last_bill,
                                const nd_freeze_bill_date_t *bill_dates,
                                nd_freeze_bill_event_t *out, uint8_t max);

#endif
