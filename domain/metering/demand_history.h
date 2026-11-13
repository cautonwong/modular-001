#ifndef ND_DOMAIN_METERING_DEMAND_HISTORY_H
#define ND_DOMAIN_METERING_DEMAND_HISTORY_H

#include <stdint.h>

#include "domain/common/calendar.h"
#include "domain/metering/demand.h"

/*
 * IDIS demand attributes that the existing nd_demand_snapshot_t does not
 * carry. The current value stays in nd_demand_t; this is history/status only.
 */
typedef struct {
    uint32_t last_value;
    uint32_t last_fee_value;
    nd_datetime_t last_capture_time;
    nd_datetime_t period_start;
    uint8_t status;
} nd_demand_history_t;

void nd_demand_history_init(nd_demand_history_t *history);
int nd_demand_history_capture(nd_demand_history_t *history,
                              const nd_demand_snapshot_t *snapshot,
                              const nd_datetime_t *capture_time,
                              const nd_datetime_t *period_start,
                              uint8_t status);
int nd_demand_history_reset(nd_demand_history_t *history);

#endif
