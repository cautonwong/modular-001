#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"


extern const uint32_t record_head_t1[];
extern const uint32_t record_head_t0[];


extern void* get_pipeline_ctx(struct instance_t* instance, pid_t pipeline_id);

extern int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_6_Collection(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_e_Monitor(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf);

#define POINTER_EMPTY(a)  if(a == NULL) { return -1; }
struct record_item_t
{
    uint16_t  event_flag_id_start;  // 事件代码
    uint16_t  event_flag_id_end;    // 事件代码2   若无填0
    uint32_t len;                   // 申请e2p时 为字节长度，申请flash时为扇区数
    uint32_t max_depth;             // 默认最大深度
};

struct parameter_item_t
{
    uint16_t paramerer_index;
    uint16_t same;
    uint16_t len;
};

//一个表格只能放在一个存储器上，注册的时候绑定设备
const struct record_item_t  record_item[] =
{
    { OverCurrentAS ,       OverCurrentAE,              800,       10 },// 过流 预留空间待调整
    { PowerOffS     ,       PowerOffE,                 4000,      100 },//  0x3011    电能表掉电事件             
    { MeterWrS      ,       0,                          600,       10 },//  0x3012    电能表编程事件             
    { MeterClrS     ,       0,                          600,       10 },//  0x3013    电能表清零事件             
    { EventClrS     ,       0,                          600,       10 },//  0x3015    电能表事件清零事件         
    { SetTimeS      ,       0,                          600,       10 },//  0x3016    电能表校时事件             
    { DayTabWrS     ,       0,                          600,       10 },//  0x3017    电能表时段表编程事件       
    { SeasonTabWrS  ,       0,                          600,       10 },//  0x3018    电能表时区表编程事件       
    { WeekTabWrS    ,       0,                          600,       10 },//  0x3019    电能表周休日编程事件       
    { BillDayWrS    ,       0,                          600,       10 },//  0x301A    电能表结算日编程事件  
    { CoverOpenS    ,       CoverOpenE,                 600,       10 },//  0x301B    电能表开盖事件             
    { SwitchOffS    ,       0,                          600,       10 },//  0x301F    电能表跳闸事件             
    { SwitchOnS     ,       0,                          600,       10 },//  0x3020    电能表合闸事件             
    { HolidayWrS    ,       0,                          600,       10 },//  0x3021    电能表节假日编程事件       
    { ActCombWrS    ,       0,                          600,       10 },//  0x3022    电能表有功组合方式编程事件 
    { FeeTabWrS     ,       0,                          600,       10 },//  0x3024    电能表费率参数表编程事件   
    { StepTabWrS    ,       0,                          600,       10 },//  0x3025    电能表阶梯表编程事件       
    { SecretKeyWrS  ,       0,                          600,       10 },//  0x3026    电能表密钥更新事件 
    { ErrPlugCardS  ,       0,                          600,       10 },//  0x3027    电能表异常插卡事件         
    { RechargeS     ,       0,                          600,       10 },//  0x3028    电能表购电记录             
    { RefundS       ,       0,                          600,       10 },//  0x3029    电能表退费记录  
    { SwitchErrS    ,       SwitchErrE,                 600,       10 },//  0x302B    电能表负荷开关误动作事件   
    { PowerAbnormalS,       PowerAbnormalE,             600,       10 },//  0x302C    电能表电源异常事件 
    { RTCErrS       ,       RTCErrE,                    600,       10 },//  0x302E    电能表时钟故障事件 
    { EmuErrS       ,       EmuErrE,                    600,       10 },//  0x302F    电能表计量芯片故障事件
    { BroadcastTimeS,       0,                          600,       10 },//  0x303C    广播校时事件 
    { ZeroLineErrS  ,       ZeroLineErrE,               600,       10 },//  0x3040    零线电流异常事件 

    { BillFreeze,           0,                         4000,       10 },// 结算日冻结
};


//一个表格只能放在一个存储器上，注册的时候绑定设备   FLASH  分钟冻结
const struct record_item_t  record_item_1[] =
{
    { MinuteFreeze1, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze2, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze3, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze4, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze5, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze6, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze7, 0,    830 * 4096,    35040 },// 待调整
    { MinuteFreeze8, 0,    830 * 4096,    35040 },// 待调整
};


//一个表格只能放在一个存储器上，注册的时候绑定设备   FLASH  其他
const struct record_item_t  record_item_2[] =
{
    { HourFreeze , 0,     5 * 4096,    254 },//小时冻结
    { DayFreeze  , 0,    27 * 4096,    365 },//日冻结
    { MonthFreeze, 0,     5 * 4096,     24 },//日冻结
};


const struct parameter_item_t  parameter_item[] =
{
    // 管理区

    //冻结管理区
    { MinuteFreeze1                 , 0x01,   sizeof(struct log_manager_t)},// 分冻结8个方案，管理区申请8个？
    { MinuteFreeze2                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze3                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze4                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze5                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze6                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze7                 , 0x01,   sizeof(struct log_manager_t)},
    { MinuteFreeze8                 , 0x01,   sizeof(struct log_manager_t)},

    { HourFreeze                    , 0x01,   sizeof(struct log_manager_t)},  //小时冻结  管理区
    { DayFreeze                     , 0x01,   sizeof(struct log_manager_t)},  //日冻结    管理区
    { BillFreeze                    , 0x01,   sizeof(struct log_manager_t)},  //结算日冻结 管理区
    { MonthFreeze                   , 0x01,   sizeof(struct log_manager_t)},  //月冻结 管理区 
    { SeasonTabCutFreeze            , 0x01,   sizeof(struct log_manager_t)},  //时区表切换冻结 管理区
    { DayTabCutFreeze               , 0x01,   sizeof(struct log_manager_t)},  //日时段表切换冻结 管理区
    { FeePriceCutFreeze             , 0x01,   sizeof(struct log_manager_t)},  //费率电价切换冻结 管理区
    { StepCutFreeze                 , 0x01,   sizeof(struct log_manager_t)},  //阶梯切换冻结 管理区
    { StepBillFreeze                , 0x01,   sizeof(struct log_manager_t)},  //阶梯结算冻结 管理区
    { InstantFreeze                 , 0x01,   sizeof(struct log_manager_t)},  //瞬时冻结 管理区
                                                         
    //事件管理区
    { OverCurrentAS                  , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { PowerOffS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { MeterWrS                       , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { MeterClrS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { EventClrS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SetTimeS                       , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { DayTabWrS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SeasonTabWrS                   , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { WeekTabWrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { BillDayWrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { CoverOpenS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SwitchOffS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SwitchOnS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { HolidayWrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { ActCombWrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { FeeTabWrS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { StepTabWrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SecretKeyWrS                   , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { ErrPlugCardS                   , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { RechargeS                      , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { RefundS                        , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { SwitchErrS                     , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { PowerAbnormalS                 , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { RTCErrS                        , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { EmuErrS                        , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { BroadcastTimeS                 , 0x01,   sizeof(struct log_manager_t)},// 管理区
    { ZeroLineErrS                   , 0x01,   sizeof(struct log_manager_t)},// 管理区


    //事件关联对象
    { OverCurrentAS_oad             , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表A相过流事件
    { PowerOffS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表掉电事件            
    { MeterWrS_oad                  , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表编程事件            
    { MeterClrS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表清零事件            
    { EventClrS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表事件清零事件        
    { SetTimeS_oad                  , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表校时事件            
    { DayTabWrS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表时段表编程事件      
    { SeasonTabWrS_oad              , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表时区表编程事件      
    { WeekTabWrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表周休日编程事件      
    { BillDayWrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表结算日编程事件  
    { CoverOpenS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表开盖事件            
    { SwitchOffS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表跳闸事件            
    { SwitchOnS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表合闸事件            
    { HolidayWrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表节假日编程事件      
    { ActCombWrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表有功组合方式编程事件
    { FeeTabWrS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表费率参数表编程事件  
    { StepTabWrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表阶梯表编程事件      
    { SecretKeyWrS_oad              , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表密钥更新事件 
    { ErrPlugCardS_oad              , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表异常插卡事件        
    { RechargeS_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表购电记录            
    { RefundS_oad                   , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表退费记录  
    { SwitchErrS_oad                , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表负荷开关误动作事件  
    { PowerAbnormalS_oad            , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表电源异常事件 
    { RTCErrS_oad                   , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表时钟故障事件 
    { EmuErrS_oad                   , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   电能表计量芯片故障事件
    { BroadcastTimeS_oad            , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   广播校时事件 
    { ZeroLineErrS_oad              , 0x01,  61 }, //oad列表 1+ 4*15     默认8个   零线电流异常事件 
     

    //冻结关联对象
    { MinuteFreeze_oad              , 0x01,  61 }, //oad列表 1+ 4*15     默认7个
    { HourFreeze_oad                , 0x01,  61 }, //oad列表 1+ 4*15     
    { DayFreeze_oad                 , 0x01,  61 }, //oad列表 1+ 4*15     
    { BillFreeze_oad                , 0x01,  61 }, //oad列表 1+ 4*15   
    { MonthFreeze_oad               , 0x01,  61 }, //oad列表 1+ 4*15     
    { SeasonTabCutFreeze_oad        , 0x01,  61 }, //oad列表 1+ 4*15   
    { DayTabCutFreeze_oad           , 0x01,  61 }, //oad列表 1+ 4*15   
    { FeePriceCutFreeze_oad         , 0x01,  61 }, //oad列表 1+ 4*15   
    { StepCutFreeze_oad             , 0x01,  61 }, //oad列表 1+ 4*15   
    { StepBillFreeze_oad            , 0x01,  61 }, //oad列表 1+ 4*15   
    { InstantFreeze_oad             , 0x01,  61 }, //oad列表 1+ 4*15   


    //事件记录序号
    { OverCurrentAS_seq             , 0x01,  4   },//记录序号  电能表A相过流
    { PowerOffS_seq                 , 0x01,  4   },//记录序号  电能表掉电事件            
    { MeterWrS_seq                  , 0x01,  4   },//记录序号  电能表编程事件            
    { MeterClrS_seq                 , 0x01,  4   },//记录序号  电能表清零事件            
    { EventClrS_seq                 , 0x01,  4   },//记录序号  电能表事件清零事件        
    { SetTimeS_seq                  , 0x01,  4   },//记录序号  电能表校时事件            
    { DayTabWrS_seq                 , 0x01,  4   },//记录序号  电能表时段表编程事件      
    { SeasonTabWrS_seq              , 0x01,  4   },//记录序号  电能表时区表编程事件      
    { WeekTabWrS_seq                , 0x01,  4   },//记录序号  电能表周休日编程事件      
    { BillDayWrS_seq                , 0x01,  4   },//记录序号  电能表结算日编程事件  
    { CoverOpenS_seq                , 0x01,  4   },//记录序号  电能表开盖事件            
    { SwitchOffS_seq                , 0x01,  4   },//记录序号  电能表跳闸事件            
    { SwitchOnS_seq                 , 0x01,  4   },//记录序号  电能表合闸事件            
    { HolidayWrS_seq                , 0x01,  4   },//记录序号  电能表节假日编程事件      
    { ActCombWrS_seq                , 0x01,  4   },//记录序号  电能表有功组合方式编程事件
    { FeeTabWrS_seq                 , 0x01,  4   },//记录序号  电能表费率参数表编程事件  
    { StepTabWrS_seq                , 0x01,  4   },//记录序号  电能表阶梯表编程事件      
    { SecretKeyWrS_seq              , 0x01,  4   },//记录序号  电能表密钥更新事件 
    { ErrPlugCardS_seq              , 0x01,  4   },//记录序号  电能表异常插卡事件        
    { RechargeS_seq                 , 0x01,  4   },//记录序号  电能表购电记录            
    { RefundS_seq                   , 0x01,  4   },//记录序号  电能表退费记录  
    { SwitchErrS_seq                , 0x01,  4   },//记录序号  电能表负荷开关误动作事件  
    { PowerAbnormalS_seq            , 0x01,  4   },//记录序号  电能表电源异常事件 
    { RTCErrS_seq                   , 0x01,  4   },//记录序号  电能表时钟故障事件 
    { EmuErrS_seq                   , 0x01,  4   },//记录序号  电能表计量芯片故障事件
    { BroadcastTimeS_seq            , 0x01,  4   },//记录序号  广播校时事件 
    { ZeroLineErrS_seq              , 0x01,  4   },//记录序号  零线电流异常事件 
                                             
     //冻结记录序号                                                                 
    { MinuteFreeze1_seq             , 0x01,  4   },//分钟冻结记录方案1 记录序号
    { MinuteFreeze2_seq             , 0x01,  4   },//分钟冻结记录方案2 记录序号
    { MinuteFreeze3_seq             , 0x01,  4   },//分钟冻结记录方案3 记录序号
    { MinuteFreeze4_seq             , 0x01,  4   },//分钟冻结记录方案4 记录序号
    { MinuteFreeze5_seq             , 0x01,  4   },//分钟冻结记录方案5 记录序号
    { MinuteFreeze6_seq             , 0x01,  4   },//分钟冻结记录方案6 记录序号
    { MinuteFreeze7_seq             , 0x01,  4   },//分钟冻结记录方案7 记录序号
    { MinuteFreeze8_seq             , 0x01,  4   },//分钟冻结记录方案8 记录序号
    { HourFreeze_seq                , 0x01,  4   },//小时冻结  记录序号
    { DayFreeze_seq                 , 0x01,  4   },//日冻结    记录序号
    { BillFreeze_seq                , 0x01,  4   },//月冻结    记录序号
    { MonthFreeze_seq               , 0x01,  4   },//月冻结    记录序号
    { SeasonTabCutFreeze_seq        , 0x01,  4   },//时区表切换冻结冻结    记录序号
    { DayTabCutFreeze_seq           , 0x01,  4   },//日时段表切换冻结冻结    记录序号
    { FeePriceCutFreeze_seq         , 0x01,  4   },//费率电价切换冻结冻结    记录序号
    { StepCutFreeze_seq             , 0x01,  4   },//阶梯切换冻结冻结    记录序号
    { StepBillFreeze_seq            , 0x01,  4   },//阶梯结算冻结冻结    记录序号
    { InstantFreeze_seq             , 0x01,  4   },//瞬时冻结冻结    记录序号
                                             
    //冻结周期                       
    { MinuteFreeze_period           , 0x01, 24 }, //分钟冻结 周期   3*8  
    { HourFreeze_period             , 0x01,  3 }, //小时冻结冻结 周期   3 
    { DayFreeze_period              , 0x01,  3 }, //日冻结冻结 周期   3 
    { BillFreeze_period             , 0x01,  3 }, //结算日冻结冻结 周期   3 
    { MonthFreeze_period            , 0x01,  3 }, //月冻结冻结 周期   3 
    { SeasonTabCutFreeze_period     , 0x01,  3 }, //时区表切换冻结冻结 周期   3 
    { DayTabCutFreeze_period        , 0x01,  3 }, //日时段表切换冻结冻结 周期   3 
    { FeePriceCutFreeze_period      , 0x01,  3 }, //费率电价切换冻结冻结 周期   3 
    { StepCutFreeze_period          , 0x01,  3 }, //阶梯切换冻结冻结 周期   3 
    { StepBillFreeze_period         , 0x01,  3 }, //阶梯结算冻结冻结 周期   3 
    { InstantFreeze_period          , 0x01,  3 }, //瞬时冻结冻结 周期   3 
                                     
    //冻结深度                    
    { MinuteFreeze_depth            , 0x01, 31 }, //分钟冻结 深度     2*15  
    { HourFreeze_depth              , 0x01,  2 }, //分钟冻结 深度     2   
    { DayFreeze_depth               , 0x01,  2 }, //分钟冻结 深度     2   
    { BillFreeze_depth              , 0x01,  2 }, //分钟冻结 深度     2   
    { MonthFreeze_depth             , 0x01,  2 }, //分钟冻结 深度     2   
    { SeasonTabCutFreeze_depth      , 0x01,  2 }, //分钟冻结 深度     2   
    { DayTabCutFreeze_depth         , 0x01,  2 }, //分钟冻结 深度     2   
    { FeePriceCutFreeze_depth       , 0x01,  2 }, //分钟冻结 深度     2   
    { StepCutFreeze_depth           , 0x01,  2 }, //分钟冻结 深度     2   
    { StepBillFreeze_depth          , 0x01,  2 }, //分钟冻结 深度     2   
    { InstantFreeze_depth           , 0x01,  2 }, //分钟冻结 深度     2   


    { AutoDisplayList,     0x64,   2 },//100屏*2字节
    { KeyDisplayList,      0x64,   2 },//100屏*2字节
//    { DisplayPara,         0x01,  22 },
    
    //4XXX
    { Comm_Number,         0x01,   6 },//表地址
    { Meter_Number,        0x01,   6 },//表号
    { User_Number,         0x01,   6 },//客户编号
    { Meter_Num,           0x01,   1 },//计量元件数
    { SeasonTable_Changetime,   0x01,   7 },//两套时区切换时间
    { DayTable_Changetime,      0x01,   7 },//两套时段切换时间
    { SeasonDay_Para,      0x05,   1 },//时区时段参数（年时区数、日时段表数、日时段数、费率数、公共假日数）
    { SecretKey_Num,       0x01,   1 },//秘钥总条数
    { HoliDay_Table,       0x0A,   6 },//公共假日表，10张，每张6字节
    { Weekend,             0x01,   1 },//周休日特征字
    { WeekendFee,          0x01,   1 },//周休日采用费率号
    { Season_Table0,       0x0E,   3 },//当前套时区表，≤14
    { Season_Table1,       0x0E,   3 },//备用套时区表，≤14
    { Day_Table00,         0x08,   42},//当前套日时段表数据
    { Day_Table10,         0x08,   42},//备用套日时段表数据
    { Meter_CT,            0x01,   4 },//CT
    { Meter_PT,            0x01,   4 },//PT
    { Pulse_Width,         0x01,   1 },//脉冲宽度
    { ASCII_AssetsMan,     0x01,   32},//资产管理编码
    { ASCII_Un,            0x01,   6 },//额定电压
    { ASCII_Ib,            0x01,   6 },//额定电流
    { ASCII_Imax,          0x01,   6 },//最大电流
    { ASCII_AccuracyClass1,0x01,   4 },//有功准确度等级
    { Meter_Const1,        0x01,   4 },//有功脉冲常数
    { ASCII_MeterType,     0x01,   32},//电能表型号
    { MeterRunWord,        0x01,   1 },//电能表运行特征字
    { SoftWareNo,          0x01,   16},//软件备案号
    { Measure_Mode,        0x01,   1 },//有功组合特征字
    { Balance_Time,        0x03,   2 },//结算日
    { Meter_Password,      0x01,   8 },//应用连接密码
    { Meter_Location1,     0x01,   10},//设备地理位置经度
    { Meter_Location2,     0x01,   10},//设备地理位置纬度
    { Meter_Location3,     0x01,   5 },//设备地理位置高度
    
    { Device_descriptor,   0x01,   32},//设备描述符
    
    { Factory_No,          0x01,    4}, //厂家代码
    { Version_SoftWare,    0x01,    4}, //软件版本
    { Date_SoftWare,       0x01,    6}, //软件版本日期
    { Version_HardWare,    0x01,    4}, //硬件版本
    { Date_HardWare,       0x01,    4}, //硬件版本日期
    { Extend_Info,         0x01,    6}, //扩展信息
    
    { Produced_Date,       0x01,    7}, //生产日期
//    { Subset_List,         0x05,    2}, //子设备列表(num+2*oi)
    { Protocol_List,       0x01,   20}, //协议列表
    { FollowReport_en,     0x01,    1}, //允许跟随上报
    { ActiveReport_en,     0x01,    1}, //允许主动上报
    { Comm_Master_en,      0x01,    1}, //允许与主站通讯
    { Report_channel,      0x03,    4}, //上报通道
    
    { ImaxProtectValue,    0x01,    4}, //继电器拉闸电流门限值
    { ImaxProtectTime,     0x01,    2}, //超电流门限保护延时时间

};



int16_t supply_parameter_info(uint16_t id, uint32_t* addr, uint16_t* size,uint16_t *offset)
{
    uint16_t i, k;

    POINTER_EMPTY(addr);
    *addr = 0x00;

    for (i = 0; i < sizeof(parameter_item) / sizeof(parameter_item[0]); i++)
    {
        *addr += parameter_item[i].len * parameter_item[i].same + 0x02;  //加两个字节的校验码；
        if ((parameter_item[i].paramerer_index + (parameter_item[i].same - 0x01) >= id) && 
            (parameter_item[i].paramerer_index <= id))
        {
            POINTER_EMPTY(size);
            *size = parameter_item[i].same * parameter_item[i].len + 0x02;
            
            POINTER_EMPTY(offset);
            *offset = (id - parameter_item[i].paramerer_index) * parameter_item[k].len;

            return 1;
        }
    }
    *addr = 0x00;
    return -1;
}


int16_t supply_event_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i, k;

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item) / sizeof(record_item[0]); i++)
    {
        *addr += record_item[i].len;
        if ((event_flag == record_item[i].event_flag_id_start) || 
            (event_flag == record_item[i].event_flag_id_end))
        {
            POINTER_EMPTY(size);
            *size = record_item[i].len;

            POINTER_EMPTY(depth); 
            *depth = record_item[i].max_depth;

            return 1;
        }
    }
    *addr = 0x00;
    return -1;
}

int16_t supply_event_flash_1_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i, k;

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item_1) / sizeof(record_item_1[0]); i++)
    {

        *addr += record_item_1[i].len;
        if ((event_flag == record_item_1[i].event_flag_id_start) ||
            (event_flag == record_item_1[i].event_flag_id_end))
        {
            POINTER_EMPTY(addr);
            *size = record_item_1[i].len;

            POINTER_EMPTY(depth);
            *depth = record_item_1[i].max_depth;

            return 1;
        }
    }
    *addr = 0x00;
    return -1;
}

int16_t supply_event_flash_2_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i, k;

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item_2) / sizeof(record_item_2[0]); i++)
    {

        *addr += record_item_2[i].len;

        if ((event_flag == record_item_2[i].event_flag_id_start) ||
            (event_flag == record_item_2[i].event_flag_id_end))
        {
            POINTER_EMPTY(addr);
            *size = record_item_2[i].len;

            POINTER_EMPTY(depth);
            *depth = record_item_2[i].max_depth;

            return 1;
        }
    }
    *addr = 0x00;
    return -1;
}
//快照记录头类型
// 冻结记录头
const uint32_t record_head_freez[] = {2, 0x20230200, 0x20210200 };

// 标准事件记录头   事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态
const uint32_t record_head_t0[] = {5, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200};

// 分项事件记录头    事件记录序号、事件发生时间、事件结束时间、事件上报状态
const uint32_t record_head_t1[] = {4, 0x20220200, 0x201E0200, 0x20200200, 0x33000200};
// 其他事件记录头种类 待添加



// 通过oad 获取 事件相关的存储地址信息
int get_record_id_info(struct instance_t* instance, uint32_t oad, struct record_storage_id_info_t* info)
{
    
    uint16_t SaveId = 0, SeqId = 0, CaptureId = 0, PeriodId = 0, DepthId = 0;
    uint8_t SaveDevice, SaveBlock;
    uint8_t schem = get_OAD_Schem(oad);
    uint8_t temp = 0;
    uint16_t OI;
    uint8_t attr, class = 7;

    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    memset((uint8_t*)info, 0, sizeof(struct record_storage_id_info_t));
    
    OI = get_OAD_OI(oad);
    attr = get_OAD_Attribute(oad);

    if (((OI >= 0x3000) && (OI <= 0x3008)) || (OI == 0x300B))
    {
        class = 24;
    }


    
    info->record_head = record_head_t0; //默认事件记录单元

    switch (oad&0xF0000000)
    {
        case 0x30000000: //事件类
        {
            info->capture_len = 61;
            info->record_head = record_head_t0;     // 记录单元
            info->acc_time = NULL;
            switch (oad&0xFFFF0000)// OI
            {

                case 0x30050000:
                {
                    SaveId = OverCurrentAS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = OverCurrentAS_oad;          // 关联对象存储地址信息
                    SeqId = OverCurrentAS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->OverCurrentAS_acc_time;
                }
                break;
                
                case 0x30110000:
                {
                    SaveId = PowerOffS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = PowerOffS_oad;          // 关联对象存储地址信息
                    SeqId = PowerOffS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->PowerOffS_acc_time;
                }
                break;
                case 0x30120000:
                {
                    SaveId = MeterWrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = MeterWrS_oad;          // 关联对象存储地址信息
                    SeqId = MeterWrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x30130000:
                {
                    SaveId = MeterClrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = MeterClrS_oad;          // 关联对象存储地址信息
                    SeqId = MeterClrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;

                case 0x301A0000:
                case 0x30190000:
                case 0x30180000:
                case 0x30170000:
                case 0x30160000:
                case 0x30150000:
                {
                    temp = (((oad & 0xFFFF0000) - 0x30150000) >> 16);
                    SaveId = EventClrS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = EventClrS_oad + temp;          // 关联对象存储地址信息
                    SeqId = EventClrS_seq + temp;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x301B0000:
                {
                    SaveId = CoverOpenS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = CoverOpenS_oad;          // 关联对象存储地址信息
                    SeqId = CoverOpenS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->CoverOpenS_acc_time;
                }
                break;


                case 0x30220000:
                case 0x30210000:
                case 0x30200000:
                case 0x301F0000:
                {
                    temp = (((oad & 0xFFFF0000) - 0x301F0000) >> 16);
                    SaveId = SwitchOffS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = SwitchOffS_oad + temp;          // 关联对象存储地址信息
                    SeqId = SwitchOffS_seq + temp;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x30290000:
                case 0x30280000:
                case 0x30270000:
                case 0x30260000:
                case 0x30250000:
                case 0x30240000:
                {
                    temp = (((oad & 0xFFFF0000) - 0x30240000) >> 16);
                    SaveId = FeeTabWrS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = FeeTabWrS_oad + temp;          // 关联对象存储地址信息
                    SeqId = FeeTabWrS_seq + temp;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x302B0000:
                {
                    SaveId = SwitchErrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = SwitchErrS_oad;          // 关联对象存储地址信息
                    SeqId = SwitchErrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->SwitchErrS_acc_time;
                }
                break;
                case 0x302C0000:
                {
                    SaveId = PowerAbnormalS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = PowerAbnormalS_oad;          // 关联对象存储地址信息
                    SeqId = PowerAbnormalS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->PowerAbnormalS_acc_time;
                }
                break;
                case 0x302E0000:
                {
                    SaveId = RTCErrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = RTCErrS_oad;          // 关联对象存储地址信息
                    SeqId = RTCErrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x302F0000:
                {
                    SaveId = EmuErrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = EmuErrS_oad;          // 关联对象存储地址信息
                    SeqId = EmuErrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->EmuErrS_acc_time;
                }
                break;
                case 0x303C0000:
                {
                    SaveId = BroadcastTimeS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = BroadcastTimeS_oad;          // 关联对象存储地址信息
                    SeqId = BroadcastTimeS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x30400000:
                {
                    SaveId = ZeroLineErrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = ZeroLineErrS_oad;          // 关联对象存储地址信息
                    SeqId = ZeroLineErrS_seq;              // 序号存储地址信息
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->ZeroLineErrS_acc_time;
                }
                break;

                default:
                    return -1;
                    break;
            } 
            
        }
        break;
        case 0x50000000: //冻结类
        {
            info->capture_len = 61;
            info->period_len = 3;
            info->depth_len = 2;
            info->record_head = record_head_freez;  // 记录单元
            switch (oad&0xFFFF0000)// OI
            {
                case 0x50000000://瞬时冻结
                {
                    SaveId = InstantFreeze;                    // 记录存储地址信息
                    SaveDevice = RES_DEV_FLASH;
                    SaveBlock = mbase_block_event_flash2;
                    CaptureId = InstantFreeze_oad;             // 关联对象存储地址信息
                    SeqId = InstantFreeze_seq;                 // 序号存储地址信息
                    PeriodId = InstantFreeze_period;           // 周期存储地址信息
                    DepthId = InstantFreeze_depth;             // 深度存储地址信息
                }
                break;
                case 0x50020000://8方案 待调整
                {
                    SaveId = MinuteFreeze1;                 // + schem 记录存储地址信息
                    SaveDevice = RES_DEV_FLASH;
                    SaveBlock = mbase_block_event_flash1;
                    CaptureId = MinuteFreeze_oad;           // 关联对象存储地址信息
                    SeqId = MinuteFreeze1_seq;              // 序号存储地址信息
                    PeriodId = MinuteFreeze_period;         // 周期存储地址信息
                    DepthId = MinuteFreeze_depth;           // 深度存储地址信息
                    info->period_len = 24;
                    info->depth_len = 31;
                }
                break;

                case 0x50030000://小时冻结
                case 0x50040000://日冻结
                //case 0x50050000://结算日冻结
                case 0x50060000://月冻结
                //case 0x50070000://年冻结 （不要求，但需占用位置）
                case 0x50080000://时区表切换冻结
                case 0x50090000://日时段表切换冻结
                case 0x500A0000://费率电价切换冻结
                case 0x500B0000://阶梯切换冻结
                {
                    temp = (((oad & 0xFFFF0000) - 0x50030000) >> 16);
                    SaveId = HourFreeze + temp;                    // 记录存储地址信息
                    SaveDevice = RES_DEV_FLASH;
                    SaveBlock = mbase_block_event_flash2;
                    CaptureId = HourFreeze_oad + temp;             // 关联对象存储地址信息
                    SeqId = HourFreeze_seq + temp;                 // 序号存储地址信息
                    PeriodId = HourFreeze_period + temp;           // 周期存储地址信息
                    DepthId = HourFreeze_depth + temp;             // 深度存储地址信息
                }
                break;

                case 0x50050000: //结算日冻结
                {
                    SaveId = BillFreeze;                    // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = BillFreeze_oad;             // 关联对象存储地址信息
                    SeqId = BillFreeze_seq;                 // 序号存储地址信息
                    PeriodId = BillFreeze_period;           // 周期存储地址信息
                    DepthId = BillFreeze_depth;             // 深度存储地址信息
                }
                break;

                case 0x50110000://阶梯结算冻结
                {
                    SaveId = StepBillFreeze;                    // 记录存储地址信息
                    SaveDevice = RES_DEV_FLASH;
                    SaveBlock = mbase_block_event_flash2;
                    CaptureId = StepBillFreeze_oad;             // 关联对象存储地址信息
                    SeqId = StepBillFreeze_seq;                 // 序号存储地址信息
                    PeriodId = StepBillFreeze_period;           // 周期存储地址信息
                    DepthId = StepBillFreeze_depth;             // 深度存储地址信息
                    break;
                }
                break;
                default:
                    return -1;
                    break;
            }
            
        }
        break;
        default:
            return -1;
            break;
    }


    info->save_index = ((SaveDevice << 24) + (SaveBlock << 16) + SaveId);
    info->capture_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + CaptureId);
    info->sequence_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeqId);

    if (PeriodId)
    {
        info->period_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodId);
    }
    if (DepthId)
    {
        info->depth_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DepthId);
    }

    return 1;
}

// 通过oad 获取 事件rcsd
// mode = 1，获取记录头；   mode=2，获取配置关联对象， mode = 3，获取所有关联对象
int get_capture_list(struct instance_t* instance, uint32_t oad, uint32_t *rcsd_list, uint8_t mode)
{
    uint8_t i, num;
    uint32_t *p = (rcsd_list + 1);
    uint32_t rcsd_num = 0;
    uint8_t tempbuf[200];//待调整  读取e2p关联对象缓存
    struct record_storage_id_info_t info;
    int ret = get_record_id_info(instance, oad, &info);
    
    if (ret < 0)
    {
        return -1;
    }
   
    if(mode & 0x01)
    {
        if (info.record_head != NULL)
        {
            num = info.record_head[0];
            
            if (info.record_head[0] > REQ_RCSD_MAXNUM)
            {
                return -1;
            }
            
            for(i = 0; i < num; i++)
            {
                memcpy(p, (info.record_head+1+i), 4);
                p++;
                rcsd_num += 1;
            }
        }
    }

    if(mode & 0x02)
    {  
        nd_get_index(instance, info.capture_index, &tempbuf[0], sizeof(tempbuf));
     
        num = tempbuf[0];
        if (num > REQ_RCSD_MAXNUM)
        {
            return -1;
        }

        for(i = 0; i < num; i++)
        {
            memcpy(p, (tempbuf+1+4*i), 4);
            p++;
            rcsd_num += 1;
        }
     }  
    *rcsd_list =  rcsd_num;
    
    return 1;
}



// data_time_s 转4字节
int fc_fun_01(uint8_t *des, uint8_t* src, enum format_conversion_mode mode)
{
    uint32_t temp;
    uint8_t *p = des;

    //tag 处理
    *p++ = *src++;
    if (mode == RFC_WRITE)
    {
        temp = dt_buf_to_second(src);
        p += uint32_to_buf_msb(temp, p);
    }
    else if (mode == RFC_READ)
    {
        temp = buf_to_uint32_msb(src);
        p += second_to_dt_buf(temp, p);  
    }
    return (p - des);
}
// record存储格式特殊处理的 oad
const struct format_conversion_t format_conversion_tab[] =
{
    {   0x201E0200,     8,      5,      fc_fun_01   },
    {   0x20200200,     8,      5,      fc_fun_01   },
    {   0x20210200,     8,      5,      fc_fun_01   },
};

//enum format_conversion_mode // record_change_format
//
//    RFC_WRITE,           //存储写入转换
//    RFC_READ,            //协议读出转换
//    RFC_GET_ORIGINAL_LEN,//获取原协议格式len
//    RFC_GET_STORE_LEN,   //获取存储格式len
//
int record_change_format(uint8_t *des,  uint8_t* src, uint32_t oad, enum format_conversion_mode mode)
{
    uint16_t i;

    for (i = 0; i < (sizeof(format_conversion_tab) / sizeof(format_conversion_tab[0])); i++)
    {
        if (oad == format_conversion_tab[i].oad)
        {
            break;
        }
    }

    if (i >= (sizeof(format_conversion_tab) / sizeof(format_conversion_tab[0])))
    {
        return -1;
    }

    if (mode == RFC_GET_ORIGINAL_LEN)
    {
        return format_conversion_tab[i].original_len;
    }
    else if (mode == RFC_GET_STORE_LEN)
    {
        return format_conversion_tab[i].store_len;
    }
    else
    {
        return format_conversion_tab[i].format_fun(des, src, mode);
    }
}


int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode)
{
    uint8_t OIA1 = get_OAD_OIA1(id);
    switch (mode)
	{
    case RESOURCE_MAP_GET:
        switch (OIA1)
        {
        case 0x0://电量
            return GET_0_Energy(instance, id, buf);
            break;
        case 0x1://需量
            return GET_1_Demand(instance, id, buf);
            break;
        case 0x2://变量
            return GET_2_Variable(instance, id, buf);
            break;
        case 0x3://事件
            return GET_3_Event(instance, id, buf);
            break;
        case 0x4://参变量
            return GET_4_Parameter(instance, id, buf);
            break;
        case 0x5://冻结
            return GET_5_Frozen(instance, id, buf);
            break;
        case 0x6://采集
            return GET_6_Collection(instance, id, buf);
            break;
        case 0x7://集合
            return GET_7_Aggregate(instance, id, buf);
            break;
        case 0x8://控制
            return GET_8_Control(instance, id, buf);
            break;
        case 0xe://监控
            return GET_e_Monitor(instance, id, buf);
            break;
        case 0xf://其他
            return GET_f_Other(instance, id, buf);
            break;
        default:
            break;
        }
        break;
    case RESOURCE_MAP_SET:
      	switch (OIA1)
        {
        case 0x3:// 事件
            return SET_3_Event(instance, id, buf);
        case 0x4://参变量
            return SET_4_Parameter(instance, id, buf);
        case 0x5:// 事件
            return SET_5_Frozen(instance, id, buf);
        case 0x8:// 控制类
            return SET_8_Control(instance, id, buf);
        case 0xf://其他
            return SET_f_Other(instance, id, buf);
        default:
            break;
        }
        break;
    case RESOURCE_MAP_ACTION:
        switch (OIA1)
        {
        case 0x4://参变量
            return ACTION_4_Parameter(instance, id, buf);
        case 0x8://控制类
            return ACTION_8_Parameter(instance, id, buf);
        default:
            break;
        }
        break;
	default:
		break;
	}
	return -1;
}

#if 0
#define DT_END 0xED

#define calnum(x)                    (sizeof(x) / sizeof(x[0]))

struct attr_tag_t
{
    uint8_t attr;
    const uint8_t* tags;
};

struct data_tag_t
{
    uint16_t oi1;
    uint16_t oi2;
    const struct attr_tag_t *dtags;
    uint8_t number;
    uint8_t info;
};

struct tag_len_t
{
    uint8_t tag;
    uint8_t datalen;
};


//注意 长度  0的 可能不需要，也可能未 核对。
const struct tag_len_t tag_data_len[] =
{
     {  TYPE_null                   ,  0    },
     {  TYPE_array                  ,  0    },
     {  TYPE_structure              ,  0    },
     {  TYPE_bool                   ,  0    },
     {  TYPE_bit_string             ,  0    },
     {  TYPE_double_long            ,  4    },
     {  TYPE_double_long_unsigned   ,  4    },

     {  TYPE_octet_string            ,  0xFF    },
     {  TYPE_visible_string          ,  0xFF    },
     {  TYPE_UTF8_string             ,  0xFF    },

     {  TYPE_integer                ,  1    },
     {  TYPE_long                   ,  2    },
     {  TYPE_unsigned               ,  1    },
     {  TYPE_long_unsigned          ,  2    },

     {  TYPE_long64                 ,  8    },
     {  TYPE_long64_unsigned        ,  8    },
     {  TYPE_enum                    ,  0    },
     {  TYPE_float32                 ,  4    },
     {  TYPE_float64                 ,  8    },
     {  TYPE_date_time               ,  10    },
     {  TYPE_date                    ,  0    },
     {  TYPE_time                    ,  0    },
     {  TYPE_date_time_s              ,  7    },
     {  TYPE_OI                      ,  2    },
     {  TYPE_OAD                     ,  4    },
     {  TYPE_ROAD                    ,  0    },
     {  TYPE_OMD                     ,  4    },
     {  TYPE_TI                      ,  0    },
     {  TYPE_TSA                     ,  0    },
     {  TYPE_MAC                     ,  0    },
     {  TYPE_RN                      ,  0    },
     {  TYPE_Region                  ,  0    },
     {  TYPE_Scaler_Unit             ,  0    },
     {  TYPE_RSD                     ,  0    },
     {  TYPE_CSD                     ,  0    },
     {  TYPE_MS                      ,  0    },
     {  TYPE_SID                     ,  0    },
     {  TYPE_SID_MAC                     ,  0    },
     {  TYPE_COMDCB                      ,  0    },
     {  TYPE_RCSD                        ,  0    },
     {  TYPE_VQDS                        ,  0    },

     {  TYPE_USER_HIGH_ENERGY_INTEGER    ,  0    },
     {  TYPE_USER_HIGH_ENERGY_UNSIGNED   ,  0    },
     {  TYPE_USER_NOT_CARE               ,  0    },
     {  TYPE_USER_SPECIAL_PROCESS        ,  0    },
     {  TYPE_USER_PUSH_PROCESS           ,  0    },
     {  TYPE_USER_ERROR                  ,  0    },
};



const uint8_t _0000_02_tag[] = { TYPE_array, 13, TYPE_double_long, DT_END };
const uint8_t _0000_04_tag[] = { TYPE_array, 13, TYPE_long64, DT_END };
const uint8_t _0000_06_tag[] = { TYPE_array, 13, TYPE_integer, DT_END };
const struct attr_tag_t _0000_tag[] = { {2, _0000_02_tag}, {4, _0000_04_tag}, {6, _0000_06_tag} };

const uint8_t _2022_02_tag[] = { TYPE_double_long_unsigned, DT_END };
const struct attr_tag_t _2022_tag[] = { {2, _2022_02_tag}};

const uint8_t _201E_02_tag[] = { TYPE_date_time_s, DT_END };
const struct attr_tag_t _201E_tag[] = { {2, _201E_02_tag} };

const uint8_t _NULL_02_tag[] = { TYPE_null, DT_END };
const struct attr_tag_t _NULL_tag[] = { {2, _NULL_02_tag} };

const struct data_tag_t data_tag[] =
{
    {   0x0000, 0x0000,    _0000_tag,  calnum(_0000_tag),  0    }, // 组合有功
    {   0x201E, 0x2021,    _201E_tag,  calnum(_201E_tag),  0    }, // 记录时间
    {   0x2022, 0x2023,    _2022_tag,  calnum(_2022_tag),  0    }, // 记录序号
    {   0x2024, 0x2024,    _NULL_tag,  calnum(_NULL_tag),  0    }, // 事件发生源
    {   0x3300, 0x3300,    _NULL_tag,  calnum(_NULL_tag),  0    }, // 事件上报状态
    //待添加

};

const uint8_t*  get_tag(uint32_t oad)
{
    uint16_t i, oi;
    uint8_t index, attr, num, info;
    const struct attr_tag_t* data_tags;
    
    oi = (oad & 0xFFFF0000) >> 16;
    index = oad & 0x000000FF;
    attr = (oad & 0x0000FF00) >> 8;
    num = 0;

    // -------------get tag start
    for (i = 0; i < calnum(data_tag); i++)
    {
        if ((oi >= data_tag[i].oi1) && (oi <= data_tag[i].oi2))
        {
            data_tags = data_tag[i].dtags;
            num = data_tag[i].number;
            info = data_tag[i].info;
            break;
        }
    }

    if (i >= calnum(data_tag))
    {
        return _NULL;
    }

    // 不同的索引tag 不一样
    for (i = 0; i < num; i++)
    {
        if (attr == data_tags->attr)
        {
            return data_tags->tags;
        }
    }

    return _NULL;

}
int add_tag(uint8_t* des, uint8_t* src, uint8_t* tags)
{
    uint16_t i, oi;
    uint8_t index,attr,num,info;
    const struct attr_tag_t* data_tags;
    uint8_t* attr_tags;

    uint8_t * p, * tmp;

    p = des;
    tmp = src;

    
    //*attr_tags



}
//  obj_num 关联对象个数 , srclen 源数据（不带ta）总长度， des_size目标
int record_data_add_tag(uint8_t* des, uint8_t* src, uint32_t* obj_list, uint8_t obj_num, uint16_t srclen, uint16_t des_size)
{
    uint8_t i, *p, *tmp;
    uint16_t taglen;
    uint32 oad;
    uint8_t* attr_tags;
    
    p = des;
    tmp = src;
    // 逐个查询obj list 获取所有tag长度
    for (i = 0; i < obj_num; i++)
    {
        oad = *obj_list++;
        
        attr_tags = get tag(oad);    
        add_tag(p, tmp, oad, attr_tags);

        
    }



}
// 00000200  00000400 
// 01 13 06 
// 00000001 00000001 00000001 00000001
// 01 13 06  XXXX 06 XXXX

#endif