
#include "nidavellir.h"
#include "applications.h"


#define Max_tou_fee 12

#define  ADD_POS    0x01
#define  DEC_POS    0x02
#define  ADD_NEG    0x04
#define  DEC_NEG    0x08

static uint32_t ec_pluse_cnt = 0;


//计算电能校验和
// type:0 电量   1 用电量
uint32_t __cal_EC_checksum(struct instance_t* instance, uint8_t type)
{
    uint8_t *buff;
    uint16_t len;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    switch(type)
    {
        case 0:
            buff =  (uint8_t *)&mbase_cb->active_p[0];
            len = (uint16_t)offsetof(struct mbase_control_block_t, Energy_chksum) - offsetof(struct mbase_control_block_t, active_p[0]);       
            break;

        case 1:
            buff =  (uint8_t *)&mbase_cb->MonthEnegery_dec;
            len = (uint16_t)offsetof(struct mbase_control_block_t, UseEnergy_chksum) - offsetof(struct mbase_control_block_t, MonthEnegery_dec);
            break;

        default:
            return 0;
            break;
    }

    return do_checksum(buff, len);
}

// 判断电能校验和             
// 正确返回1，错误返回-1
// type:0 电量   1 用电量
int judge_EC_checksum(struct instance_t* instance, uint8_t type)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t cs = 0;
    
    switch(type)
    {
        case 0:
            cs = mbase_cb->Energy_chksum;
            break;

        case 1:
            cs = mbase_cb->UseEnergy_chksum;
            break;

        default:
            return 0;
            break;
    }

    if (cs != __cal_EC_checksum(instance, type))
    {
        return -1;
    }

    return 1;
}


// 更新电能校验和             
// type:0 电量   1 用电量
int updata_EC_checksum(struct instance_t* instance, uint8_t type)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t *cs = NULL;
    
    switch(type)
    {
        case 0:
            cs = &mbase_cb->Energy_chksum;
            break;

        case 1:
            cs = &mbase_cb->UseEnergy_chksum;
            break;

        default:
            return 0;
            break;
    }

    *cs = __cal_EC_checksum(instance, type);

    return 1;
}


//写双备份区
//type :0 电量   1 用电量
int16_t Save_EC2(struct instance_t* instance, uint8_t type)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t len_tmp, energy_len =  (offsetof(struct mbase_control_block_t, Energy_chksum) - offsetof(struct mbase_control_block_t, active_p[0]));
    uint16_t use_energy_len =  (offsetof(struct mbase_control_block_t, UseEnergy_chksum) - offsetof(struct mbase_control_block_t, MonthEnegery_dec));
    uint32_t s_index;
    uint8_t *p_tmp = (uint8_t*)&mbase_cb->active_p;
    uint16_t ID = Energy_block;
    int ret;

    switch(type)
    {
        case 0:
            ID = Energy_block;
            p_tmp = (uint8_t*)&mbase_cb->active_p;
            len_tmp = energy_len + sizeof(mbase_cb->Energy_chksum);
            break;

        case 1:
            ID = UseEnergy_block;
            p_tmp = (uint8_t*)&mbase_cb->MonthEnegery_dec;
            len_tmp = use_energy_len + sizeof(mbase_cb->UseEnergy_chksum);
            break;

        default:
            return -1;
            break;
    } 

    s_index = (uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_parameter_1 << 16) + ID;
    ret = nd_set_index(instance, s_index, p_tmp, len_tmp); 
    if (ret < 0) //无效 暂不处理
    {
        return -1;
    }

    return 1;

}


//电能的 校验恢复 主入口
//type :0 电量   1 用电量
//mode :0 上电校验恢复   1 运行期校验恢复
int16_t Check_EC_recover(struct instance_t* instance, uint8_t type, uint8_t mode)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t check_sum[2] = {0,};
    uint32_t addr = 0x00,s_index;
    uint16_t len_tmp, energy_len =  (offsetof(struct mbase_control_block_t, Energy_chksum) - offsetof(struct mbase_control_block_t, active_p[0]));
    uint16_t use_energy_len =  (offsetof(struct mbase_control_block_t, UseEnergy_chksum) - offsetof(struct mbase_control_block_t, MonthEnegery_dec));
    uint16_t block_len = (offsetof(struct mbase_control_block_t, persistent_energy_EC_label) - offsetof(struct mbase_control_block_t, active_p[0]));
    uint8_t *p_block = (uint8_t*)&mbase_cb->active_p;
    uint8_t *p_tmp = (uint8_t*)&mbase_cb->active_p;
    uint16_t ID = Energy_block;
    int ret = 0;

    
    //EC0 有效则退出
    ret = judge_EC_checksum(instance, type);
    if (ret > 0)
    {
        return 1;
    }


    switch(type)
    {
        case 0:
            ID = Energy_block;
            p_tmp = (uint8_t*)&mbase_cb->active_p;
            len_tmp = energy_len + sizeof(mbase_cb->Energy_chksum);
            break;

        case 1:
            ID = UseEnergy_block;
            p_tmp = (uint8_t*)&mbase_cb->MonthEnegery_dec;
            len_tmp = use_energy_len + sizeof(mbase_cb->Energy_chksum);
            break;

        default:
            return 0;
            break;
    }


    if (mode == 0) 
    {
        //读EC1
        if (mbase_cb->ec_save_success == 0x5AA5)
        {
            //EC block 从active_p开始
            p_block = (uint8_t*)&mbase_cb->active_p;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                    offsetof(struct mbase_control_block_t, active_p) -  offsetof(struct mbase_control_block_t, persistent_section_start));
            nd_get_offset(instance, s_index, p_block, block_len);

            // EC1 只使用一次   // 需清标志      
            mbase_cb->ec_save_success = 0;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                    offsetof(struct mbase_control_block_t, ec_save_success)  -  offsetof(struct mbase_control_block_t, persistent_section_start));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->ec_save_success, sizeof(mbase_cb->ec_save_success));

            ret = judge_EC_checksum(instance, type);

            if (ret > 0) //EC1 有效    完成恢复退出
            {
                return 1;
            }
        }
    
    }


    //读EC2 
    s_index = (uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_parameter_1 << 16) + ID;
    ret = nd_get_index(instance, s_index, p_tmp, len_tmp); 
    if (ret < 0) //无效 暂不处理
    {
        return -1;
    }

    ret = judge_EC_checksum(instance, type); 
    if (ret < 0) //无效 暂不处理
    {
        return -1;
    }


    return 1;
}



void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t SaveData_flag = 0; // BIT0:第1个电能校验   BIT1:第2个电能校验  。。。。
    uint8_t Updata_flag = 0;   // BIT0:第1个电能校验   BIT1:第2个电能校验  。。。。
    
    uint8_t fee_index = 0;
    uint32_t s_index;

    if (mbase_cb->meter_const == 0)
    {
        return;
    }

    Check_EC_recover(instance, 0, 1);
    Check_EC_recover(instance, 1, 1);

   
    if((mbase_cb->TOUNo == 0) || (mbase_cb->TOUNo> Max_tou_fee))
    {
        fee_index = Max_tou_fee-1;
    }
    else
    {
        fee_index = mbase_cb->TOUNo;
    }
    
    if(mbase_cb->p_pluse > 0)
    {
        Updata_flag |= BIT0;
        mbase_cb->active_p_dec[0] += mbase_cb->p_pluse;
        mbase_cb->active_p_dec[fee_index] += mbase_cb->p_pluse;
        if(mbase_cb->active_energy_cal_word & ADD_POS)
        {
            Updata_flag |= BIT1;
            mbase_cb->MonthEnegery_dec += mbase_cb->p_pluse;
        }
        mbase_cb->p_pluse = 0;
    }
    
    //费率正电能，月度费率电能
    if(mbase_cb->active_p_dec[fee_index] >= mbase_cb->meter_const)
    {
        Updata_flag |= BIT0;
        mbase_cb->active_p[fee_index] += 1;
        //mbase_cb->month_active_p[fee_index] += 1;
        mbase_cb->active_p_dec[fee_index] -= mbase_cb->meter_const; 
    }    
    //总正电能，总月度电能
    if(mbase_cb->active_p_dec[0] >= mbase_cb->meter_const)
    {
        Updata_flag |= BIT0;
        mbase_cb->active_p[0] += 1;
        //mbase_cb->month_active_p[0] += 1;
        mbase_cb->active_p_dec[0] -= mbase_cb->meter_const;
        SaveData_flag |= BIT0;
    }
    //-------
    
    if(mbase_cb->n_pluse > 0)
    {
        Updata_flag |= BIT0;
        mbase_cb->active_n_dec[0] += mbase_cb->n_pluse;
        mbase_cb->active_n_dec[fee_index] += mbase_cb->n_pluse;
        if(mbase_cb->active_energy_cal_word & ADD_NEG)
        {
            Updata_flag |= BIT1;
            mbase_cb->MonthEnegery_dec += mbase_cb->n_pluse;
        }
        mbase_cb->n_pluse = 0;
    }

    if(mbase_cb->active_n_dec[fee_index] >= mbase_cb->meter_const)
    {
        Updata_flag |= BIT0;
        mbase_cb->active_n[fee_index] += 1;
        //mbase_cb->month_active_n[fee_index] += 1;
        mbase_cb->active_n_dec[fee_index] -= mbase_cb->meter_const;
    }
    
    if(mbase_cb->active_n_dec[0] >= mbase_cb->meter_const)
    {
        
        Updata_flag |= BIT0;
        mbase_cb->active_n[0] += 1;
        //mbase_cb->month_active_n[0] += 1;
        mbase_cb->active_n_dec[0] -= mbase_cb->meter_const;
        SaveData_flag |= BIT0;
    }

    //--------
    if(mbase_cb->MonthEnegery_dec >= mbase_cb->meter_const/100 )
    {
        
        mbase_cb->MonthEnegery += 1;
        mbase_cb->MonthEnegery_dec = 0;

        Updata_flag |= BIT1;
        if((mbase_cb->MonthEnegery % 100)==0)
        {
            SaveData_flag |= BIT1;
        }
    }

    //------
    
    if (Updata_flag & BIT0)
    {
        updata_EC_checksum(instance, 0);
    }

    if (Updata_flag & BIT1)
    {
        updata_EC_checksum(instance, 1);
    }



    if (SaveData_flag & BIT0)
    {
        Save_EC2(instance, 0);
    }

    if (SaveData_flag & BIT1)
    {
        Save_EC2(instance, 1);
    }


}

void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint32_t s_index;

    ec_pluse_cnt = 0;

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    Check_EC_recover(instance, 0, 0);
    Check_EC_recover(instance, 1, 0);

    //完成上电      清标志
    if(mbase_cb->ec_save_success == 0x5AA5)
    {
        mbase_cb->ec_save_success = 0;
        s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                offsetof(struct mbase_control_block_t, ec_save_success)  -  offsetof(struct mbase_control_block_t, persistent_section_start));
        nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->ec_save_success, sizeof(mbase_cb->ec_save_success));
    }

}


void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->ec_save_success = 0;
    
}

int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* mbase_cb, uint8_t index , uint8_t type)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    
    uint8_t word = mbase_cb->active_energy_cal_word;
    
    if((mbase_cb->meter_const==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(word & ADD_POS)
    {
        energy_kwh += mbase_cb->active_p[index];
        energy_pulse += mbase_cb->active_p_dec[index];
    }
    if(word & DEC_POS)
    {
        energy_kwh -= mbase_cb->active_p[index];
        energy_pulse -= mbase_cb->active_p_dec[index];
    }
    if(word & ADD_NEG)
    {
        energy_kwh += mbase_cb->active_n[index];
        energy_pulse += mbase_cb->active_n_dec[index];
    }
    if(word & DEC_NEG)
    {
        energy_kwh -= mbase_cb->active_n[index];
        energy_pulse -= mbase_cb->active_n_dec[index];
    }
    
    
    if(type == 1)
    {
        energy_kwh = energy_pulse*10000 / mbase_cb->meter_const;//0.0001kwh
    }
    else
    {
        energy_kwh = energy_kwh*100;//0.01kwh
    }
    
    return energy_kwh;
}


int32_t Get_Active_Energy(struct mbase_control_block_t* mbase_cb, uint8_t index , uint8_t type , uint8_t pn)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    
    if((mbase_cb->meter_const==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(pn ==0)
    {
        energy_kwh += mbase_cb->active_p[index];
        energy_pulse += mbase_cb->active_p_dec[index];
    }
    else
    {
        energy_kwh += mbase_cb->active_n[index];
        energy_pulse += mbase_cb->active_n_dec[index];
    }

    if(type == 1)
    {
        energy_kwh = energy_pulse*10000 / mbase_cb->meter_const;//0.0001kwh
    }
    else
    {
        energy_kwh = energy_kwh*100;//0.01kwh
    }
    
    return energy_kwh;
}



void Energy_Pluse_Count(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    //DISABLE_ALL_INT();
    if((ec_pluse_cnt < 64) && (ec_pluse_cnt > 0))
    {
        if(mbase_cb->power_dir == 0)
        {
            mbase_cb->p_pluse += ec_pluse_cnt;
        }
        else
        {
            mbase_cb->n_pluse += ec_pluse_cnt;
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
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t block_len = (offsetof(struct mbase_control_block_t, persistent_energy_EC_label) - offsetof(struct mbase_control_block_t, active_p[0]));

    
    for(fee_index = 0; fee_index < (MAX_FEE_NO+1) ; fee_index++)
    {
        mbase_cb->active_n[fee_index] = 0x00;
        mbase_cb->active_n_dec[fee_index] = 0x00;
        
        mbase_cb->active_p[fee_index] = 0x00;
        mbase_cb->active_p_dec[fee_index] = 0x00;
          
    }

    mbase_cb->MonthEnegery = 0;
    mbase_cb->MonthEnegery_dec = 0;

    updata_EC_checksum(instance, 0);
    updata_EC_checksum(instance, 1);
    
    Save_EC2(instance, 0);
    Save_EC2(instance, 1);

    //清 EC1 
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_p) -  offsetof(struct mbase_control_block_t, persistent_section_start));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->active_p, block_len);
}

void Energy_Set(struct instance_t* instance, uint32_t basedata, uint8_t dir)
{
    uint8_t fee_index = 0;
    uint32_t s_index;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t block_len = (offsetof(struct mbase_control_block_t, persistent_energy_EC_label) - offsetof(struct mbase_control_block_t, active_p[0]));

    if(mbase_cb->factor_mode == false)  return;
      
    if(dir == 0) //正向
    {
        for(fee_index = 0; fee_index < (MAX_FEE_NO+1) ; fee_index++)
        {
            mbase_cb->active_p[fee_index] = 0x00;
            mbase_cb->active_p_dec[fee_index] = 0x00;
        }
        mbase_cb->active_p[0] = basedata;
        mbase_cb->active_p[1] = basedata;
    }
    else    //反向
    {
        for(fee_index = 0; fee_index < (MAX_FEE_NO+1) ; fee_index++)
        {
            mbase_cb->active_n[fee_index] = 0x00;
            mbase_cb->active_n_dec[fee_index] = 0x00;
        }
        mbase_cb->active_n[0] = basedata;
        mbase_cb->active_n[1] = basedata;
    }

    mbase_cb->MonthEnegery = 0;
    mbase_cb->MonthEnegery_dec = 0;

    updata_EC_checksum(instance, 0);
    updata_EC_checksum(instance, 1);
    
    Save_EC2(instance, 0);
    Save_EC2(instance, 1);

    //清 EC1 
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    offsetof(struct mbase_control_block_t, active_p) -  offsetof(struct mbase_control_block_t, persistent_section_start));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->active_p, block_len);
}

