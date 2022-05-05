#include "nidavellir.h"
#include "fr_698.h"

typedef struct 
{
    uint16_t    oad;
    int8_t     convert;//换算
    uint8_t     uint;//单位
}ATBE3;

const ATBE3 Atbe3_table[] = 
{
    0x2000,     -1,     unit_V,     //电压
    0x2001,     -3,     unit_A,     //电流
    0x2004,     -1,     unit_W,    //有功功率
    0x2006,     -1,     unit_VA,
    0x2007,     -1,     unit_W,
    0x200A,     -3,     unit_undefined,
    0x200F,     -2,     unit_Hz,
    0x2010,     -1,     unit_tempture,
    0x2011,     -2,     unit_V,
    0x2013,     0,      unit_minute,
    0x2014,     0,      unit_undefined,
    0x2015,     0,      0,
    0x2031,     -2,     unit_kWh,
    0x2040,     0,      unit_undefined,
    0x2041,     0,      unit_undefined,
};

int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t i,j,maxnum=0;
    uint16_t oad;
    uint8_t index,attribute, temp[10] = {0};
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    typedef struct 
    {
        unsigned long  id;
        unsigned char  num;//个数
        unsigned char  tag;
        unsigned char* addr;
        unsigned char  len;
    }idmatch;
    
    idmatch idbuf[] = 
    {
        0x20000200,     0x01,   TYPE_long_unsigned,         (uint8_t*)&mbase_cb->voltage_ac,        2,  //电压
        0x20000201,     0x00,   TYPE_long_unsigned,         (uint8_t*)&mbase_cb->voltage_ac,        2,  //电压
        0x20010200,     0x01,   TYPE_double_long,           (uint8_t*)&mbase_cb->current_l,         4,  //火线电流
        0x20010201,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->current_l,         4,  //火线电流
        0x20010400,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->current_n,         4,  //零线电流
        0x20040200,     0x02,   TYPE_double_long,           (uint8_t*)&mbase_cb->active_power,      4,  //有功功率
        0x20040201,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->active_power,      4,  //有功功率
        0x20040202,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->active_power,      4,  //有功功率
        0x20060200,     0x02,   TYPE_double_long,           (uint8_t*)&mbase_cb->apparent_power,    4,  //视在功率
        0x20060201,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->apparent_power,    4,  //视在功率
        0x20070200,     0x02,   TYPE_double_long,           (uint8_t*)&mbase_cb->active_avg_power,  4,  //1分钟平均功率
        0x20070201,     0x00,   TYPE_double_long,           (uint8_t*)&mbase_cb->active_avg_power,  4,  //1分钟平均功率
        0x200A0200,     0x02,   TYPE_long,                  (uint8_t*)&mbase_cb->power_factor,      2,  //功率因数
        0x200A0201,     0x00,   TYPE_long,                  (uint8_t*)&mbase_cb->power_factor,      2,  //功率因数
        0x200A0202,     0x00,   TYPE_long,                  (uint8_t*)&mbase_cb->power_factor,      2,  //功率因数
        0x200F0200,     0x00,   TYPE_long_unsigned,         (uint8_t*)&mbase_cb->frequency,         2,  //频率
        0x20100200,     0x00,   TYPE_long,                  (uint8_t*)&mbase_cb->temperature,       2,  //温度
        0x20110200,     0x00,   TYPE_long_unsigned,         (uint8_t*)&mbase_cb->voltage_battery,   2,  //电池电压
        0x20130200,     0x00,   TYPE_double_long_unsigned,  (uint8_t*)&mbase_cb->battery_used_time, 4,  //电池工作时间     
        0x20310200,     0x00,   TYPE_double_long_unsigned,  (uint8_t*)&mbase_cb->MonthEnegery,      4,  //月度用电量
        0x20150400,     0x00,   TYPE_bit_string,            (uint8_t*)&mbase_cb->report_state,      4,  //跟随上报状态字
        0x20400200,     0x00,   TYPE_bit_string,            (uint8_t*)&mbase_cb->cmd_dostate,       2,  //控制命令执行状态字
        0x20410200,     0x00,   TYPE_bit_string,            (uint8_t*)&mbase_cb->cmd_errstate,      2,  //控制命令错误状态字
    };
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    if(attribute == 0x03)//属性3，换算及单位
    {
        maxnum = sizeof(Atbe3_table)/sizeof(ATBE3);
        for(i=0; i<maxnum; i++)
        {
            if(oad == Atbe3_table[i].oad)
            {
                *p++ = TYPE_Scaler_Unit;
                *p++ = Atbe3_table[i].convert;
                *p++ = Atbe3_table[i].uint;
                return (p-buf);
            }
        }
        return -1;
    }
    
    maxnum = sizeof(idbuf)/sizeof(idmatch);
    for(i=0; i<maxnum; i++)
    {
        if((id & 0xFFFF1FFF) == idbuf[i].id)
        {
            switch(mode)
            {
                case 0x00:
                    if(idbuf[i].num != 0)   //数组
                    {
                        *p++ = TYPE_array;//数组
                        *p++ = idbuf[i].num;//单相一个元素
                    }
                    else
                    {
                        idbuf[i].num = 1;
                    }
                      
                    for(j=0; j<idbuf[i].num; j++)
                    {
                        *p++ = idbuf[i].tag;
                        if(idbuf[i].tag == TYPE_bit_string)
                        {
                            *p++ = idbuf[i].len*8;
                        }
                        mem_swap_cpy(p, (unsigned char*)idbuf[i].addr, idbuf[i].len);
                        p = p + idbuf[i].len;
                    }
                    return (p-buf);
                    break;
                case 0x01:  //T L形式
                    if(idbuf[i].num != 0)   //数组
                    {
                        *p++ = TYPE_array;//数组
                        *p++ = idbuf[i].num;//单相一个元素
                    }
                    for(j=0; j<idbuf[i].num; j++)
                    {
                        *p++ = idbuf[i].tag;
                        if(idbuf[i].tag != TYPE_bit_string)
                        {
                            *p++ = idbuf[i].len;
                        }
                    }
                    return (p-buf);
                default:
                    return -DAR_OBJECT_NOT_EXIST;
            }
        }
    }
    
    switch (oad)
    {
        case 0x2014://电能表运行状态字
            if(index > 7)  return -1;
            if(attribute == 0x02)
            {
                if(index == 0)
                {
                    *p++ = TYPE_array;
                    *p++ = 7;
                    for(i=1; i<8; i++)
                    {
                        *p++ = TYPE_bit_string;
                        *p++ = 16;
                        memcpy(p, (uint8_t*)&mbase_cb->run_state[i], 0x02);
                        p = p + 2;
                    }
                    return (p-buf);
                }
                else
                {
                    *p++ = TYPE_bit_string;
                    *p++ = 16;
                    mem_swap_cpy(p, (uint8_t*)&mbase_cb->run_state[index], 0x02);
                    p = p + 2;
                    return (p-buf);
                }
            }
            else if(attribute == 0x03)
            {
            
            }
            else
                return -1;
  
        default:
            break;
    }
    
     switch (id & 0xFFFF1FFF)
    {
        case 0x20220200://事件记录序号
        case 0x20230200://冻结记录序号
            *p++ = TYPE_double_long_unsigned;
            memset(p, 0xFF, 4);
            p = p + 4;
            return (p-buf);
            break;
        case 0x201E0200://事件发生时间
        case 0x20200200://事件结束时间  
        case 0x20210200://数据冻结时间 
            *p++ = TYPE_date_time_s;
            rtc_get_from_tm(instance,temp);
            *p++ = temp[6];
            *p++ = temp[5];
            *p++ = temp[4];
            *p++ = temp[3];
            *p++ = temp[2];
            *p++ = temp[1];
            *p++ = temp[0];
            return (p-buf);
            break;
        case 0x20240200:// 时间发生源  
        case 0x33000200:// 事件上报状态   
            *p++ = TYPE_null;
            return (p-buf);
            break;    
        case 0x20000000:  //测试代码
            break;
        default:
            break;
    }
   
    
    return -DAR_OBJECT_NOT_EXIST;;
}
