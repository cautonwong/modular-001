#ifndef ND_IDIS_SCHEDULER_H
#define ND_IDIS_SCHEDULER_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/calendar.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_calendar.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_script_table.h"

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t action_index;
    uint8_t calendar_index;
    uint8_t valid;
    uint32_t executed;
} nd_idis_scheduler_state_t;

void nd_idis_scheduler_init(nd_idis_scheduler_state_t *state);

/*
 * Executes the activity calendar action whose start time equals now, once per
 * day and action. Returns 1 when a script ran, 0 when nothing is due, -1 on a
 * script failure.
 */
int nd_idis_scheduler_on_second(nd_idis_scheduler_state_t *state,
                                const nd_idis_calendar_state_t *calendar,
                                const nd_script_state_t *scripts,
                                const nd_cosem_port_t *port,
                                uint8_t client_sap,
                                const nd_datetime_t *now);

/* Runs the same action matching for every configured calendar instance. */
int nd_idis_scheduler_on_second_multi(
    nd_idis_scheduler_state_t *state,
    const nd_idis_calendar_state_t *const *calendars, uint8_t count,
    const nd_script_state_t *scripts, const nd_cosem_port_t *port,
    uint8_t client_sap, const nd_datetime_t *now);

#endif
