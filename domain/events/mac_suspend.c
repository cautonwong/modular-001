#include "domain/events/mac_suspend.h"

static uint8_t same_date(uint8_t day, uint8_t month, uint16_t year,
                         const nd_datetime_t *now)
{
    return (uint8_t)(day == now->day && month == now->month
                     && year == now->year);
}

void nd_mac_suspend_add(nd_mac_suspend_state_t *state, uint16_t limit,
                        const nd_datetime_t *now)
{
    if (state->error_count >= limit) {
        return;
    }
    state->error_count++;
    if (state->error_count == limit) {
        state->suspended = 1;
        state->suspend_date_set = 1;
        state->suspend_day = now->day;
        state->suspend_month = now->month;
        state->suspend_year = now->year;
    }
}

uint8_t nd_mac_suspend_step(nd_mac_suspend_state_t *state, uint16_t limit,
                            const nd_datetime_t *now)
{
    uint8_t midnight = (uint8_t)(now->hour == 0 && now->minute == 0
                                 && now->second == 0);
    uint8_t day_changed =
        state->last_date_set
        && !same_date(state->last_day, state->last_month, state->last_year,
                      now);
    uint8_t suspend_date_changed =
        state->suspend_date_set
        && !same_date(state->suspend_day, state->suspend_month,
                      state->suspend_year, now);

    if (state->error_count > 0
        && (day_changed || midnight || suspend_date_changed)) {
        state->error_count = 0;
        state->suspended = 0;
        state->suspend_date_set = 0;
    }

    state->last_date_set = 1;
    state->last_day = now->day;
    state->last_month = now->month;
    state->last_year = now->year;
    state->suspended = (uint8_t)(state->error_count == limit);

    return (uint8_t)(day_changed || midnight);
}
