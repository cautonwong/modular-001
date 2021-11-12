
#include "nidavellir.h"
#include "applications.h"


#define Max_tou_fee 12





void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    uint8 fee_index = 0;
    int16 p_pluse = 0 , n_pluse = 0;
    
    if((mbase_control_block->TOUNo == 0) || (mbase_control_block->TOUNo> Max_tou_fee))
    {
        fee_index = Max_tou_fee-1;
    }
    else
    {
        fee_index = mbase_control_block->TOUNo;
    }
    
    //p_pluse = mbase_control_block->emu_pluse.current_active_p - mbase_control_block->emu_pluse.last_active_p;
    //n_pluse = mbase_control_block->emu_pluse.current_active_n - mbase_control_block->emu_pluse.last_active_n;
    
    if(p_pluse > 0)
    {
        mbase_control_block->active_p_dec[0] += p_pluse;
        mbase_control_block->active_p_dec[fee_index] += p_pluse;
    }
    if(mbase_control_block->active_p_dec[0] > mbase_control_block->meter_const)
    {
        mbase_control_block->active_p[0] += 1;
        mbase_control_block->active_p_dec[0] -= mbase_control_block->meter_const;
        mbase_control_block->month_active_p[0] += 1;
        
        mbase_control_block->active_p[fee_index] += 1;
        mbase_control_block->active_p_dec[fee_index] -= mbase_control_block->meter_const;
        mbase_control_block->month_active_p[fee_index] += 1;        
    }
    if(n_pluse > 0)
    {
        mbase_control_block->active_n_dec[0] += n_pluse;
        mbase_control_block->active_n_dec[fee_index] += n_pluse;
    }    
    if(mbase_control_block->active_n_dec[0] > mbase_control_block->meter_const)
    {
        mbase_control_block->active_n[0] += 1;
        mbase_control_block->active_n_dec[0] -= mbase_control_block->meter_const;
        mbase_control_block->month_active_n[0] += 1;
        
        mbase_control_block->active_n[fee_index] += 1;
        mbase_control_block->active_n_dec[fee_index] -= mbase_control_block->meter_const;
        mbase_control_block->month_active_n[fee_index] += 1;        
    }

}








