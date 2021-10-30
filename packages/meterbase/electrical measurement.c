
#include "nidavellir.h"
#include "applications.h"

#define MAX_VOLTAGE  400000 //400v
#define MIN_VOLTAGE  50000 //50v
#define UN_VOLTAGE   220000 //220v


    //uint32 voltage_ac;
    //uint32 current_l;
    //uint32 active_power;
    //uint32 current_n;
    //uint32 frequency;
    //uint32 power_factor;
    //uint32 temperature;
    //uint32 voltage_battery;


    float k_u;
    float k_il;
    float k_in;
    float k_p;
    
    
void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    int32_t temp = 0;
    
    
    
    mbase_control_block->voltage_ac = temp * mbase_control_block->k_u;
    if(mbase_control_block->voltage_ac > MAX_VOLTAGE)
    {
        mbase_control_block->voltage_ac = UN_VOLTAGE;
    }
    if(mbase_control_block->voltage_ac < MIN_VOLTAGE)
    {
        mbase_control_block->voltage_ac = 0;
    }    
    
    
    
    
}








