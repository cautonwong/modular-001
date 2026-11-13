#include "domain/metering/demand_history.h"

#include <string.h>

void nd_demand_history_init(nd_demand_history_t *history)
{
    if (history != NULL) {
        memset(history, 0, sizeof(*history));
    }
}

int nd_demand_history_capture(nd_demand_history_t *history,
                              const nd_demand_snapshot_t *snapshot,
                              const nd_datetime_t *capture_time,
                              const nd_datetime_t *period_start,
                              uint8_t status)
{
    if (history == NULL || snapshot == NULL || capture_time == NULL) {
        return -1;
    }
    nd_datetime_t start;
    if (period_start != NULL) {
        start = *period_start;
    } else if (nd_calendar_add_seconds(capture_time,
                                       -(int32_t)snapshot->period_seconds,
                                       &start) != 0) {
        return -1;
    }
    history->last_value = snapshot->current_value;
    history->last_fee_value = snapshot->current_fee_value;
    history->last_capture_time = *capture_time;
    history->period_start = start;
    history->status = status;
    return 0;
}

int nd_demand_history_reset(nd_demand_history_t *history)
{
    if (history == NULL) {
        return -1;
    }
    memset(history, 0, sizeof(*history));
    return 0;
}
