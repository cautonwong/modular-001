#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "fr_698.h"


#define U32toBuf(x)  ((uint8_t)(x)), ((uint8_t)(x>>8)), ((uint8_t)(x>>16)), ((uint8_t)(x>>24))

// 初始化默认数据
// 过流事件关联对象
const uint8_t init_30050200[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t init_302B0200[] = { 0x01,0x05, U32toBuf(0xf2052201), U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t init_50020300[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };


const uint8_t  init_MinuteFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_HourFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_DayFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_BillFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_MonthFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_SeasonTabCutFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_DayTabCutFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_FeePriceCutFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_StepCutFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_StepBillFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
const uint8_t  init_InstantFreeze_oad[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };
                   
const uint8_t  init_OverCurrentAS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_PowerOffS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_MeterWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_MeterClrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_EventClrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SetTimeS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_DayTabWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SeasonTabWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_WeekTabWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_BillDayWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_CoverOpenS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SwitchOffS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SwitchOnS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_HolidayWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_ActCombWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_FeeTabWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_StepTabWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SecretKeyWrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_ErrPlugCardS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_RechargeS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_RefundS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_SwitchErrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_PowerAbnormalS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_RTCErrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_EmuErrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_BroadcastTimeS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };
const uint8_t  init_ZeroLineErrS_oad[] = { 0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201) };





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
    nd_set_index(instance, info.sequence_index, &temp, 4);

    //累计时间清零
    if (info.acc_time)
    {
        *info.acc_time = 0;
    }
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


#if 0
      //OverCurrentAS_oad,   //关联对象    电能表A相过流事件
          //PowerOffS_oad,   //关联对象    电能表掉电事件           停电下未处理
          MeterWrS_oad,   //关联对象    电能表编程事件            
          MeterClrS_oad,   //关联对象    电能表清零事件            
          EventClrS_oad,   //关联对象    电能表事件清零事件        
          SetTimeS_oad,   //关联对象    电能表校时事件   

          DayTabWrS_oad,   //关联对象    电能表时段表编程事件      
          SeasonTabWrS_oad,   //关联对象    电能表时区表编程事件      
          WeekTabWrS_oad,   //关联对象    电能表周休日编程事件      
          BillDayWrS_oad,   //关联对象    电能表结算日编程事件  
          CoverOpenS_oad,   //关联对象    电能表开盖事件            
          SwitchOffS_oad,   //关联对象    电能表跳闸事件            
          SwitchOnS_oad,   //关联对象    电能表合闸事件            
          HolidayWrS_oad,   //关联对象    电能表节假日编程事件      
          ActCombWrS_oad,   //关联对象    电能表有功组合方式编程事件
          FeeTabWrS_oad,   //关联对象    电能表费率参数表编程事件  
          StepTabWrS_oad,   //关联对象    电能表阶梯表编程事件      
          SecretKeyWrS_oad,   //关联对象    电能表密钥更新事件 
          ErrPlugCardS_oad,   //关联对象    电能表异常插卡事件        
          RechargeS_oad,   //关联对象    电能表购电记录            
          RefundS_oad,   //关联对象    电能表退费记录  
          SwitchErrS_oad,   //关联对象    电能表负荷开关误动作事件  
          PowerAbnormalS_oad,   //关联对象    电能表电源异常事件 
          RTCErrS_oad,   //关联对象    电能表时钟故障事件 
          EmuErrS_oad,   //关联对象    电能表计量芯片故障事件
          BroadcastTimeS_oad,   //关联对象    广播校时事件 
          ZeroLineErrS_oad,   //关联对象    零线电流异常事件 
#endif




}

//停电下每秒调用  主要处理事件累计时间
int event_LowPower_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    PowerOffS_LowPower_pro(instance, pipeline, mesg);
    PowerAbnormalS_LowPower_pro(instance, pipeline, mesg);
}
