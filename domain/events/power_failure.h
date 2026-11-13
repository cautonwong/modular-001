#ifndef ND_DOMAIN_EVENTS_POWER_FAILURE_H
#define ND_DOMAIN_EVENTS_POWER_FAILURE_H

#include <stdint.h>

#include "domain/common/calendar.h"

typedef struct {
    uint16_t threshold_seconds; /* failures at or above this are "long" */
    uint16_t count;
    uint16_t long_count;
    uint32_t last_duration;
    uint8_t active;
    nd_datetime_t off_time;
} nd_power_failure_state_t;

void nd_power_failure_init(nd_power_failure_state_t *state,
                           uint16_t threshold_seconds);

/* Records the supply loss; a second call while active keeps the first time. */
int nd_power_failure_off(nd_power_failure_state_t *state,
                         const nd_datetime_t *now);

/* Records the supply return; returns the outage duration in seconds. */
uint32_t nd_power_failure_on(nd_power_failure_state_t *state,
                             const nd_datetime_t *now);

#endif
