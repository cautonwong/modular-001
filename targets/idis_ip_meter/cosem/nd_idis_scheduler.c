#include "targets/idis_ip_meter/cosem/nd_idis_scheduler.h"

#include <string.h>

void nd_idis_scheduler_init(nd_idis_scheduler_state_t *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

static int scheduler_one(nd_idis_scheduler_state_t *state,
                         const nd_idis_calendar_state_t *calendar,
                         const nd_script_state_t *scripts,
                         const nd_cosem_port_t *port, uint8_t client_sap,
                         const nd_datetime_t *now, uint8_t calendar_index)
{
    if (state == NULL || calendar == NULL || scripts == NULL || port == NULL
        || now == NULL) {
        return -1;
    }
    const nd_calendar_bank_t *bank = &calendar->calendar.active;
    uint8_t day_id = nd_calendar_day_id(bank, now);
    if (day_id == 0) {
        return 0;
    }
    const nd_calendar_day_t *day = nd_calendar_find_day(bank, day_id);
    if (day == NULL || day->actions == NULL) {
        return 0;
    }
    const nd_calendar_action_t *action = nd_calendar_select_action(day, now);
    if (action == NULL || action->start.hour != now->hour
        || action->start.minute != now->minute
        || action->start.second != now->second) {
        return 0;
    }
    uint8_t action_index = (uint8_t)(action - day->actions);
    if (state->valid && state->year == now->year && state->month == now->month
        && state->day == now->day && state->hour == now->hour
        && state->minute == now->minute
        && state->action_index == action_index
        && state->calendar_index == calendar_index) {
        return 0;
    }
    /* Mark first so a failing script is not retried on every tick. */
    state->year = now->year;
    state->month = now->month;
    state->day = now->day;
    state->hour = now->hour;
    state->minute = now->minute;
    state->action_index = action_index;
    state->calendar_index = calendar_index;
    state->valid = 1;
    nd_obis_t ln = {
        .a = action->script_ln[0],
        .b = action->script_ln[1],
        .c = action->script_ln[2],
        .d = action->script_ln[3],
        .e = action->script_ln[4],
        .f = action->script_ln[5],
    };
    if (nd_idis_script_execute(scripts, port, client_sap, &ln,
                               action->script_selector)
        != ND_OK) {
        return -1;
    }
    state->executed++;
    return 1;
}

int nd_idis_scheduler_on_second(nd_idis_scheduler_state_t *state,
                                const nd_idis_calendar_state_t *calendar,
                                const nd_script_state_t *scripts,
                                const nd_cosem_port_t *port,
                                uint8_t client_sap,
                                const nd_datetime_t *now)
{
    return nd_idis_scheduler_on_second_multi(state, &calendar, 1, scripts,
                                             port, client_sap, now);
}

int nd_idis_scheduler_on_second_multi(
    nd_idis_scheduler_state_t *state,
    const nd_idis_calendar_state_t *const *calendars, uint8_t count,
    const nd_script_state_t *scripts, const nd_cosem_port_t *port,
    uint8_t client_sap, const nd_datetime_t *now)
{
    if (calendars == NULL || count == 0) {
        return -1;
    }
    int fired = 0;
    for (uint8_t index = 0; index < count; index++) {
        int result = scheduler_one(state, calendars[index], scripts, port,
                                   client_sap, now, index);
        if (result < 0) {
            return -1;
        }
        fired += result;
    }
    return fired > 0 ? 1 : 0;
}
