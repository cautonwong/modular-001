
#include "nidavellir.h"
#include "applications.h"


#define Max_tou_fee 12

#define  ADD_POS    0x01
#define  DEC_POS    0x02
#define  ADD_NEG    0x04
#define  DEC_NEG    0x08

static uint32_t ec_pluse_cnt = 0;

void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    uint8_t fee_index = 0;
    uint32_t s_index;

    
    if((mbase_control_block->TOUNo == 0) || (mbase_control_block->TOUNo> Max_tou_fee))
    {
        fee_index = Max_tou_fee-1;
    }
    else
    {
        fee_index = mbase_control_block->TOUNo;
    }
    
    if(mbase_control_block->p_pluse > 0)
    {
        mbase_control_block->active_p_dec[0] += mbase_control_block->p_pluse;
        mbase_control_block->active_p_dec[fee_index] += mbase_control_block->p_pluse;
        mbase_control_block->p_pluse = 0;
    }
    
    //费率正电能，月度费率电能
    if(mbase_control_block->active_p_dec[fee_index] >= mbase_control_block->meter_const)
    {
        mbase_control_block->active_p[fee_index] += 1;
        mbase_control_block->month_active_p[fee_index] += 1;
        mbase_control_block->active_p_dec[fee_index] -= mbase_control_block->meter_const;
    }    
    //总正电能，总月度电能
    if(mbase_control_block->active_p_dec[0] >= mbase_control_block->meter_const)
    {
        mbase_control_block->active_p[0] += 1;
        mbase_control_block->month_active_p[0] += 1;
        mbase_control_block->active_p_dec[0] -= mbase_control_block->meter_const;
        
        s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
        offsetof(struct mbase_control_block_t, active_p));
        nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_p, 52);
    }

    
    if(mbase_control_block->n_pluse > 0)
    {
        mbase_control_block->active_n_dec[0] += mbase_control_block->n_pluse;
        mbase_control_block->active_n_dec[fee_index] += mbase_control_block->n_pluse;
        mbase_control_block->n_pluse = 0;
    }

    if(mbase_control_block->active_n_dec[fee_index] >= mbase_control_block->meter_const)
    {
        mbase_control_block->active_n[fee_index] += 1;
        mbase_control_block->month_active_n[fee_index] += 1;
        mbase_control_block->active_n_dec[fee_index] -= mbase_control_block->meter_const;
    }
    
    if(mbase_control_block->active_n_dec[0] >= mbase_control_block->meter_const)
    {
        mbase_control_block->active_n[0] += 1;
        mbase_control_block->month_active_n[0] += 1;
        mbase_control_block->active_n_dec[0] -= mbase_control_block->meter_const;
        
       s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
       offsetof(struct mbase_control_block_t, active_n));
       nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_n, 52);
    }
    
}

void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline)
{
    ec_pluse_cnt = 0;

    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    mbase_control_block->meter_const = 2000;
    mbase_control_block->active_energy_cal_word = 0x05;

}


void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline)
{
    Energy_Meter(instance , pipeline);
    /*
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_p_dec));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_p_dec, 26);
    
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_n_dec));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_n_dec, 26);
    */
}

int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t type)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    
    uint8_t word = mbase_control_block->active_energy_cal_word;
    
    if((mbase_control_block->meter_const==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(word & ADD_POS)
    {
        energy_kwh += mbase_control_block->active_p[index];
        energy_pulse += mbase_control_block->active_p_dec[index];
    }
    if(word & DEC_POS)
    {
        energy_kwh -= mbase_control_block->active_p[index];
        energy_pulse -= mbase_control_block->active_p_dec[index];
    }
    if(word & ADD_NEG)
    {
        energy_kwh += mbase_control_block->active_n[index];
        energy_pulse += mbase_control_block->active_n_dec[index];
    }
    if(word & DEC_NEG)
    {
        energy_kwh -= mbase_control_block->active_n[index];
        energy_pulse -= mbase_control_block->active_n_dec[index];
    }
    
    
    if(type == 1)
    {
        energy_kwh = energy_pulse*10000 / mbase_control_block->meter_const;//0.0001kwh
    }
    else
    {
        energy_kwh = energy_kwh*100;//0.01kwh
    }
    
    return energy_kwh;
}


int32_t Get_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t type , uint8_t pn)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    
    if((mbase_control_block->meter_const==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(pn ==0)
    {
        energy_kwh += mbase_control_block->active_p[index];
        energy_pulse += mbase_control_block->active_p_dec[index];
    }
    else
    {
        energy_kwh += mbase_control_block->active_n[index];
        energy_pulse += mbase_control_block->active_n_dec[index];
    }

    if(type == 1)
    {
        energy_kwh = energy_pulse*10000 / mbase_control_block->meter_const;//0.0001kwh
    }
    else
    {
        energy_kwh = energy_kwh*100;//0.01kwh
    }
    
    return energy_kwh;
}


int32_t Get_Month_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t pn)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    
    if((mbase_control_block->meter_const==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    
    if(pn ==0)
    {
        energy_kwh += mbase_control_block->month_active_p[index];
        energy_pulse += mbase_control_block->month_active_p[index];
    }
    else
    {
        energy_kwh += mbase_control_block->month_active_n[index];
        energy_pulse += mbase_control_block->month_active_n[index];
    }

    energy_kwh = energy_kwh*100;//0.01kwh

    return energy_kwh;
}

void Energy_Pluse_Count(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    //DISABLE_ALL_INT();
    if((ec_pluse_cnt < 64) && (ec_pluse_cnt > 0))
    {
        if(mbase_control_block->power_dir == 0)
        {
            mbase_control_block->p_pluse += ec_pluse_cnt;
        }
        else
        {
            mbase_control_block->n_pluse += ec_pluse_cnt;
        }
        ec_pluse_cnt = 0;
    }
    //ENABLE_ALL_INT();
}

void Energy_Pluse_Add(void)
{
    ec_pluse_cnt++;
}



void Energy_clr(struct instance_t* instance)
{
    uint8_t fee_index = 0;
    uint32_t s_index;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    for(fee_index = 0; fee_index < MAX_FEE_NO ; fee_index++)
    {
        mbase_control_block->active_n[fee_index] = 0x00;
        mbase_control_block->active_n_dec[fee_index] = 0x00;
        
        mbase_control_block->active_p[fee_index] = 0x00;
        mbase_control_block->active_p_dec[fee_index] = 0x00;
          
        mbase_control_block->month_active_p[fee_index] = 0x00;
        mbase_control_block->month_active_n[fee_index] = 0x00;
    }
    
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_p));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_p, (MAX_FEE_NO+1)<<2);
    
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_n));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_control_block->active_n, (MAX_FEE_NO+1)<<2);

}
