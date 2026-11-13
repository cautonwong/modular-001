#ifndef ND_DOMAIN_EVENTS_MAC_SUSPEND_H
#define ND_DOMAIN_EVENTS_MAC_SUSPEND_H

#include <stdint.h>

#include "domain/common/calendar.h"

typedef struct {
    uint16_t error_count;
    uint8_t suspended;
    uint8_t suspend_date_set;
    uint8_t suspend_day;
    uint8_t suspend_month;
    uint16_t suspend_year;
    uint8_t last_date_set;
    uint8_t last_day;
    uint8_t last_month;
    uint16_t last_year;
} nd_mac_suspend_state_t;

/* Counts one MAC error and suspends when the daily limit is reached. */
void nd_mac_suspend_add(nd_mac_suspend_state_t *state, uint16_t limit,
                        const nd_datetime_t *now);

/* Applies the daily rollover. Returns 1 when the caller should clear its
 * password-error counter, which shares the same day boundary. */
uint8_t nd_mac_suspend_step(nd_mac_suspend_state_t *state, uint16_t limit,
                            const nd_datetime_t *now);

#endif
