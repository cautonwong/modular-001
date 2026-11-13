#include "domain/events/power_failure.h"

#include <string.h>

void nd_power_failure_init(nd_power_failure_state_t *state,
                           uint16_t threshold_seconds)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
        state->threshold_seconds = threshold_seconds;
    }
}

int nd_power_failure_off(nd_power_failure_state_t *state,
                         const nd_datetime_t *now)
{
    if (state == NULL || now == NULL) {
        return -1;
    }
    if (!state->active) {
        state->off_time = *now;
        state->active = 1;
    }
    return 0;
}

uint32_t nd_power_failure_on(nd_power_failure_state_t *state,
                             const nd_datetime_t *now)
{
    if (state == NULL || now == NULL || !state->active) {
        return 0;
    }
    int32_t minutes = (int32_t)(nd_calendar_total_minutes(now)
                                - nd_calendar_total_minutes(&state->off_time));
    int32_t seconds = minutes * 60 + (int32_t)now->second
        - (int32_t)state->off_time.second;
    uint32_t duration = seconds > 0 ? (uint32_t)seconds : 0;
    state->last_duration = duration;
    if (state->count < 0xFFFF) {
        state->count++;
    }
    if (duration >= state->threshold_seconds && state->long_count < 0xFFFF) {
        state->long_count++;
    }
    state->active = 0;
    return duration;
}
