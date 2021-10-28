
#include "nidavellir.h"
#include "meterbase.h"








void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    uint8 fee_index = 0;
    int16 p_pluse , n_pluse;
    
    if((mbase_control_block->Fee.TOUNo == 0) || (mbase_control_block->Fee.TOUNo> Max_tou_fee))
    {
        fee_index = Max_tou_fee-1;
    }
    else
    {
        fee_index = mbase_control_block->Fee.TOUNo;
    }
    
    p_pluse = mbase_control_block->emu_pluse.current_active_p - mbase_control_block->emu_pluse.last_active_p;
    n_pluse = mbase_control_block->emu_pluse.current_active_n - mbase_control_block->emu_pluse.last_active_n;
    
    if(p_pluse > 0)
    {
        mbase_control_block->energy[0].active_p_dec += p_pluse;
    }
    if(mbase_control_block->energy[0].active_p_dec > mbase_control_block->meter_const)
    {
        mbase_control_block->energy[0].active_p += 1;
        mbase_control_block->energy[0].active_p_dec -= mbase_control_block->meter_const;
        mbase_control_block->energy[0].month_active_p += 1;
    }
    if(n_pluse > 0)
    {
        mbase_control_block->energy[0].active_p_dec += n_pluse;
    }    
    if(mbase_control_block->energy[0].active_n_dec > mbase_control_block->meter_const)
    {
        mbase_control_block->energy[0].active_n += 1;
        mbase_control_block->energy[0].active_n_dec -= mbase_control_block->meter_const;
        mbase_control_block->energy[0].month_active_n += 1;
    }
    
    if(p_pluse > 0)
    {
        mbase_control_block->energy[fee_index].active_p_dec += p_pluse;
    }
    if(mbase_control_block->energy[fee_index].active_p_dec > mbase_control_block->meter_const)
    {
        mbase_control_block->energy[fee_index].active_p += 1;
        mbase_control_block->energy[fee_index].active_p_dec -= mbase_control_block->meter_const;
        mbase_control_block->energy[fee_index].month_active_p += 1;
    }
    if(n_pluse > 0)
    {
        mbase_control_block->energy[fee_index].active_p_dec += n_pluse;
    }    
    if(mbase_control_block->energy[fee_index].active_n_dec > mbase_control_block->meter_const)
    {
        mbase_control_block->energy[fee_index].active_n += 1;
        mbase_control_block->energy[fee_index].active_n_dec -= mbase_control_block->meter_const;
        mbase_control_block->energy[fee_index].month_active_n += 1;
    }
}








