#include "nidavellir.h"
#include "gs_record.h"
#include "645id_table.h"
#include "pin_function_def.h"
#include "fr_698.h"
#include "utils.h"
//参数  
//0x04001405  主动上报状态字自动复位延时时间     //默认30min
//0x04001104   8字节 主动上报模式字
//0x04001501   主动上报状态字

#define REPORT_TYPE_2015   (0x80)

struct double_protocol_report_map_t
{
    uint32_t OAD_State698;      // 698事件OAD 或 698状态
    uint8_t state_index645;      // 645状态位  
    uint8_t mode_index645;       // 645模式位  // BIT7 1: 表示 为状态 2015（非事件）
};

int report_645stat_save(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;
    
    ret = nd_set_index_e2p(instance, (uint8_t *) &mbase_cb->report_stat_645[0], mbase_block_parameter, report_stat_645_E, 108); 

    return ret;
}

int report_645stat_recover(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;

    memset((uint8_t *) &mbase_cb->report_stat_645[0], 0, sizeof(mbase_cb->report_stat_645));
    ret = nd_get_index_e2p(instance, (uint8_t *) &mbase_cb->report_stat_645[0], mbase_block_parameter, report_stat_645_E, 108); 
    if(ret < 0)
    {
        memset((uint8_t *) &mbase_cb->report_stat_645[0], 0, sizeof(mbase_cb->report_stat_645));
    }

    return ret;
}
int report_645stat_clear(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;

    memset((uint8_t *) &mbase_cb->report_stat_645[0], 0, sizeof(mbase_cb->report_stat_645));
    report_645stat_save(instance);

    return ret;
}


void event_out_check(struct instance_t* instance)
{    
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t tmp[12] = {0};

    EventOut_698_check(instance); //以698上报为准

#if 0        
    if ((mbase_cb->metertype & F_Plc_En))  //模块
    {
        if (0 == mem_compare(tmp, mbase_cb->report_stat_645, 12))
        {
            PLC_EVENOUT_DIS(); //关闭
        } 
        else
        {
            PLC_EVENOUT_EN();   //打开 置高  
        }
    }
#endif    
}

static const struct double_protocol_report_map_t  double_protocol_report_map_table[] =
{
    {0x302B0200, 0, 0},// 电能表负荷开关误动作事件  
    {RPT_2015_ESAM_ERR,         1,      1 | REPORT_TYPE_2015},     // esam错误  

    {RPT_2015_RTC_BATTERY_LOW,  3,      3 | REPORT_TYPE_2015},     // 时钟电池   

    {RPT_2015_MEMORIZER_ERR,    5,      5 | REPORT_TYPE_2015},     // 存储器故障或损坏   

    {RPT_2015_READ_BATTERY_LOW, 8,      8 | REPORT_TYPE_2015},     // 停抄电池   

    {0x302E0200, 7, 7},                 // 时钟故障   

    {RPT_2015_OVERDRAFT,        9,      9 | REPORT_TYPE_2015},     // 透支状态 

    {0x301B0200, 10, 10},               // 电能表开盖事件  
    {0x301C0200, 11, 11},               // 电能表开端钮盒事件  
    {0x302A0200, 12, 12},               // 电能表恒定磁场干扰事件  
    {0x302C0200, 13, 13},               // 电能表电源异常事件  
    {0x301F0200, 14, 14},               // 电能表跳闸事件   // 同为2015 的状态?  
    {0x30200200, 15, 15},               // 电能表合闸事件   // 同为2015 的状态?

#if 0  //三相
    {0x30000700, 16, 16},               // A相电能表失压事件  
    {0x30010700, 17, 17},               // A相电能表欠压事件  
    {0x30020700, 18, 18},               // A相电能表过压事件  
    {0x30040700, 19, 19},               // A相电能表失流事件  
#endif
    {0x30050700, 20, 20},               // A相电能表过流事件  
#if 0  //三相    
    {0x30080700, 21, 21},               // A相电能表过载事件  
    {0x30070700, 22, 22},               // A相电能表功率反向事件  
    {0x30030700, 23, 23},               // A相电能表断相事件  
    {0x30060700, 24, 24},               // A相电能表断流事件  
#endif

#if 0  //三相
    //{, 30, 30},                 // 电能表外置表跳闸成功2015bit31   //山东
    //{, 31, 31},                  // 电能表外置表合闸成功2015bit31   //山东

    {0x30000800, 32, 16},    // B相电能表失压事件  
    {0x30010800, 33, 17},    // B相电能表欠压事件  
    {0x30020800, 34, 18},    // B相电能表过压事件  
    {0x30040800, 35, 19},    // B相电能表失流事件  
    {0x30050800, 36, 20},    // B相电能表过流事件  
    {0x30080800, 37, 21},    // B相电能表过载事件  
    {0x30070800, 38, 22},    // B相电能表功率反向事件  
    {0x30030800, 39, 23},    // B相电能表断相事件  
    {0x30060800, 40, 24},    // B相电能表断流事件  

    {0x30000900, 48, 16},    // C相电能表失压事件  
    {0x30010900, 49, 17},    // C相电能表欠压事件  
    {0x30020900, 50, 18},    // C相电能表过压事件  
    {0x30040900, 51, 19},    // C相电能表失流事件  
    {0x30050900, 52, 20},    // C相电能表过流事件  
    {0x30080900, 53, 21},    // C相电能表过载事件  
    {0x30070900, 54, 22},    // C相电能表功率反向事件  
    {0x30030900, 55, 23},    // C相电能表断相事件  
    {0x30060900, 56, 24},    // C相电能表断流事件  

    {0x300F0200, 64, 32},    // 电能表电压逆相序事件  
    {0x30100200, 65, 33},    // 电能表电流逆相序事件  
    {0x301D0200, 66, 34},    // 电能表电压不平衡事件  
    {0x301E0200, 67, 35},    // 电能表电流不平衡事件  
    {0x300E0200, 68, 36},    // 电能表辅助电源掉电事件  
#endif

    {0x30110200, 69, 37},   // 电能表掉电事件  

#if 0 //三相
    {0x30090200, 70, 38},// 正向有功需量超限事件 
    {0x300A0200, 70, 38},// 反向有功需量超限事件 
    {0x300B0600, 70, 38},// 1象限无功需量超限事件 
    {0x300B0700, 70, 38},// 2象限无功需量超限事件 
    {0x300B0800, 70, 38},// 3象限无功需量超限事件 
    {0x300B0900, 70, 38},// 4象限无功需量超限事件 

    {0x300C0200, 71, 39},       // 电能表功率因数超下限事件  

    {0x302D0200, 72, 40},       // 电能表电流严重不平衡事件  
    //{, 73, 41},               // 电能表潮流反向事件  
    {0x300D0200, 74, 42},       // 电能表全失压事件  
#endif

    {0xFF310000, 80, 48},               // 电能表编程事件  //645
    {0x30130200, 81, 49},               // 电能表清零事件  
    {0x30140200, 82, 50},               // 电能表需量清零事件  
    {0x30150200, 83, 51},               // 电能表事件清零事件  
    {0x30160200, 84, 52},               // 电能表校时事件  
    {0x30170200, 85, 53},               // 电能表时段表编程事件  
    {0x30180200, 86, 54},               // 电能表时区表编程事件  
    {0x30190200, 87, 55},               // 电能表周休日编程事件  

    {0x30210200, 88, 56},               // 电能表节假日编程事件  
    {0x30220200, 89, 57},               // 电能表有功组合方式编程事件  
    {0x30230200, 90, 58},               // 电能表无功组合方式1编程事件  
    //{0x30230200, 91, 59},             // 电能表无功组合方式2编程事件  
    {0x301A0200, 92, 60},               // 电能表结算日编程事件  
    {0x30240200, 93, 61},               // 电能表费率参数表编程事件  
    {0x30250200, 94, 62},               // 电能表阶梯表编程事件  
    {0x30260200, 95, 63},               // 电能表密钥更新事件  
};


static const uint8_t  double_protocol_report_map_num =sizeof(double_protocol_report_map_table)/sizeof(double_protocol_report_map_table[0]);



// 645设置上报模式字,更新698的事件上报标识和跟随上报模式字
// 入参:mode 645上报模式字数据，  数据长度8byte
void report_mode_set_from_645(struct instance_t* instance, uint8_t* report_mode)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t cnt, mod_cnt645 = 0,  value = 0;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF,event_oad = 0xFFFFFFFF;
    uint32_t s_index;
    
    
    for(cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        
    
        mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
        mod_cnt645 &=~ REPORT_TYPE_2015;

        if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
        {
            mod_cnt_2015 = double_protocol_report_map_table[cnt].OAD_State698;
            event_oad = 0xFFFFFFFF;
        }
        else
        {            
            mod_cnt_2015 = 0xFFFFFFFF;
            event_oad = double_protocol_report_map_table[cnt].OAD_State698;            
        }

        
        if (report_mode[mod_cnt645/8] & (0x01 << (mod_cnt645 % 8)))
        {
            value = 1;
        }
        else
        {
            value = 0;
        }


        if (0xFFFFFFFF != mod_cnt_2015) //状态
        {
            if (1 == value)
            {
                mbase_cb->report_2015mode |= mod_cnt_2015;
            }
            else
            {
                mbase_cb->report_2015mode &= ~mod_cnt_2015;
            } 
            
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
            offsetof(struct mbase_control_block_t, report_2015mode));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->report_2015mode, 4);
        }

        if (0xFFFFFFFF != event_oad) //事件
        {
            if((0x30110200 == event_oad) && (1 == value))//如果设置的是掉电事件上报，应将掉电事件上报设置为结束上报 
            {
                value = 2;
            }
               
            set_event_report_flag_mode(instance, event_oad, value, 0); //上报标识

            //if(value)  value = 1;
            //set_event_report_flag_mode(instance, event_oad, value, 1); //上报模式
        }
    }

    event_out_check(instance);  
}


//698设置跟随上报模式字,更新645的上报模式字
void report_2015mode_set_from_698(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, mod_cnt645 = 0, value = 0;

    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;
    uint32_t s_index;
    uint8_t tmpbuf[2] = {0};


    for(cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        
        mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
        mod_cnt645 &=~ REPORT_TYPE_2015;

        if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
        {
            mod_cnt_2015 = double_protocol_report_map_table[cnt].OAD_State698;
            event_oad = 0xFFFFFFFF;
        }
        else
        {            
            mod_cnt_2015 = 0xFFFFFFFF;
            event_oad = double_protocol_report_map_table[cnt].OAD_State698;            
        }

        if (0xFFFFFFFF != mod_cnt_2015 )
        {

            if (mbase_cb->report_2015mode & mod_cnt_2015)
            {
                value = 1;
            }
            else
            {
                value = 0;
            }

            if (mod_cnt645 < 64)
            {
                if (1 == value)
                {
                    mbase_cb->report_mode_645[mod_cnt645/8] |= (0x01 << (mod_cnt645 % 8));
                }
                else
                {
                    mbase_cb->report_mode_645[mod_cnt645/8] &= ~(0x01 << (mod_cnt645 % 8));
                } 
            }
        }
    }

    //保存 645上报模式字
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, report_mode_645));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->report_mode_645, sizeof(mbase_cb->report_mode_645));

    EventOut_698_check(instance);  
}

// 698设置事件上报标识,同时修改645上报模式字数据
// 入参:mode 698设置事件oad和上报标识，
void report_3320flag_set_from_698(struct instance_t* instance, uint32_t oad, uint8_t value)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, mod_cnt645 = 0, rptmode = 0;
    uint32_t s_index;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;

    
    if(value == 2)//结束时刻的上报645不应支持
    {
        return;
    }

    for(cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {

        mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
        mod_cnt645 &=~ REPORT_TYPE_2015;

        if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
        {
            mod_cnt_2015 = double_protocol_report_map_table[cnt].OAD_State698;
            event_oad = 0xFFFFFFFF;
        }
        else
        {            
            mod_cnt_2015 = 0xFFFFFFFF;
            event_oad = double_protocol_report_map_table[cnt].OAD_State698;            
        }
        mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;

        if ((event_oad != 0xFFFFFFFF) && (event_oad == oad))
        {
            if (mod_cnt645 < 64)
            {

        //         rptmode = read_event_report_flag_mode(instance, oad, 1);
        //         if(rptmode != 0xFF)
        //         {
        //            if(rptmode)//需要上报方式设置为跟随上报
        //            {
                         if(value == 0)
                         {
                            mbase_cb->report_mode_645[mod_cnt645/8] &= ~(0x01 << (mod_cnt645 % 8));
                         }
                         else
                         {
                            mbase_cb->report_mode_645[mod_cnt645/8] |= (0x01 << (mod_cnt645 % 8));
                         }
        //           }
        //           else//如果被配置为主动上报，则645也不在跟随上报
        //           {
        //               mbase_cb->report_mode_645[mod_cnt645/8] &= ~(0x01 << (mod_cnt645 % 8));
        //           }

            }
        }
    }

    
    //保存 645上报模式字
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, report_mode_645));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->report_mode_645, sizeof(mbase_cb->report_mode_645));

    EventOut_698_check(instance);  

}


//  698设置事件上报方式，如果上报标识已经打开,则同时修改645上报模式字数据 
// 入参:mode 698设置事件oad和上报标识，
void report_3320mode_set_from_698(struct instance_t* instance, uint32_t oad, uint8_t value)
{

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, mod_cnt645 = 0, rptmode = 0;
    uint32_t s_index;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;

    for(cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
        mod_cnt645 &=~ REPORT_TYPE_2015;

        if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
        {
            mod_cnt_2015 = double_protocol_report_map_table[cnt].OAD_State698;
            event_oad = 0xFFFFFFFF;
        }
        else
        {            
            mod_cnt_2015 = 0xFFFFFFFF;
            event_oad = double_protocol_report_map_table[cnt].OAD_State698;            
        }


        if ((event_oad != 0xFFFFFFFF) && (event_oad == oad))
        {
            if (mod_cnt645 < 64)
            {                
                if(value == 0)//如果被设置为主动上报，就直接将645的上报模式字中的位清零，因为645所说的主动上报，其实就是2020中的跟随上报
                {
                    mbase_cb->report_mode_645[mod_cnt645/8] &= ~(0x01 << (mod_cnt645 % 8));
                }
            }
        }
    }
    
    //保存 645上报模式字
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, report_mode_645));
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->report_mode_645, sizeof(mbase_cb->report_mode_645));

    EventOut_698_check(instance);  

}



//OAD_State698  698事件oad或者698状态 2015的BITMAP
void report_645state_judge(struct instance_t* instance, uint32_t OAD_State698)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t cnt;
    uint8_t tmp,  rptmode = 0, rptflag = 0;
    uint8_t mod_cnt645, stat_cnt645;



    //针对于掉电事件在不足1h时，掉电事件配置为主动上报，则645不置位掉电事件
    if((OAD_State698 == 0x30110200))
    {
        rptflag = read_event_report_flag_mode(instance, OAD_State698, 0);
        rptmode = read_event_report_flag_mode(instance, OAD_State698, 1);
        if((rptmode == 0x00) && (rptflag != 0x00) 
        && (mbase_cb->power_on_sec_accum < mbase_cb->active_report_sec_limit))
        {
            return;
        }
    }
    
    for (cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        if (OAD_State698 == double_protocol_report_map_table[cnt].OAD_State698)
        {
            mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
            mod_cnt645 &=~ REPORT_TYPE_2015;
            stat_cnt645 = double_protocol_report_map_table[cnt].state_index645;

            if (mbase_cb->report_mode_645[mod_cnt645 / 8] & (0x01 << (mod_cnt645 % 8)))
            {
                switch(OAD_State698)
                {
                    
                    case RPT_2015_ESAM_ERR:
                    case RPT_2015_RTC_BATTERY_LOW:
                    case RPT_2015_READ_BATTERY_LOW:
                    case RPT_2015_MEMORIZER_ERR:
                    case RPT_2015_OVERDRAFT:
                        mbase_cb->report_stat_645[12 + stat_cnt645] = 0xFF;
                        mbase_cb->report_stat_645[stat_cnt645/8] |= (0x01 << (stat_cnt645 % 8));
                        break;
                    case 0x30130200: //电表清零
                        memset(&mbase_cb->report_stat_645[0], 0x00, 108);    
                        
                        if ((false == mbase_cb->key_state))// 私钥状态上报清零事件  
                        {  
                            mbase_cb->report_stat_645[12 + stat_cnt645] = 0x01;
                            mbase_cb->report_stat_645[stat_cnt645/8] |= (0x01 << (stat_cnt645 % 8));
                        }                        
                        break;
                    case 0x30150200://事件清零
                        if (0x00 == mbase_cb->flag_event_clear) // 分项清零不报
                        {
                            break;
                        }
                        
                        mbase_cb->flag_event_clear = 0x00;//645清零后应将该变量清零。
                        
                        tmp = mbase_cb->report_stat_645[12 + stat_cnt645];
                        
                        if (mbase_cb->report_stat_645[10] & 0x02)// 清零事件 
                        {
                            memset(mbase_cb->report_stat_645, 0x00, 108);
                            mbase_cb->report_stat_645[10] |= 0x02;
                            mbase_cb->report_stat_645[12 + 81] = 0x01;
                        }
                        else
                        {
                            memset(mbase_cb->report_stat_645, 0x00, 108);
                        }
                        
                        mbase_cb->report_stat_645[12 + stat_cnt645] = tmp;
                        if (tmp != 0xff)
                        {
                            mbase_cb->report_stat_645[12 + stat_cnt645] ++; 
                        }
                        mbase_cb->report_stat_645[stat_cnt645/8] |= (0x01 << (stat_cnt645 % 8));
                        break;
                    default:
                        if (mbase_cb->report_stat_645[12 + stat_cnt645] != 0xFF)
                        {
                            mbase_cb->report_stat_645[12 + stat_cnt645] ++;
                        }
                        mbase_cb->report_stat_645[stat_cnt645/8] |= (0x01 << (stat_cnt645 % 8));
                        break;                   
                }  
                
                report_645stat_save(instance);
                event_out_check(instance);
                
                return;   
            }  

            if (0x30130200 == OAD_State698)
            {
                memset(mbase_cb->report_stat_645, 0x00, 108);
                report_645stat_save(instance);
            }
            else if (0x30150200 == OAD_State698)
            {  
                if (0x00 == mbase_cb->flag_event_clear)
                {
                    return;
                }

                mbase_cb->flag_event_clear = 0x00;//645清零后应将该变量清零。    
                if (mbase_cb->report_stat_645[10] & 0x02)// 清零事件 
                {
                    memset(mbase_cb->report_stat_645, 0x00, 108);
                    mbase_cb->report_stat_645[10] |= 0x02;
                    mbase_cb->report_stat_645[12 + 81] = 0x01;
                }
                else
                {
                    memset(mbase_cb->report_stat_645, 0x00, 108);
                }
                report_645stat_save(instance);
            }

            event_out_check(instance);
        }
    }
}




//698读取事件oad,清3320中的上报oad时,同时清645上报状态字
uint8_t report_stat_reset_from_3320_698(struct instance_t* instance, uint32_t OAD_State698)
{
    uint8_t value = false;

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, mod_cnt645 = 0xFF, state_cnt645 = 0xFF,index = 0;
    uint32_t s_index;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;

    for (cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        if (OAD_State698 == double_protocol_report_map_table[cnt].OAD_State698)
        {
            mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
            mod_cnt645 &=~ REPORT_TYPE_2015;
            state_cnt645 = double_protocol_report_map_table[cnt].state_index645;
            break;
        }
    }


    //   确认3320事件时，不清2015  
    if((state_cnt645 == 14) || (state_cnt645 == 15))
    {
        return value;
    }
    

    if (state_cnt645 != 0xFF)
    {
        index = state_cnt645%8;
        mbase_cb->report_stat_645[12 + state_cnt645] = 0; 
        mbase_cb->report_stat_645[state_cnt645/8] &= ~(0x01<<index);
        mbase_cb->report_stat_read_cache645[state_cnt645/8] &= ~(0x01<<index);
#if 0
        //  外置表拉合闸成功清除  
        if((bitNo == 14) || (bitNo == 15))
        {
            bitNo += 16;
            index = bitNo%8;
            mbase_cb->report_stat_645[12 + bitNo] = 0; 
            mbase_cb->report_stat_645[bitNo/8] &= ~(0x01<<index);
            mbase_cb->report_stat_read_cache645[bitNo/8] &= ~(0x01<<index);
        }
#endif
        report_645stat_save(instance);   

        mbase_cb->report_stat_already_read645 = 0; 

        event_out_check(instance);
        value = true;
    }

    return value;
    

}




 
//645复位上报状态字同时清该通道698上报标志
//通过bit位号找上报名称oad  ,清当前通道698新增上报列表中的该oad  ,或通过bitNo查找698中2015上报状态对应位
 
uint8_t report_645stat_reset_from_645(struct instance_t* instance, uint8_t state_cnt645)
{
    
    
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i, cnt, mod_cnt645 = 0xFF,index = 0;
    uint32_t s_index;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;
    uint8_t comm_chn[4] = { chl_ir_698, chl_RS485_1_698,  chl_PLC_698,  chl_RS485_2_698 };
    uint16_t channel_id = 0xFFFF;
    uint8_t value = false;


    for (cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        if (state_cnt645 == double_protocol_report_map_table[cnt].state_index645)
        {
            mod_cnt645 = (double_protocol_report_map_table[cnt].mode_index645);
            mod_cnt645 &=~ REPORT_TYPE_2015;

            if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
            {
                mod_cnt_2015 = double_protocol_report_map_table[cnt].OAD_State698;
                event_oad = 0xFFFFFFFF;
            }
            else
            {            
                mod_cnt_2015 = 0xFFFFFFFF;
                event_oad = double_protocol_report_map_table[cnt].OAD_State698;            
            }
            break;
        }
    }

    if(event_oad == 0xFF310000) //645编程事件
    {
        return false;
    }
    
    for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        //channel_id = dlt645_get_current_channel(); //  只清645当前通道的698上报 
        channel_id = comm_chn[i];                 //  清除所有通道的698上报   山东要求 
        
        if (channel_id != 0xFFFF)
        {
            if (event_oad != 0xFFFFFFFF) // 3320 
            {
                 value = report_3320list_confirmed(instance, comm_chn[i], event_oad, 0); 
                 if (state_cnt645 == 14)// 拉闸 可能同时存在事件和状态 
                 {
                    mbase_cb->report_2015state[i] &=~ RPT_2015_SWITCH_OFF_SUCCEES;
                    value = true;
                 }
                 else if (state_cnt645 == 15)// 合闸 可能同时存在事件和状态 
                 {
                    mbase_cb->report_2015state[i] &=~ RPT_2015_SWITCH_ON_SUCCEES;
                    value = true;
                 }
            }
            if (mod_cnt_2015 != 0xFFFFFFFF)              // 2015 
            {
                 mbase_cb->report_2015state[i] &=~ mod_cnt_2015;
                 value = true;
            }
        }
    }
    return value;

}

// 698复位上报状态字(32bit 4字节  2015) 同时清645上报标志  //OAD_State698 状态位
uint8_t report_stat_reset_from_698(struct instance_t* instance, uint32_t State2015_698) 
{


    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, mod_cnt645 = 0xFF, stat_cnt645 = 0xFF;
    uint32_t s_index;
    uint32_t mod_cnt_2015 = 0xFFFFFFFF, event_oad = 0xFFFFFFFF;

    for (cnt = 0; cnt < double_protocol_report_map_num; cnt++)
    {
        if(double_protocol_report_map_table[cnt].mode_index645 & REPORT_TYPE_2015)
        {
            if (State2015_698 & double_protocol_report_map_table[cnt].OAD_State698)
            {
                mod_cnt645 = double_protocol_report_map_table[cnt].mode_index645;
                mod_cnt645 &=~ REPORT_TYPE_2015;
                stat_cnt645 = double_protocol_report_map_table[cnt].state_index645;

                mbase_cb->report_stat_645[12 + stat_cnt645] = 0; 
                mbase_cb->report_stat_645[stat_cnt645/8] &= ~(0x01 << (stat_cnt645 % 8));
                mbase_cb->report_stat_read_cache645[stat_cnt645/8] &= ~(0x01 << (stat_cnt645 % 8));

            }
        }

    }

    report_645stat_save(instance);        

    mbase_cb->report_stat_already_read645 = 0; 

    event_out_check(instance);
    
    return true;

}




//秒调用
void report_645state_reset_sec_judge(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t cnt, index;
        
    if (0 != mbase_cb->report_reset_sec_cnt_645)
    {
        mbase_cb->report_reset_sec_cnt_645--;

        if (0 == mbase_cb->report_reset_sec_cnt_645)
        {                        
            for (cnt = 0; cnt < 12; cnt++)
            {
                for (index = 0; index < 8; index++)
                {
                    if (mbase_cb->report_stat_read_cache645[cnt] & (0x01 << index))
                    {   
                        mbase_cb->report_stat_645[cnt] &= ~(0x01 << index);
                        mbase_cb->report_stat_645[12 + 8*cnt + index] = 0;
                        report_645stat_reset_from_645(instance,   8*cnt + index);
                    }
                }
            }

            mbase_cb->report_stat_already_read645 = 0;
            memset(mbase_cb->report_stat_read_cache645, 0x00, 12);
            
            report_645stat_save(instance);   
            
            event_out_check(instance);         
        }            
    }
}

void report_state_read_pro(struct instance_t* instance, uint8_t *buff, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t delay = 1;
    uint8_t cnt, index, num = 0;
    int ret = 0;
    
    mbase_cb->report_stat_already_read645 = 1;
    ndv_memcopy(mbase_cb->report_stat_read_cache645, mbase_cb->report_stat_645, 12);
    ndv_memcopy(buff, mbase_cb->report_stat_645, 12);
    buff[12] = 0xAA;

    for (cnt = 0; cnt < 12; cnt++)
    {
        for (index = 0; index < 8; index++)
        {
            if (mbase_cb->report_stat_645[cnt] & (0x01 << index))
            {
                buff[13 + num] = mbase_cb->report_stat_645[12 + 8*cnt + index];
                num ++;
            }
        }
    }
 
    buff[13 + num] = 0xAA;
    *length = 14 + num;

    ret = nd_get_index_e2p(instance, (uint8_t *)&delay, mbase_block_parameter, ReportRstTime, 1); 
    if (ret < 0)
    {
        delay = 0x30;
    }
    delay = bcd_to_hex(delay);

    mbase_cb->report_reset_sec_cnt_645 = 60*delay;
        
    if (0 == mbase_cb->report_reset_sec_cnt_645)
    {
        mbase_cb->report_reset_sec_cnt_645 = 1;
    }
}

void dlt645_clear_follow_para(struct instance_t* instance, struct context_645_t* ctx)
{
    ctx->segment_tx_no = 0;
    ctx->segment_tx = 0;
    ctx->report_stat_follow_flag = 0;
}

/**************************************************
*判断无后续帧时是否上报主动上报状态字
*
***************************************************/
int report_push_data_judge(struct instance_t* instance, uint8_t* databuf,  struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t charact = 0, length = 0;
    uint8_t tmp[12] = {0};
    int ret = 0;

    if ((0 == (ctx->ctrl & FOLLOW_FRAM_FLAG)))
    {   
        if ((1 == ctx->report_stat_follow_flag) || ((0x91 != ctx->ctrl) && (0x92 != ctx->ctrl)))
        {
            dlt645_clear_follow_para(instance, ctx);
            return -1;          
        }

        
        /*读后续帧上报标志*/ /*读698是否开启跟随上报*/   // 山东698跟随上报关联645后续帧  需要特殊处理 
        //ret = nd_get_index_e2p(instance, (uint8_t *)&charact, mbase_block_parameter, FollowReport_en, 1); 
        ret = nd_get_index_e2p(instance, (uint8_t *)&charact, mbase_block_parameter, MeterRunWord, 1); 
        if(ret < 0)
        {
            charact = 0;
        }
        
        //if ((0 == charact))     
        if (0 == (charact & 0x04))       
        {
            dlt645_clear_follow_para(instance, ctx);
           return -1;
        }  

        if (0 == mem_compare(tmp, mbase_cb->report_stat_645, 12))
        {
            dlt645_clear_follow_para(instance, ctx);
            return -1;
        }  

        report_state_read_pro(instance, databuf, &length);
        //report_state_read_pro(pdlt645_para->push_para.remain_data, (uint8*)&pdlt645_para->push_para.remain_len);

#if 0
        if (0 != pdlt645_para->push_para.remain_len)
        {
            pdlt645_para->push_para.report_flag = 1;
            pdlt645_para->push_para.push_flag = 1;

            hfp_memcopy((uint8*)&pdlt645_para->push_para.dl645_info_push.dlt645_id, rxbuf, 4);
            pdlt645_para->control_code |= 0x20;
        }
        else
        {
            dlt645_clear_follow_para();
        }
#endif

        return length;
    }
    return -1;
}


