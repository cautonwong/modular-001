
#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"


#define Max_tou_fee 12

#define  ADD_POS    0x80
#define  DEC_POS    0x40
#define  ADD_NEG    0x20
#define  DEC_NEG    0x10
#include "targets/stategrid_meter/app/metering_energy_adapter.h"
#include "domain/metering/legacy/metering_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/legacy_energy_layout.h"

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
            buff =  (uint8_t *)&nd_mbase_active_p(mbase_cb)[0];
            len = (uint16_t)legacy_energy_checksum_span();       
            break;

        case 1:
            buff =  (uint8_t *)&nd_mbase_MonthEnegery_dec(mbase_cb);
            len = (uint16_t)legacy_energy_use_checksum_span();
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
            cs = nd_mbase_Energy_chksum(mbase_cb);
            break;

        case 1:
            cs = nd_mbase_UseEnergy_chksum(mbase_cb);
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
            cs = &nd_mbase_Energy_chksum(mbase_cb);
            break;

        case 1:
            cs = &nd_mbase_UseEnergy_chksum(mbase_cb);
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
    uint16_t len_tmp, energy_len =  (legacy_energy_checksum_span());
    uint16_t use_energy_len =  (legacy_energy_use_checksum_span());
    uint32_t s_index;
    uint8_t *p_tmp = (uint8_t*)&nd_mbase_active_p(mbase_cb);
    uint16_t ID = Energy_block;
    int ret;

    switch(type)
    {
        case 0:
            ID = Energy_block;
            p_tmp = (uint8_t*)&nd_mbase_active_p(mbase_cb);
            len_tmp = energy_len + legacy_energy_checksum_size();
            break;

        case 1:
            ID = UseEnergy_block;
            p_tmp = (uint8_t*)&nd_mbase_MonthEnegery_dec(mbase_cb);
            len_tmp = use_energy_len + legacy_energy_use_checksum_size();
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
    uint16_t i, len_tmp, energy_len =  (legacy_energy_checksum_span());
    uint16_t use_energy_len =  (legacy_energy_use_checksum_span());
    uint16_t block_len = (legacy_energy_block_span());
    uint8_t *p_block = (uint8_t*)&nd_mbase_active_p(mbase_cb);
    uint8_t *p_tmp = (uint8_t*)&nd_mbase_active_p(mbase_cb);
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
            p_tmp = (uint8_t*)&nd_mbase_active_p(mbase_cb);
            len_tmp = energy_len + legacy_energy_checksum_size();
            break;

        case 1:
            ID = UseEnergy_block;
            p_tmp = (uint8_t*)&nd_mbase_MonthEnegery_dec(mbase_cb);
            len_tmp = use_energy_len + legacy_energy_use_checksum_size();
            break;

        default:
            return 0;
            break;
    }


    if (mode == 0) 
    {
        //读EC1
        if (nd_mbase_ec_save_success(mbase_cb) == 0x5AA5)
        {
            //EC block 从active_p开始
            p_block = (uint8_t*)&nd_mbase_active_p(mbase_cb);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                    legacy_energy_persistent_offset());
            nd_get_offset(instance, s_index, p_block, block_len);

            // EC1 只使用一次   // 需清标志      
            nd_mbase_ec_save_success(mbase_cb) = 0;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                    legacy_energy_ec_offset());
            nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_ec_save_success(mbase_cb), legacy_energy_ec_size());

            ret = judge_EC_checksum(instance, type);

            if (ret > 0) //EC1 有效    完成恢复退出
            {
                for(i = 0; i < 2; i++) //保存所有至 EC2
                {
                    updata_EC_checksum(instance, i); 
                    Save_EC2(instance, i);
                }
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



static void merge_update(nd_energy_update_t *total, nd_energy_update_t part)
{
    total->energy_changed |= part.energy_changed;
    total->month_changed |= part.month_changed;
    total->energy_save_due |= part.energy_save_due;
    total->month_save_due |= part.month_save_due;
}

void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    nd_energy_update_t update = {0};
    nd_energy_update_t part;

    if ((nd_mbase_meter_const(mbase_cb) / 100) == 0)
    {
        return;
    }
    if ((nd_mbase_EC_SaveDiff(mbase_cb) < 30) || (nd_mbase_EC_SaveDiff(mbase_cb) > 1000))
    {
        nd_mbase_EC_SaveDiff(mbase_cb) = 200;
    }

    Check_EC_recover(instance, 0, 1);
    Check_EC_recover(instance, 1, 1);

    part = metering_energy_add(metering_energy_ledger_from_instance(instance), ND_ENERGY_FORWARD, nd_mbase_p_pluse(mbase_cb));
    merge_update(&update, part);
    nd_mbase_p_pluse(mbase_cb) = 0;

    part = metering_energy_add(metering_energy_ledger_from_instance(instance), ND_ENERGY_REVERSE, nd_mbase_n_pluse(mbase_cb));
    merge_update(&update, part);
    nd_mbase_n_pluse(mbase_cb) = 0;

    if (update.energy_changed)
    {
        updata_EC_checksum(instance, 0);
    }
    if (update.month_changed)
    {
        updata_EC_checksum(instance, 1);
    }
    if (update.energy_save_due)
    {
        Save_EC2(instance, 0);
    }
    if (update.month_save_due)
    {
        Save_EC2(instance, 1);
    }
}

void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint32_t s_index;
    int ret = 0;
    
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    ec_pluse_cnt = 0;
    Check_EC_recover(instance, 0, 0);
    Check_EC_recover(instance, 1, 0);

    //完成上电      清标志
    if(nd_mbase_ec_save_success(mbase_cb) == 0x5AA5)
    {
        nd_mbase_ec_save_success(mbase_cb) = 0;
        s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                legacy_energy_ec_offset());
        nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_ec_save_success(mbase_cb), legacy_energy_ec_size());
    }

    //电量保存阈值 读取
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + EC_SaveDiff_E);
    ret = nd_get_index(instance, s_index, (uint8_t*)&nd_mbase_EC_SaveDiff(mbase_cb), 2);
    if(ret < 0)
    {
        nd_mbase_EC_SaveDiff(mbase_cb) = 200; //默认2度一存。
    }

}


//停电处理 刷新脉冲尾数
void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    nd_energy_update_t update = {0};
    nd_energy_update_t part;
    uint8_t i = 0;
    int ret = 0;

    nd_mbase_ec_save_success(mbase_cb) = 0;
    for(i = 0; i < 2; i++)
    {
        ret = judge_EC_checksum(instance, i);
        if (ret < 0)
        {
            return;
        }
    }

    Energy_Pluse_Count(instance, pipeline);

    part = metering_energy_add(metering_energy_ledger_from_instance(instance), ND_ENERGY_FORWARD, nd_mbase_p_pluse(mbase_cb));
    merge_update(&update, part);
    nd_mbase_p_pluse(mbase_cb) = 0;

    part = metering_energy_add(metering_energy_ledger_from_instance(instance), ND_ENERGY_REVERSE, nd_mbase_n_pluse(mbase_cb));
    merge_update(&update, part);
    nd_mbase_n_pluse(mbase_cb) = 0;

    if (update.energy_changed)
    {
        updata_EC_checksum(instance, 0);
    }
    if (update.month_changed)
    {
        updata_EC_checksum(instance, 1);
    }
}

int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* mbase_cb, uint8_t index , uint8_t type)
{
    int32_t energy_kwh = 0;
    int32_t energy_pulse = 0;
    uint8_t word = nd_mbase_active_energy_cal_word(mbase_cb);
    
    if((nd_mbase_meter_const(mbase_cb)==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(word & ADD_POS)
    {
        energy_kwh += nd_mbase_active_p(mbase_cb)[index];
        energy_pulse += nd_mbase_active_p_dec(mbase_cb)[index];
    }
    if(word & DEC_POS)
    {
        energy_kwh -= nd_mbase_active_p(mbase_cb)[index];
        energy_pulse -= nd_mbase_active_p_dec(mbase_cb)[index];
    }
    if(word & ADD_NEG)
    {
        energy_kwh += nd_mbase_active_n(mbase_cb)[index];
        energy_pulse += nd_mbase_active_n_dec(mbase_cb)[index];
    }
    if(word & DEC_NEG)
    {
        energy_kwh -= nd_mbase_active_n(mbase_cb)[index];
        energy_pulse -= nd_mbase_active_n_dec(mbase_cb)[index];
    }
    
    if(type == 1)
    {
        if(energy_pulse < 0)    
        {   
            energy_pulse = ~energy_pulse+1;
            energy_kwh = energy_pulse*10000/nd_mbase_meter_const(mbase_cb);
            energy_kwh = -energy_kwh;
        }
        else
        {
            energy_kwh = energy_pulse*10000/nd_mbase_meter_const(mbase_cb);//0.0001kwh
        }
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
    
    if((nd_mbase_meter_const(mbase_cb)==0) || (index>Max_tou_fee))
    {
        return 0;
    }
    if(pn ==0)
    {
        energy_kwh += nd_mbase_active_p(mbase_cb)[index];
        energy_pulse += nd_mbase_active_p_dec(mbase_cb)[index];
    }
    else
    {
        energy_kwh += nd_mbase_active_n(mbase_cb)[index];
        energy_pulse += nd_mbase_active_n_dec(mbase_cb)[index];
    }

    if(type == 1)
    {
        energy_kwh = energy_pulse*10000 / nd_mbase_meter_const(mbase_cb);//0.0001kwh
    }
    else
    {
        energy_kwh = energy_kwh*100;//0.01kwh
    }
    
    return energy_kwh;
}



//有功 电能获取
//phase: 0/1/2/3 : t/a/b/c
//FeeNo: 0:总,  1-12: F1-F12
//type: 0:4字节hex(x.xx), 1:8字节hex(x.xxxx)
//pn:   0:正,   1:反
//有功 电能获取
//phase: 0/1/2/3 : t/a/b/c
//FeeNo: 0:总,  1-12: F1-F12
//type: 0:4字节hex(x.xx), 1:8字节hex(x.xxxx)
//pn:   0:正,   1:反
int64_t Get_Active_Energy_TABC(struct mbase_control_block_t* mbase_cb, uint8_t phase, uint8_t FeeNo , uint8_t type , uint8_t pn)
{
    if((nd_mbase_meter_const(mbase_cb) == 0) || (FeeNo > Max_tou_fee) || (phase != 0))
    {
        return 0;
    }

    return metering_energy_read(metering_energy_ledger_from_mbase(mbase_cb),
                                pn ? ND_ENERGY_REVERSE : ND_ENERGY_FORWARD,
                                FeeNo,
                                type ? ND_ENERGY_RES_0_0001_KWH : ND_ENERGY_RES_0_01_KWH);
}
//组合有功 电能获取
//FeeNo: 0:总,  1-12: F1-F12
//type: 0:4字节hex(x.xx), 1:8字节hex(x.xxxx)
int64_t Get_Cal_Active_Energy_T(struct mbase_control_block_t* mbase_cb, uint8_t FeeNo , uint8_t type)
{
    if((nd_mbase_meter_const(mbase_cb) == 0) || (FeeNo > Max_tou_fee))
    {
        return 0;
    }

    return metering_energy_read_combined(
        metering_energy_ledger_from_mbase(mbase_cb), FeeNo,
        type ? ND_ENERGY_RES_0_0001_KWH : ND_ENERGY_RES_0_01_KWH);
}
void Energy_Pluse_Count(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    //DISABLE_ALL_INT();
    if((ec_pluse_cnt < 64) && (ec_pluse_cnt > 0))
    {
        if(nd_mbase_power_dir(mbase_cb) == 0)
        {
            nd_mbase_p_pluse(mbase_cb) += ec_pluse_cnt;
        }
        else
        {
            nd_mbase_n_pluse(mbase_cb) += ec_pluse_cnt;
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
    uint32_t s_index;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t block_len = (legacy_energy_block_span());

    metering_energy_clear(metering_energy_ledger_from_instance(instance));

    updata_EC_checksum(instance, 0);
    updata_EC_checksum(instance, 1);

    Save_EC2(instance, 0);
    Save_EC2(instance, 1);
    //清 EC1
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    legacy_energy_persistent_offset());
    nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_active_p(mbase_cb), block_len);
}

void Energy_Set(struct instance_t* instance, uint32_t basedata, uint8_t dir)
{
    uint32_t s_index;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t block_len = (legacy_energy_block_span());

    if(nd_mbase_factor_mode(mbase_cb) == false)  return;

    metering_energy_set_total(metering_energy_ledger_from_instance(instance),
                              dir == 0 ? ND_ENERGY_FORWARD : ND_ENERGY_REVERSE,
                              basedata);

    updata_EC_checksum(instance, 0);
    updata_EC_checksum(instance, 1);

    Save_EC2(instance, 0);
    Save_EC2(instance, 1);
    //清 EC1
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    legacy_energy_persistent_offset());
    nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_active_p(mbase_cb), block_len);
}
