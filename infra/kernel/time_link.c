#include "nidavellir.h"
#include "type_def.h"
#include "time_link.h"
#include "cpu_surport.h"


struct tick_timer_list_t
{
    uint8_t (*tickfunc)(void *);
    void                 *tickpara;
    uint32_t             timeout;
    uint32_t             dlyctr;
    uint8_t                stoped;
};

static struct tick_timer_list_t tick_timer_list[TIME_OBJ_NUM];






uint8_t ndv_timer_create(struct tm_list_init_t *tm_init)
{    
    if(tm_init->tm_id < TIME_OBJ_NUM)
    {
        tick_timer_list[tm_init->tm_id].tickfunc = tm_init->tickfunc;
        tick_timer_list[tm_init->tm_id].tickpara = tm_init->tickpara;
        tick_timer_list[tm_init->tm_id].timeout = tm_init->timeout;
        tick_timer_list[tm_init->tm_id].dlyctr = tm_init->timeout;
        tick_timer_list[tm_init->tm_id].stoped = tm_init->stoped;
        return true;
    }
    
    return false;
}


uint8_t ndv_timer_start(uint8_t timerid)
{
    if(timerid < TIME_OBJ_NUM)
    {
        if(tick_timer_list[timerid].tickfunc != 0)
        {
            tick_timer_list[timerid].stoped = 0;
            tick_timer_list[timerid].dlyctr = tick_timer_list[timerid].timeout + 1;
            return true;
        }
    }
    return false;
}


uint8_t ndv_timer_stop(uint8_t timerid)
{
    if(timerid < TIME_OBJ_NUM)
    {
        if(tick_timer_list[timerid].tickfunc != 0)
        {
            tick_timer_list[timerid].stoped = 1;
            return true;
        }
    }
    return false;
}


void ndv_timer_tick(void *para)
{
    uint8_t i;
    
    for(i=0;i<TIME_OBJ_NUM;i++)
    {
        if((tick_timer_list[i].tickfunc == 0) || (tick_timer_list[i].timeout == 0))
        {
           continue;
        }
        if (tick_timer_list[i].stoped == 0) 
        {
            tick_timer_list[i].dlyctr--;   

            if (tick_timer_list[i].dlyctr == 0)  
            {
                tick_timer_list[i].dlyctr = tick_timer_list[i].timeout;
           
                if (!tick_timer_list[i].tickfunc(tick_timer_list[i].tickpara))
                {
                    tick_timer_list[i].stoped = 1;
                }
            }
        }
    }
}


uint8_t ndv_timer_start_one(uint8_t timerid, uint32_t count)
{
    if(timerid < TIME_OBJ_NUM)
    {
        if(tick_timer_list[timerid].tickfunc != 0)
        {
            tick_timer_list[timerid].stoped = 0;
            tick_timer_list[timerid].timeout = count;
            tick_timer_list[timerid].dlyctr  = tick_timer_list[timerid].timeout + 1;
            return true;
        }
    }
    return false;
} 

void ndv_timer_reset(void)
{
    uint8_t i;
    for(i=0;i<TIME_OBJ_NUM;i++)
    {
        tick_timer_list[i].tickfunc = 0;
        tick_timer_list[i].tickpara = 0;
        tick_timer_list[i].timeout = 0;
        tick_timer_list[i].dlyctr = 0;
        tick_timer_list[i].stoped = 0;
    }
}

void ndv_timer_init(void)
{
    struct isr_reg_ctx_t IsrReg;

    ndv_timer_reset();
    IsrReg.IsrFunctionRegister = ndv_timer_tick;
    IsrReg.para = (void*)(0);
    Isr_handle_regsit((struct isr_reg_ctx_t*)&IsrReg , IRQ_SYSTICK);
    cm0_systick_init(1);
}

/** @} */
