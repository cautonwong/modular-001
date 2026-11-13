#ifndef ND_DOMAIN_EVENTS_POWER_EVENT_H
#define ND_DOMAIN_EVENTS_POWER_EVENT_H

#include <stdint.h>

/* The 0xAA suffix closes the event at power-on time instead of the stored
 * power-down time. */
uint8_t nd_event_close_uses_poweron(uint32_t oad);

typedef struct {
    uint32_t power_off_acc;
    uint32_t power_off_low;
    uint32_t cover_open_acc;
    uint32_t cover_open_low;
    uint32_t power_abnormal_acc;
    uint32_t power_abnormal_low;
} nd_event_power_resume_state_t;

/* Folds the low-power accumulators into the event duration and clears them. */
void nd_event_power_resume(nd_event_power_resume_state_t *state);

#endif
