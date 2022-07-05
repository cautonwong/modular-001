
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "pin_function_def.h"
#include "time_link.h"

#define   RELAY_PLUSE_WD  8


static uint8_t shut_pluse_driver(void *ptr)
{
    REALY_ON_DIS_ACTIVE();
    REALY_OFF_DIS_ACTIVE();
    
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
    
    REALY_ON_ACTIVE();
    
    ndv_timer_start_one(RELAY_TM, RELAY_PLUSE_WD);
    
    return true;
}



uint8_t Magnetic_hold_relay_disconnect(void)
{
    REALY_OFF_ACTIVE();
  
    ndv_timer_start_one(RELAY_TM, RELAY_PLUSE_WD);
    
    return true;
}





/** @} */