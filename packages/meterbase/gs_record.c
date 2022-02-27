#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "fr_698.h"
#include "resource_tool.h"


#define U32toBufLSB(x)  ((uint8_t)(x)), ((uint8_t)(x>>8)), ((uint8_t)(x>>16)), ((uint8_t)(x>>24))
#define U32toBufMSB(x)  ((uint8_t)(x>>24)),((uint8_t)(x>>16)), ((uint8_t)(x>>8)), ((uint8_t)(x))
// 初始化默认数据
// 过流事件关联对象
const uint8_t init_30050200[] = { 0x01,0x08, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x20002200), U32toBufMSB(0x20012200), U32toBufMSB(0x20042200), U32toBufMSB(0x200A2200), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201) };
const uint8_t init_302B0200[] = { 0x01,0x05, U32toBufMSB(0xf2052201), U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201) };
const uint8_t init_50020300[] = { 0x01,0x07, U32toBufMSB(0x00100401), U32toBufMSB(0x00200401), U32toBufMSB(0x20000200), U32toBufMSB(0x20010200), U32toBufMSB(0x20010400), U32toBufMSB(0x20040200), U32toBufMSB(0x200A0200) };


const uint8_t  init_MinuteFreeze_oad[]      = { 0x07, U32toBufMSB(0x00100401), U32toBufMSB(0x00200401), U32toBufMSB(0x20000200), U32toBufMSB(0x20010200), U32toBufMSB(0x20010400), U32toBufMSB(0x20040200), U32toBufMSB(0x200A0200) };
const uint8_t  init_HourFreeze_oad[]        = { 0x02, U32toBufMSB(0x00100401), U32toBufMSB(0x00200401) };
const uint8_t  init_DayFreeze_oad[]         = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_BillFreeze_oad[]        = { 0x04, U32toBufMSB(0x00000400), U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20310200) };
const uint8_t  init_MonthFreeze_oad[]       = { 0x03, U32toBufMSB(0x00000400), U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), };
const uint8_t  init_SeasonTabCutFreeze_oad[]= { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_DayTabCutFreeze_oad[]   = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_FeePriceCutFreeze_oad[] = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_StepCutFreeze_oad[]     = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_StepBillFreeze_oad[]    = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
const uint8_t  init_InstantFreeze_oad[]     = { 0x03, U32toBufMSB(0x00100400), U32toBufMSB(0x00200400), U32toBufMSB(0x20040200) };
           
          
const uint8_t  init_MinuteFreeze_period[]   = { 0x07, 0x00, 0x0F,     0x00, 0x00, 0x00,   0x00, 0x00, 0x00,   0x00, 0x00, 0x00,   0x00, 0x00, 0x00,   0x00, 0x00, 0x00,   0x00, 0x00, 0x00,   0x00, 0x00, 0x00, };   
const uint8_t  init_HourFreeze_period[]     = { 0x02, 0x00, 0x01 };
const uint8_t  init_DayFreeze_period[]      = { 0x03, 0x00, 0x01 };
const uint8_t  init_BillFreeze_period[]     = { 0x04, 0x00, 0x01 };
const uint8_t  init_MonthFreeze_period[]    = { 0x03, 0x00, 0x01 };
const uint8_t  init_SeasonTabCutFreeze_period[] = { 0x03, 0x00, 0x01 };
const uint8_t  init_DayTabCutFreeze_period[] = { 0x03, 0x00, 0x01 };
const uint8_t  init_FeePriceCutFreeze_period[] = { 0x03, 0x00, 0x01 };//待调整
const uint8_t  init_StepCutFreeze_period[]  = { 0x03, 0x00, 0x01 };//待调整
const uint8_t  init_StepBillFreeze_period[] = { 0x03, 0x00, 0x01 };//待调整
const uint8_t  init_InstantFreeze_period[]  = { 0x03, 0x00, 0x00 };                                           
                                                                                         
const uint8_t  init_MinuteFreeze_depth[] = { 0x88, 0xE0,    0x00, 0x00,     0x00, 0x00,     0x00, 0x00,     0x00, 0x00,     0x00, 0x00,     0x00, 0x00,     0x00, 0x00, };
const uint8_t  init_HourFreeze_depth[] = { 0x00,0xFE };
const uint8_t  init_DayFreeze_depth[] = { 0x01,0x6D };
const uint8_t  init_BillFreeze_depth[] = { 0x00,0x0C };
const uint8_t  init_MonthFreeze_depth[] = { 0x00,0x18};
const uint8_t  init_SeasonTabCutFreeze_depth[] = { 0x00,0x02 };
const uint8_t  init_DayTabCutFreeze_depth[] = { 0x00,0x02 };
const uint8_t  init_FeePriceCutFreeze_depth[] = { 0x00,0x02 };//待调整                                                       
const uint8_t  init_StepCutFreeze_depth[] = { 0x00,0x02 };//待调整                                                           
const uint8_t  init_StepBillFreeze_depth[] = { 0x00,0x02 };//待调整                                                         
const uint8_t  init_InstantFreeze_depth[] = { 0x00,0x03 };

const uint8_t  init_SecurityMode_En[] = { 0x01 };
const uint8_t  init_SecurityMode_EXData[] = { 0x00 };


const uint8_t  init_OverCurrentAS_oad[] = { 0x08, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x20002200), U32toBufMSB(0x20012200), U32toBufMSB(0x20042200), U32toBufMSB(0x200A2200), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201) };
const uint8_t  init_PowerOffS_oad[]     = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201)};
const uint8_t  init_MeterWrS_oad[]      = { 0x00 };
const uint8_t  init_MeterClrS_oad[]     = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201)};
const uint8_t  init_EventClrS_oad[]     = { 0x00 };
const uint8_t  init_SetTimeS_oad[]      = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_DayTabWrS_oad[]     = { 0x00 };
const uint8_t  init_SeasonTabWrS_oad[]  = { 0x01, U32toBufMSB(0x40152200)};
const uint8_t  init_WeekTabWrS_oad[]    = { 0x01, U32toBufMSB(0x40132200) };
const uint8_t  init_BillDayWrS_oad[]    = { 0x01, U32toBufMSB(0x41162200) };
const uint8_t  init_CoverOpenS_oad[]    = { 0x04, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201)};
const uint8_t  init_SwitchOffS_oad[]    = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_SwitchOnS_oad[]     = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_HolidayWrS_oad[]    = { 0x00 };
const uint8_t  init_ActCombWrS_oad[]    = { 0x01, U32toBufMSB(0x41122200) };
const uint8_t  init_FeeTabWrS_oad[]     = { 0x00 };  //待调整
const uint8_t  init_StepTabWrS_oad[]    = { 0x00 };  //待调整
const uint8_t  init_SecretKeyWrS_oad[]  = { 0x01, U32toBufMSB(0xF1002400) };
const uint8_t  init_ErrPlugCardS_oad[]  = { 0x00 };  //待调整
const uint8_t  init_RechargeS_oad[]     = { 0x00 };  //待调整
const uint8_t  init_RefundS_oad[]       = { 0x00 };  //待调整
const uint8_t  init_SwitchErrS_oad[]    = { 0x05, U32toBufMSB(0xF2052201), U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201) };
const uint8_t  init_PowerAbnormalS_oad[]= { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_RTCErrS_oad[]       = { 0x04, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201), U32toBufMSB(0x00108201), U32toBufMSB(0x00208201) };
const uint8_t  init_EmuErrS_oad[]       = { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_BroadcastTimeS_oad[]= { 0x02, U32toBufMSB(0x00102201), U32toBufMSB(0x00202201) };
const uint8_t  init_ZeroLineErrS_oad[]  = { 0x02, U32toBufMSB(0x20010201), U32toBufMSB(0x20010400) };





// 获取事件参数配置表
int get_event_para_pack(struct instance_t* instance, uint32_t oad, struct event_para_pack_t *para_pack)
{
    struct mbase_control_block_t* mbase_cb;

    uint16_t OI = ((uint16_t)((oad & 0xFFFF0000) >> 16));

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    switch (OI) //
    {
        case 0x3005:// 过流
        {
            struct para_698_tag_t OverCurrentAS_para[2] =
            {
                {TYPE_double_long,      (uint8_t*)&mbase_cb->OverCurrent_threshold,      4},
                {TYPE_unsigned,         (uint8_t*)&mbase_cb->OverCurrent_duration_para,  1},
            };
            
            memcpy((uint8_t*)para_pack->event_para, (uint8_t*)OverCurrentAS_para , sizeof(OverCurrentAS_para));
            para_pack->para_num = sizeof(OverCurrentAS_para)/ sizeof(OverCurrentAS_para[0]);
            return 1;
        }
        break;
        case 0x3040:// 零线电流异常
        {
            struct para_698_tag_t ZeroLineErrS_para[3] =
            {
                {TYPE_double_long,      (uint8_t*)&mbase_cb->ZeroLineErrS_Current_threshold,    4},
                {TYPE_long_unsigned,    (uint8_t*)&mbase_cb->ZeroLineErrS_rate_threshold,       2},
                {TYPE_unsigned,         (uint8_t*)&mbase_cb->ZeroLineErrS_duration_para,        1},
            };

            memcpy((uint8_t*)para_pack->event_para, (uint8_t*)ZeroLineErrS_para, sizeof(ZeroLineErrS_para));
            para_pack->para_num = sizeof(ZeroLineErrS_para) / sizeof(ZeroLineErrS_para[0]);
            return 1;
        }
        break;

    default:
        break;
    }
}




uint8_t get_activeflag(struct instance_t* instance, uint16_t RecordId)
{
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    if (ctrl_block->active_flag[RecordId / 8] & (1 << (RecordId % 8)))
    {
        return true;
    }
    return false;

}
void set_activeflag(struct instance_t* instance, uint16_t RecordId)
{
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    ctrl_block->active_flag[RecordId / 8] |= (1 << (RecordId % 8));
}

void clr_activeflag(struct instance_t* instance, uint16_t RecordId)
{
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    ctrl_block->active_flag[RecordId / 8] &= ~(1 << (RecordId % 8));
}

void set_recordflag(struct instance_t* instance, uint16_t RecordId)
{
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    ctrl_block->record_flag[RecordId / 8] |= (1 << (RecordId % 8));
}

void clr_recordflag(struct instance_t* instance, uint16_t RecordId)
{
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    ctrl_block->record_flag[RecordId / 8] &= ~(1 << (RecordId % 8));
}


int event_duration_pro(uint8_t duration_para, uint8_t* duration, uint8_t state, struct instance_t* instance, uint16_t RecordId)
{
    uint8_t activeflag = get_activeflag(instance, RecordId);

    if (state) // 发生
    {
        if (true == activeflag)
        {
            *duration = duration_para;
        }

        if (*duration < duration_para)
        {
            (*duration)++;
        }
        else if (false == activeflag)
        {
            set_activeflag(instance, RecordId);
            return 1; // 记录发生
        }
    }
    else // 结束
    {
        if (false == activeflag)
        {
            *duration = 0;
        }

        if (*duration > 0)
        {
            (*duration)--;
        }
        else if (true == activeflag)
        {
            clr_activeflag(instance, RecordId);
            return -1; // 记录结束
        }
    }

    return 0; // 不记录
}




//记录接口
//发送记录标识
void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload)
{
    struct message_t mesg;

    mesg.from = kernel;
    mesg.to = pid_resource_manager;
    mesg.ctrl = 0;
    mesg.qos = 0;
    mesg.len = 4;
    mesg.id = RecordId;
    mesg.oad_n = 1;

    set_recordflag(instance, mesg.id);

    send_mesg(instance, NULL, &mesg, payload);
}

//事件记录接口
// mode 1：记录发生   0 不记录。 -1 记录结束
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode)
{
    if (mode == 1) // 发生
    {
        send_recordflag_mesg(instance, task, RecordId, NULL);
    }
    else if (mode == -1) //结束
    {
        send_recordflag_mesg(instance, task, RecordId + 1, NULL);
    }
}

// 上电运行期间 每秒的调用
int record_judge(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	ia_over_event_judge(instance, pipeline, mesg);
    CoverOpen_judge(instance, pipeline, mesg);
}


//冻结或事件清零 接口    传入要清除的oad 
int clr_record(struct instance_t* instance, uint32_t oad)
{
    uint32_t temp = 0;
    struct record_storage_id_info_t info;
    int ret = get_record_id_info(instance, oad, &info);

    if (ret < 0)
    {
        return -1;
    }

    //清记录
    nd_log_clean(instance, info.save_index);
    //清序号
    nd_set_index(instance, info.sequence_index, (uint8_t*)&temp, 4);

    //累计时间清零
    if (info.acc_time)
    {
        *info.acc_time = 0;
    }
}

//多个记录 清零
static int list_record_clr(struct instance_t* instance, const uint32_t* oad_list, uint16_t num)
{
    uint16_t i;
    int ret;

    for (i = 0; i < num; i++)
    {
        ret = clr_record(instance, oad_list[i]);
        if (ret < 0)
        {
            return -1;
        }
    }
    return 1;
}


//现存冻结oad列表
const uint32_t freeze_oad[] =
{
    0x50020200,   //分钟冻结记录方案1 
    0x50022200,   //分钟冻结记录方案2       
    0x50024200,   //分钟冻结记录方案3
    0x50026200,   //分钟冻结记录方案4
    0x50028200,   //分钟冻结记录方案5
    0x5002A200,   //分钟冻结记录方案6
    0x5002C200,   //分钟冻结记录方案7
    0x5002E200,   //分钟冻结记录方案8
    0x50030200,   //小时冻结
    0x50040200,   //日冻结
    0x50050200,   //结算日冻结
    0x50060200,   //月冻结
    //0x50070200,   //年冻结 （不要求，但需占用位置）
    0x50080200,   //时区表切换冻结
    0x50090200,   //日时段表切换冻结
    //0x500A0200,   //费率电价切换冻结
    //0x500B0200,   //阶梯切换冻结
    //0x50110200,   //阶梯结算冻结
    0x50000200,   //瞬时冻结
};

//现存事件oad列表
const uint32_t event_oad[] =
{
    0x30050700,   //A过流 
    0x30110200,   //电能表掉电事件                      
    0x30120200,   //电能表编程事件            
    //0x30130200,   //电能表清零事件       // 单独处理      
    //0x30150200,   //电能表事件清零事件   // 单独处理      
    0x30160200,   //电能表校时事件            
    0x30170200,   //电能表时段表编程事件      
    0x30180200,   //电能表时区表编程事件      
    0x30190200,   //电能表周休日编程事件      
    0x301A0200,   //电能表结算日编程事件  
    0x301B0200,   //电能表开盖事件            
    0x301F0200,   //电能表跳闸事件            
    0x30200200,   //电能表合闸事件            
    0x30210200,   //电能表节假日编程事件      
    0x30220200,   //电能表有功组合方式编程事件
    //0x30240200,   //电能表费率参数表编程事件  
    //0x30250200,   //电能表阶梯表编程事件      
    0x30260200,   //电能表密钥更新事件 
    //0x30270200,   //电能表异常插卡事件        
    //0x30280200,   //电能表购电记录            
    //0x30290200,   //电能表退费记录  
    0x302B0200,   //电能表负荷开关误动作事件  
    0x302C0200,   //电能表电源异常事件 
    0x302E0200,   //电能表时钟故障事件 
    0x302F0200,   //电能表计量芯片故障事件
    0x303C0200,   //广播校时事件 
    0x30400200,   //零线电流异常事件  
};



//事件清零 // 不包含事件清零事件
int event_clr(struct instance_t* instance)
{
    int ret;

    ret = list_record_clr(instance, event_oad, (sizeof(event_oad) / sizeof(event_oad[0])));
    
    return ret;
}
//冻结清零
int freeze_clr(struct instance_t* instance)
{
    int ret;

    ret = list_record_clr(instance, freeze_oad, (sizeof(freeze_oad) / sizeof(freeze_oad[0])));
    return ret;
}

int meter_clr(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    //电量清零 待添加

    freeze_clr(instance); //冻结清零
    event_clr(instance);  // 事件清零
    clr_record(instance, 0x30150200); //事件清零事件 清零
    if ((mbase_cb->factor_mode == true)) //电表清零事件 清零
    {
        clr_record(instance, 0x30130200);
    }

    //其他清零  待添加

}

// 每秒调用
int event_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
      ia_over_event_judge(instance, pipeline, mesg);    // 电能表A相过流事件
      PowerOff_event_judge(instance, pipeline, mesg);   // 电能表掉电事件  
      SwitchErr_event_judge(instance, pipeline, mesg);  // 电能表负荷开关误动作事件
      ZeroLineErr_event_judge(instance, pipeline, mesg);// 零线电流异常事件 
      CoverOpen_event_judge(instance, pipeline, mesg);  // 电能表开盖事件  
      RTCErr_event_judge(instance, pipeline, mesg);     // 电能表时钟故障事件 
      EmuErr_event_judge(instance, pipeline, mesg);     // 电能表计量芯片故障事件

      return 1;
#if 0
           ///////OverCurrentAS = 11,             //  0x3005    A过流发生 


          //PowerOffS,  //  0x3011    电能表掉电事件                                               
                                              
          MeterWrS,  //  0x3012    电能表编程事件                                                
          /////MeterClrS,  //  0x3013    电能表清零事件                                                        
          /////EventClrS,  //  0x3015    电能表事件清零事件                                
          //SetTimeS,  //  0x3016    电能表校时事件                                    
          //DayTabWrS,  //  0x3017    电能表时段表编程事件                                        
          //SeasonTabWrS,  //  0x3018    电能表时区表编程事件                                
          WeekTabWrS,  //  0x3019    电能表周休日编程事件                                              
          BillDayWrS,  //  0x301A    电能表结算日编程事件  
          //////CoverOpenS,  //  0x301B    电能表开盖事件                                      

          //SwitchOffS,  //  0x301F    电能表跳闸事件                                                                           
          //SwitchOnS,  //  0x3020    电能表合闸事件                                          
          HolidayWrS,  //  0x3021    电能表节假日编程事件                                                
          ActCombWrS,  //  0x3022    电能表有功组合方式编程事件                                            
          FeeTabWrS,  //  0x3024    电能表费率参数表编程事件                                    
          StepTabWrS,  //  0x3025    电能表阶梯表编程事件                                           
          //////SecretKeyWrS,  //  0x3026    电能表密钥更新事件 
          //////ErrPlugCardS,  //  0x3027    电能表异常插卡事件                                           
          /////RechargeS,  //  0x3028    电能表购电记录                                       
          //RefundS,  //  0x3029    电能表退费记录  
          SwitchErrS,  //  0x302B    电能表负荷开关误动作事件                                               
                                      
          PowerAbnormalS,  //  0x302C    电能表电源异常事件 

          RTCErrS,  //  0x302E    电能表时钟故障事件 

          EmuErrS,  //  0x302F    电能表计量芯片故障事件

          BroadcastTimeS,  //  0x303C    广播校时事件 
          ZeroLineErrS,  //  0x3040    零线电流异常事件  
 
#endif




}

//停电下每秒调用  主要处理事件累计时间
int event_LowPower_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    PowerOff_LowPower_pro(instance, pipeline, mesg);
    PowerAbnormal_LowPower_pro(instance, pipeline, mesg);
}






enum scheme_mode
{
    FRZ_1 = 0,  //顺序固定
    FRZ_2,
    FRZ_3,
    FRZ_4,
    FRZ_5,
    FRZ_6,
    FRZ_7,
    FRZ_8,

    START_MODE = 10,
    END_MODE = 11,
};


//record_head 记录头 首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11位 结束
int read_record_head(struct instance_t* instance, uint8_t* databuf, const uint32_t* record_head, enum scheme_mode mode, uint16_t *remain_size,  struct record_storage_id_info_t *record_id_info)
{
    uint8_t *p , i, scheme_valid = true,  normal_type;
    int16_t single_len = 0, transform_len;
    uint32_t obj,seq;
    
    
    p = databuf;
    if (record_head)
    {
        for (i = 0; i < (record_head[0]); i++)
        {
            normal_type = 1;
            scheme_valid = true;
            obj = record_head[i+1];
            switch (obj)
            {
                case 0x20220200: //事件记录序号
#if 0               //序号统一最后补     
                    nd_get_index(instance, record_id_info->sequence_index, (uint8_t *)&seq, sizeof(seq));
                    *p++ = 0x06;
                    memcpy(p, &seq, 4);
                    p += 4;
                    single_len = 1 + 4;
                    //normal_type = 0;
#endif                    
                    break;
                case 0x201E0200: //事件发生时间
                    if (mode == END_MODE)
                    {
                        scheme_valid = false;
                    }
                    break;
                case 0x20200200: //事件结束时间
                    if (mode == START_MODE)
                    {
                        scheme_valid = false;
                    }
                    break;
                default:
                    break;
            }

            if (normal_type)
            {
                single_len = 0;
                
                if (true == scheme_valid)
                {
                    single_len = nd_get(instance, obj, p, *remain_size, NULL);
                    if (single_len > 0)
                    {
                        p += single_len;
                    }
                    else
                    {
                        single_len = 1;
                        memset(p, 0, single_len); //找不到填 NULL 
                        p += single_len;
                    }
                }
                else
                {
                    single_len = nd_size(instance, obj);
                    if (single_len > 0)
                    {
                        memset(p, 0xFF, single_len);
                        p += single_len;
                    }
                    else
                    {
                        single_len = 1;
                        memset(p, 0, single_len); //找不到填 NULL 
                        p += single_len;
                        
                    }
                }
                *remain_size -= single_len;
            }
        }
    }
    return (p - databuf);
}
//记录时捕获对象转换
uint32_t record_capture_change(uint32_t obj, uint16_t record_id)
{
    switch (record_id)
    {
    case SetTimeS:    
        if (obj == 0x201E0200)
        {
            obj = 0x40002200;
        }
        else if (obj == 0x20200200)
        {
            obj = 0x40008200;
        }
        break;
    case MeterWrS: //若第一个编程为校时,则编程开始时间取校时前时间
        if (obj == 0x201E0200)
        {
            obj = 0x40002200;
        }
        break;
    default:
        break;
    }

    return obj;
}
//方案号 & 属性特征 判断
//mode 0-7 为冻结方案， 10为 事件发生 11为 结束
uint8_t check_scheme(enum scheme_mode mode, uint32_t obj, uint16_t record_id)
{
    uint8_t scheme, scheme_valid;
    scheme = (uint8_t)(0x07 & (uint8_t)(obj >> 13));
    scheme_valid = true;
    //记录头
    switch (obj)
    {
    case 0x20210200: //冻结时间
    case 0x20230200: //冻结记录
        break;
        
    case 0x33020206: //事件编程    //特殊处理 结束时更新
        break;

    case 0x33150206: //编程节假日对象  事件记录用
    case 0x33150207: //编程前节假日内容 事件记录用
    case 0x33140206: //编程时段表对象  事件记录用
    case 0x33140207: //编程前时段表 事件记录用
    case 0x330C0206: //事件清零
    case 0x20220200: //事件记录序号
        if (mode == END_MODE)
        {
            scheme_valid = false;
        }
        break;
    case 0x33000200: //通道上报状态
    case 0x20240200: //事件发生源
    case 0x201E0200: //事件发生时间
        if (mode == END_MODE)
        {
            scheme_valid = false;
        }
        break;
    case 0x20200200: //事件结束时间
        if (mode == START_MODE)
        {
            scheme_valid = false;
        }
        break;
        
    default:
        //配置关联对象
        switch (mode)
        {
        case START_MODE:    //发生
            if ((scheme == 0) || (scheme == 3) || (scheme == 4))
            {
                scheme_valid = false;
            }
            break;
        case END_MODE:      //结束
            if ((scheme == 0) || (scheme == 1) || (scheme == 2))
            {
                scheme_valid = false;
            }
            break;
        default:
            if (mode != scheme)
            {
                scheme_valid = false;
            }
            break;
        }
        break;
    }

    if(record_id == SetTimeS) //待调整
    {
        scheme_valid = true;
    }
    
    return scheme_valid;

}
//capturelist  关联对象列表  首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11为 结束
int read_record_capture(struct instance_t* instance, uint8_t* databuf, uint32_t* CaptureList, enum scheme_mode mode, uint16_t* remain_size,  struct record_storage_id_info_t *record_id_info)
{
    uint8_t* p, i, scheme, scheme_valid = true;
    int16_t single_len = 0;
    uint32_t obj;
    uint8_t tmpbuf[200];//单个关联对象 最大长度
    uint16_t RecordID =  (uint16_t)record_id_info->save_index;
    struct circular_info_t circular_info = {0};
    uint8_t scheme_invalid_wr_flag = true;
    int ret;

    p = databuf;
    if (CaptureList)
    {
        //read last1
        if(mode == END_MODE)  // 结束需要先获取上一次存储数据
        {
            ret = nd_log_output(instance, record_id_info->save_index,  p, 1, &circular_info);
            if(ret < 0)
            {
                return -1;
            }
            
            if(circular_info.valid_num == 0)
            {
                return -1;
            }
            
            circular_info.curr_num = circular_info.valid_num - 1;//上1次

            ret = nd_log_output(instance, record_id_info->save_index,  p, circular_info.unit_size, &circular_info);
            if(ret < 0)
            {
                return -1;
            }
        }
        
        for (i = 0; i < (CaptureList[0]); i++)
        {
            obj = CaptureList[1 + i];
            scheme = (obj >> 13) & 0x07; 

            obj = record_capture_change(obj,RecordID);
           
            scheme_valid = true;
            scheme_valid = check_scheme(mode,obj, RecordID); //方案号 & 属性特征 判断
            
            scheme_invalid_wr_flag = true; //结束记录时 不更新不相关数据。
            if (( mode == END_MODE ) && ( scheme_valid == false ))
            {
                scheme_invalid_wr_flag = false;
            }
            
            single_len = 0;
 
            if (true == scheme_valid)
            {
                single_len = nd_get(instance, obj, tmpbuf, *remain_size, NULL);
                if ((single_len > 0) && (single_len < (sizeof(tmpbuf))))
                {
                    // 特殊事件关联对象的特殊处理 
                    // format改变，长度改变
                    ret = record_change_format(p, tmpbuf, obj, RFC_WRITE);
                    if (ret >= 0)
                    {
                        single_len = ret;
                    }
                    else
                    {
                        memcpy(p, tmpbuf, single_len);
                    }
                }
                if (single_len > 0)
                {
                    p += single_len;
                    *remain_size -= single_len;
                }
                else
                {
                    memset(p, 0, 1); //找不到填 NULL 
                    p += 1;
                    *remain_size -= 1;
                }
            }
            else
            {
                single_len = nd_size(instance, obj);
                if (single_len > 0)
                {
                    ret = record_change_format(tmpbuf, p, obj, RFC_GET_STORE_LEN);
                    if (ret >= 0)
                    {
                        single_len = ret;
                    }
                }

                if (single_len > 0)
                {
                    if (scheme_invalid_wr_flag == true)
                    {
                        memset(p, 0xFF, single_len);
                    }
                    p += single_len;
                    *remain_size -= single_len;
                }
                else
                {
                    if (scheme_invalid_wr_flag == true)
                    {
                        memset(p, 0, 1); //找不到填 NULL 
                    }
                    p += 1;
                    *remain_size -= 1;
                }
            }
        }
    }
    return (p - databuf);
}

int sync_data_record(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    enum scheme_mode mode = START_MODE;
    uint8_t data_buff[400] = { 0 }, payload[100];
    uint8_t *pdata = data_buff;
    uint16_t remain_size;
    uint32_t sequence = 0,Oad;
    uint32_t CaptureList[50 + 1] = { 0 };
    struct record_storage_id_info_t record_id_info; 
    uint8_t pairs_event = 0;
    uint8_t freez_scheme = 0;
    int ret;
      
    remain_size = sizeof(data_buff);
    
    //mesg 的id 就是事件代码
    int r = recv_mesg_header(instance, pipeline, mesg);
    
    if (r > 0)
    {
        Oad = 0xFFFFFFFF;
        fifoBuf_getData(pipeline->in_queue, payload, mesg->len);
        switch (mesg->id)
        {
            case OverCurrentAE:         //过流事件结束
            case OverCurrentAS:         //过流事件发生
                Oad = 0x30050700;
                mode = START_MODE + (mesg->id - OverCurrentAS);
                pairs_event = 1;
                break;
            case PowerOffE:             //电能表掉电事件结束
            case PowerOffS:             //电能表掉电事件发生
                Oad = 0x30110200;
                mode = START_MODE + (mesg->id - PowerOffS);
                pairs_event = 1;
                break;
            case MeterWrS:             //电能表编程事件  
            case MeterWrR:
                Oad = 0x30120200;
                mode = START_MODE + (mesg->id - MeterWrS);
                break;
            case MeterClrS:      
                Oad = 0x30130200;
                mode = START_MODE;
                break;
            case EventClrS:
                Oad = 0x30150200;
                mode = START_MODE;
                break;
            case SetTimeS:
                Oad = 0x30160200;
                mode = START_MODE;
                break;
            
            case DayTabWrS:
            case SeasonTabWrS:
            case WeekTabWrS:
            case BillDayWrS:
                Oad = 0x30170200 + ((mesg->id - DayTabWrS) << 16);
                mode = START_MODE ;
                break;

            case CoverOpenE:            //开盖事件结束
            case CoverOpenS:            //开盖事件发生
                Oad = 0x301B0200;
                mode = START_MODE + (mesg->id - CoverOpenS);
                pairs_event = 1;
                break;

            case SwitchOffS:
            case SwitchOnS:
            case HolidayWrS:
            case ActCombWrS:
                Oad = 0x301F0200 + ((mesg->id - SwitchOffS) << 16);
                mode = START_MODE;
                break;

            case FeeTabWrS:
            case StepTabWrS:
            case SecretKeyWrS:
            case ErrPlugCardS:
            case RechargeS:
            case RefundS:
                Oad = 0x30240200 + ((mesg->id - FeeTabWrS) << 16);
                mode = START_MODE;
                break;

            case SwitchErrE:            
            case SwitchErrS:            
                Oad = 0x302B0200;
                mode = START_MODE + (mesg->id - SwitchErrS);
                pairs_event = 1;
                break;
            case PowerAbnormalE:
            case PowerAbnormalS:
                Oad = 0x302C0200;
                mode = START_MODE + (mesg->id - PowerAbnormalS);
                pairs_event = 1;
                break;
            case RTCErrE:
            case RTCErrS:
                Oad = 0x302E0200;
                mode = START_MODE + (mesg->id - RTCErrS);
                pairs_event = 1;
                break;
            case EmuErrE:
            case EmuErrS:
                Oad = 0x302F0200;
                mode = START_MODE + (mesg->id - EmuErrS);
                pairs_event = 1;
                break;

            case BroadcastTimeS:
                Oad = 0x303C0200;
                mode = START_MODE;
                break;
            case ZeroLineErrE:
            case ZeroLineErrS:
                Oad = 0x30400200;
                mode = START_MODE + (mesg->id - ZeroLineErrS);
                pairs_event = 1;
                break;

            case InstantFreeze:         //瞬时冻结
                Oad = 0x50030200;
                mode = FRZ_1;
                break;

            case MinuteFreeze1: //分钟冻结记录方案1
            case MinuteFreeze2: //分钟冻结记录方案2
            case MinuteFreeze3: //分钟冻结记录方案3
            case MinuteFreeze4: //分钟冻结记录方案4
            case MinuteFreeze5: //分钟冻结记录方案5
            case MinuteFreeze6: //分钟冻结记录方案6
            case MinuteFreeze7: //分钟冻结记录方案7
            case MinuteFreeze8: //分钟冻结记录方案8 
                freez_scheme = (mesg->id - MinuteFreeze1);//(uint8_t)(0x07 & (uint8_t)(oad >> 13))
                Oad = 0x50020200 + (freez_scheme<<13);
                mode = FRZ_1 + freez_scheme;                
                break;     

            case SeasonTabCutFreeze:    //时区表切换冻结
            case DayTabCutFreeze:       //日时段表切换冻结
            case FeePriceCutFreeze:     //费率电价切换冻结
            case StepCutFreeze:         //阶梯切换冻结
            case MonthFreeze:           //月冻结
            case BillFreeze:            //结算日冻结
            case DayFreeze:             //日冻结
            case HourFreeze:            //小时冻结
                Oad = 0x50030200 + ((mesg->id - HourFreeze)<<16);
                mode = FRZ_1;
                break;

            case StepBillFreeze:        //阶梯结算冻结
                Oad = 0x50110200;
                mode = FRZ_1;
                break;
            default:

                break;
        }

        if (Oad == 0xFFFFFFFF)
        {
            return 0;
        }
        //以下部分共用
        //获取 存储地址信息
        ret = get_record_id_info(instance, Oad, &record_id_info);
        if (ret <= 0)
        {
            return -1;
        }

        //获取 关联对象列表
        get_capture_list(instance, Oad, CaptureList, 3);

        //读取关联对象数据
        pdata += read_record_capture(instance, pdata, CaptureList, mode, &remain_size, &record_id_info);

        // 读取序号 ++
        if ((pairs_event != 1) || (mode != END_MODE)) 
        {
            nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
            mem_swap_cpy(data_buff + 1, (uint8_t*)&sequence, 4);
            sequence++;
            nd_set_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
        }

        //调用ND_SET保存数据
        if (mode == END_MODE)
        {
            nd_log_append_revise(instance, record_id_info.save_index, data_buff, 0, (pdata - data_buff));
        }
        else
        {
            nd_log_append(instance, record_id_info.save_index, data_buff, (pdata - data_buff));
        }


    }
    return 0;//返回0代表结束
}

