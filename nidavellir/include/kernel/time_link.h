/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TIME_LINK_H
#define _TIME_LINK_H




enum tick_time_id_t 
{
    SYS_HOOK_TM = 0,
    RS4851_TM,
    MODULE_TM,
    IR_TM,
    RELAY_TM,
    METER_COVER_TM,
    TIME_OBJ_NUM,
};

struct tm_list_init_t
{
    uint32_t timeout;
    void *tickpara;
    uint8_t (*tickfunc)(void *);
    uint8_t tm_id;
    uint8_t stoped;
};

extern uint8_t ndv_timer_create(struct tm_list_init_t *tm_init);

extern uint8_t ndv_timer_start(uint8_t timerid);

extern uint8_t ndv_timer_stop(uint8_t timerid);

extern void ndv_timer_tick(void *para);

extern uint8_t ndv_timer_start_one(uint8_t timerid, uint32_t count);

extern void ndv_timer_init(void);

#endif
