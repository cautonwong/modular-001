#include "domain/freeze/backfill.h"

static uint32_t total_day(const nd_datetime_t *datetime)
{
    return nd_calendar_total_day(datetime->day, datetime->month, datetime->year);
}

static uint32_t total_seconds(const nd_datetime_t *datetime)
{
    return nd_calendar_total_minutes(datetime) * 60u + datetime->second;
}

uint8_t nd_freeze_day_backfill(const nd_datetime_t *now,
                               const nd_datetime_t *power_down,
                               const nd_datetime_t *last_day,
                               uint16_t period_days,
                               nd_datetime_t *out, uint8_t max)
{
    uint32_t now_day = total_day(now);
    uint32_t measured_from = total_day(power_down);
    uint32_t last = total_day(last_day);
    uint32_t window;
    uint32_t recent;
    uint8_t count = 0;

    if (period_days == 0) {
        period_days = 1;
    }
    if (now_day <= measured_from) {
        return 0;
    }

    window = (uint32_t)ND_FREEZE_DAY_CATCH_UP_MAX * period_days;
    if ((now_day - measured_from) / period_days > ND_FREEZE_DAY_CATCH_UP_MAX) {
        recent = now_day > window ? now_day - window + 1 : 1;
    } else {
        recent = measured_from + period_days;
    }

    while (recent <= now_day && count < max) {
        if (recent > last) {
            last = recent;
            nd_calendar_from_total_day(recent, &out[count]);
            count++;
        }
        recent += period_days;
    }
    return count;
}

uint8_t nd_freeze_bill_backfill(const nd_datetime_t *now,
                                const nd_datetime_t *power_down,
                                const nd_datetime_t *last_bill,
                                const nd_freeze_bill_date_t *bill_dates,
                                nd_freeze_bill_event_t *out, uint8_t max)
{
    nd_datetime_t cursor = *power_down;
    nd_datetime_t last = *last_bill;
    uint32_t now_seconds = total_seconds(now);
    uint32_t power_down_seconds = total_seconds(power_down);
    uint32_t last_seconds = total_seconds(&last);
    uint8_t count = 0;
    uint8_t month;

    cursor.second = 0;
    cursor.minute = 0;

    for (month = 0; month < ND_FREEZE_BILL_CATCH_UP_MONTHS; month++) {
        nd_datetime_t when[ND_FREEZE_BILL_DATES];
        uint8_t slot[ND_FREEZE_BILL_DATES];
        uint8_t n = 0;
        uint8_t i;

        for (i = 0; i < ND_FREEZE_BILL_DATES; i++) {
            nd_datetime_t candidate;
            uint32_t candidate_seconds;
            uint8_t j;

            if (!nd_freeze_bill_date_valid(&bill_dates[i])) {
                continue;
            }

            candidate = cursor;
            candidate.day = bill_dates[i].day;
            candidate.hour = bill_dates[i].hour;
            candidate.minute = 0;
            candidate.second = 0;
            candidate_seconds = total_seconds(&candidate);

            for (j = n; j > 0 && total_seconds(&when[j - 1]) > candidate_seconds; j--) {
                when[j] = when[j - 1];
                slot[j] = slot[j - 1];
            }
            when[j] = candidate;
            slot[j] = i;
            n++;
        }

        for (i = 0; i < n; i++) {
            uint32_t when_seconds = total_seconds(&when[i]);

            if (when_seconds > now_seconds) {
                return count;
            }
            if (when_seconds > last_seconds && when_seconds >= power_down_seconds) {
                if (count == max) {
                    return count;
                }
                out[count].when = when[i];
                out[count].slot = slot[i];
                count++;
                last = when[i];
                last_seconds = when_seconds;
            }
        }

        if (cursor.month < 12) {
            cursor.month++;
        } else {
            cursor.month = 1;
            cursor.year++;
        }
    }
    return count;
}
