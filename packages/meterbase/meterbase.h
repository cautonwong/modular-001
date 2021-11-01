#ifndef __METER_BASE_H__
#define __METER_BASE_H__

#include "type_def.h"

struct fee_t
{
    unsigned char   DayTableNo;//日表序号
    unsigned char	TOUNo;//费率序号
    unsigned char   STEPNo;//阶梯序号
    unsigned long	TOUPrice;//费率电价
    unsigned long	STEPPrice;//阶梯电价
    unsigned char   SeasonTable;//时区表主副表序号
    unsigned char   DayTable;//日表主副表序号
    unsigned char   TouPriceTable;//费率电价表主副表序号
    unsigned char   StepTable;//阶梯表主副表序号
    unsigned char	StepSeason;//阶梯时区表主副表序号
    unsigned long	Price;//当前电价
    unsigned char	StepNum;//阶梯数
};

struct energy_t
{
    uint32 active_p;
    uint32 active_n;
    uint32 month_active_p;
    uint32 month_active_n;
    uint16 active_p_dec;
    uint16 active_n_dec;
};


struct emu_pluse_t
{
    uint32 last_active_p;
    uint32 last_active_n;
    uint32 current_active_p;
    uint32 current_active_n;
};



struct mbase_control_block_t
{
    int id;
    //电量数据
    uint32 meter_const;
    struct energy_t energy[13];
    struct emu_pluse_t emu_pluse;
    //瞬时量数据
    //需量数据
    //事件负荷冻结数据
    //费率数据
    struct fee_t Fee;
    //费用数据
};

#define   Max_tou_fee         14
#define   MaxTouNum           14


#endif