#include "domain/freeze/schedule.h"

#define EPOCH_MINUTES (24u * 60u)

static uint8_t minutes_since_epoch(const nd_datetime_t *now, uint32_t *out)
{
    uint32_t total = nd_calendar_total_minutes(now);

    if (total < EPOCH_MINUTES) {
        return 0;
    }
    *out = total - EPOCH_MINUTES;
    return 1;
}

uint8_t nd_freeze_period_due(const nd_datetime_t *now,
                             const nd_freeze_period_t *period,
                             uint16_t unit_minutes)
{
    uint32_t minutes;
    uint32_t step;

    if (!period->valid || period->interval == 0 || unit_minutes == 0) {
        return 0;
    }
    if (now->second != 0 || !minutes_since_epoch(now, &minutes)) {
        return 0;
    }
    step = (uint32_t)period->interval * unit_minutes;
    return (uint8_t)((minutes % step) == 0);
}

uint8_t nd_freeze_month_due(const nd_datetime_t *now)
{
    return (uint8_t)(now->second == 0 && now->minute == 0 && now->hour == 0
                     && now->day == 1);
}

uint8_t nd_freeze_bill_date_valid(const nd_freeze_bill_date_t *bill)
{
    if (bill->day == 0x63 || bill->day == 0x99 || bill->day == 0xFF) {
        return 0;
    }
    return (uint8_t)(bill->day >= 1 && bill->day <= 31 && bill->hour <= 23);
}

static uint32_t hour_index(const nd_datetime_t *datetime)
{
    return nd_calendar_total_day(datetime->day, datetime->month, datetime->year)
           * 24u + datetime->hour;
}

uint8_t nd_freeze_bill_due(const nd_datetime_t *now,
                           const nd_datetime_t *last_bill,
                           const nd_freeze_bill_date_t *bill)
{
    nd_datetime_t scheduled;
    uint32_t scheduled_hour;

    if (!nd_freeze_bill_date_valid(bill)) {
        return 0;
    }
    if (now->second != 0 || now->minute != 0) {
        return 0;
    }

    scheduled = *now;
    scheduled.day = bill->day;
    scheduled.hour = bill->hour;
    scheduled_hour = hour_index(&scheduled);

    return (uint8_t)(hour_index(now) == scheduled_hour
                     && hour_index(last_bill) != scheduled_hour);
}

void nd_freeze_instant_arm(nd_freeze_instant_t *state, uint16_t delay_seconds)
{
    state->remaining = delay_seconds;
    state->armed = 1;
}

uint8_t nd_freeze_instant_step(nd_freeze_instant_t *state)
{
    if (!state->armed) {
        return 0;
    }
    if (state->remaining > 0) {
        state->remaining--;
    }
    if (state->remaining != 0) {
        return 0;
    }
    state->armed = 0;
    return 1;
}
