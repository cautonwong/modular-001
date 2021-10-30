/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TIME_LINK_H
#define _TIME_LINK_H

extern uint8 ndv_timer_create(uint8 (*tickfunc)(void *), void *tickpara, uint16 timeout, uint8 stoped);

extern uint8 ndv_timer_start(uint8 timerid);

extern uint8 ndv_timer_stop(uint8 timerid);

extern void ndv_timer_tick(void);

extern uint8 ndv_timer_start_one(uint8 timerid, uint16 count);



#endif
