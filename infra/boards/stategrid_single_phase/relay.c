
#include "nidavellir.h"
#include "type_def.h"
#include "infra/boards/stategrid_single_phase/relay_pins.h"
#include "time_link.h"

#define   RELAY_PLUSE_WD  8


static uint8_t shut_pluse_driver(void *ptr)
{
    nd_relay_on_pulse(0);
    nd_relay_off_pulse(0);
    
    //(void*)ptr;
    
    return 0;
}




uint8_t Magnetic_hold_relay_Init(void)
{
   struct tm_list_init_t tm;

   tm.timeout = RELAY_PLUSE_WD;
   tm.tickpara = 0;
   tm.tickfunc = shut_pluse_driver;
   tm.tm_id = RELAY_TM;
   tm.stoped = true;
   
   ndv_timer_create(&tm);
   
   return true;
}


uint8_t Magnetic_hold_relay_connect(void)
{
    
    nd_relay_on_pulse(1);
    
    ndv_timer_start_one(RELAY_TM, RELAY_PLUSE_WD);
    
    return true;
}



uint8_t Magnetic_hold_relay_disconnect(void)
{
    nd_relay_off_pulse(1);
  
    ndv_timer_start_one(RELAY_TM, RELAY_PLUSE_WD);
    
    return true;
}





/** @} */