#include "nidavellir.h"
#include "fr_698.h"


//参数初始化  
extern const uint8_t  init_MinuteFreeze_oad[];
extern const uint8_t  init_HourFreeze_oad[];
extern const uint8_t  init_DayFreeze_oad[];
extern const uint8_t  init_BillFreeze_oad[];
extern const uint8_t  init_MonthFreeze_oad[];
extern const uint8_t  init_SeasonTabCutFreeze_oad[];
extern const uint8_t  init_DayTabCutFreeze_oad[];
extern const uint8_t  init_FeePriceCutFreeze_oad[];
extern const uint8_t  init_StepCutFreeze_oad[];
extern const uint8_t  init_StepBillFreeze_oad[];
extern const uint8_t  init_InstantFreeze_oad[];
extern const uint8_t  init_Event_ReportFlag_3[];  
extern const uint8_t  init_Event_ReportFlag_1[]; 
extern const uint8_t  init_Event_ReportFlag_0[]; 
extern const uint8_t  init_Event_ReportMode_1[]; 
extern const uint8_t  init_Event_ReportMode_0[]; 

extern const uint8_t  init_MinuteFreeze_period[];        
extern const uint8_t  init_HourFreeze_period[];          
extern const uint8_t  init_DayFreeze_period[];           
extern const uint8_t  init_BillFreeze_period[];          
extern const uint8_t  init_MonthFreeze_period[];         
extern const uint8_t  init_SeasonTabCutFreeze_period[];  
extern const uint8_t  init_DayTabCutFreeze_period[];   
extern const uint8_t  init_FeePriceCutFreeze_period[]; 
extern const uint8_t  init_StepCutFreeze_period[];     
extern const uint8_t  init_StepBillFreeze_period[];    
extern const uint8_t  init_InstantFreeze_period[];     

extern const uint8_t  init_MinuteFreeze_depth[];             
extern const uint8_t  init_HourFreeze_depth[];               
extern const uint8_t  init_DayFreeze_depth[];                
extern const uint8_t  init_BillFreeze_depth[];               
extern const uint8_t  init_MonthFreeze_depth[];        
extern const uint8_t  init_SeasonTabCutFreeze_depth[]; 
extern const uint8_t  init_DayTabCutFreeze_depth[];    
extern const uint8_t  init_FeePriceCutFreeze_depth[];  
extern const uint8_t  init_StepCutFreeze_depth[];     
extern const uint8_t  init_StepBillFreeze_depth[];    
extern const uint8_t  init_InstantFreeze_depth[];     

extern const uint8_t  init_BeforSet_SeasonT[];

extern const uint8_t  init_SecurityMode_En[];
extern const uint8_t  init_SecurityMode_EXData[];

extern const uint8_t  init_OverCurrentAS_oad[];
extern const uint8_t  init_PowerOffS_oad[];
extern const uint8_t  init_MeterWrS_oad[];
extern const uint8_t  init_MeterClrS_oad[];
extern const uint8_t  init_EventClrS_oad[];
extern const uint8_t  init_SetTimeS_oad[];
extern const uint8_t  init_DayTabWrS_oad[];
extern const uint8_t  init_SeasonTabWrS_oad[];
extern const uint8_t  init_WeekTabWrS_oad[];
extern const uint8_t  init_BillDayWrS_oad[];
extern const uint8_t  init_CoverOpenS_oad[];
extern const uint8_t  init_SwitchOffS_oad[];
extern const uint8_t  init_SwitchOnS_oad[];
extern const uint8_t  init_HolidayWrS_oad[];
extern const uint8_t  init_ActCombWrS_oad[];
extern const uint8_t  init_FeeTabWrS_oad[];
extern const uint8_t  init_StepTabWrS_oad[];
extern const uint8_t  init_SecretKeyWrS_oad[];
extern const uint8_t  init_ErrPlugCardS_oad[];
extern const uint8_t  init_RechargeS_oad[];
extern const uint8_t  init_RefundS_oad[];
extern const uint8_t  init_SwitchErrS_oad[];
extern const uint8_t  init_PowerAbnormalS_oad[];
extern const uint8_t  init_RTCErrS_oad[];
extern const uint8_t  init_EmuErrS_oad[];
extern const uint8_t  init_BroadcastTimeS_oad[];
extern const uint8_t  init_ZeroLineErrS_oad[];




extern const uint8_t  init_AutoDisplayList[];
extern const uint8_t  init_KeyDisplayList[];

extern const uint8_t   init_Comm_Number[];//表地址
extern const uint8_t   init_Meter_Number[];//表号
extern const uint8_t   init_User_Number[];//客户编号
extern const uint8_t   init_Meter_Num[];//计量元件数
extern const uint8_t   init_SeasonTable_Changetime[];//两套时区切换时间
extern const uint8_t   init_SeasonTable_Changetime[];//两套时段切换时间
extern const uint8_t   init_SeasonDay_Para[];//时区时段参数（年时区数、日时段表
extern const uint8_t   init_SecretKey_Num[];//秘钥总条数
extern const uint8_t   init_HoliDay_Table[];//公共假日表，10张，每张6字节
extern const uint8_t   init_Weekend[];//周休日特征字
extern const uint8_t   init_WeekendFee[];//周休日采用费率号
extern const uint8_t   init_Season_Table0[];//当前套时区表，≤14
extern const uint8_t   init_Season_Table0[];//备用套时区表，≤14
extern const uint8_t   init_Day_Table00[];//当前套日时段表数据
extern const uint8_t   init_Day_Table00[];//备用套日时段表数据
extern const uint8_t   init_Meter_CT[];//CT
extern const uint8_t   init_Meter_PT[];//PT
extern const uint8_t   init_Pulse_Width[];//脉冲宽度
extern const uint8_t   init_ASCII_AssetsMan[];//资产管理编码
extern const uint8_t   init_ASCII_Un[];//额定电压
extern const uint8_t  init_ASCII_Ib[];//额定电流
extern const uint8_t  init_ASCII_Imax[];//最大电流
extern const uint8_t    init_ASCII_Imin[];//最小电流
extern const uint8_t    init_ASCII_Itr[];//转折电流
extern const uint8_t  init_ASCII_AccuracyClass1[];//有功准确度等级
extern const uint8_t  init_Meter_Const1[];//有功脉冲常数
extern const uint8_t  init_ASCII_MeterType[];//电能表型号
extern const uint8_t  init_MeterRunWord[];//电能表运行特征字
extern const uint8_t  init_SoftWareNo[];//软件备案号
extern const uint8_t  init_Measure_Mode[];//有功组合特征字
extern const uint8_t  init_Balance_Time[];//结算日
extern const uint8_t  init_Meter_Password[];//应用连接密码
extern const uint8_t  init_Meter_Location1[];//设备地理位置经度
extern const uint8_t  init_Meter_Location2[];//设备地理位置纬度
extern const uint8_t  init_Meter_Location3[];//设备地理位置高度

extern const uint8_t  init_Device_descriptor[];//设备描述符

extern const uint8_t  init_Factory_No[]; //厂家代码
extern const uint8_t  init_Version_SoftWare[]; //软件版本
extern const uint8_t  init_Date_SoftWare[]; //软件版本日期
extern const uint8_t  init_Version_HardWare[]; //硬件版本
extern const uint8_t  init_Date_HardWare[]; //硬件版本日期
extern const uint8_t  init_Extend_Info[]; //扩展信息

extern const uint8_t  init_Produced_Date[]; //生产日期
extern const uint8_t  init_Subset_List[]; //子设备列表(num+2*oi)
extern const uint8_t  init_Protocol_List[]; //协议列表
extern const uint8_t  init_FollowReport_en[]; //允许跟随上报
extern const uint8_t  init_ActiveReport_en[]; //允许主动上报
extern const uint8_t  init_Comm_Master_en[]; //允许与主站通讯
extern const uint8_t  init_Report_channel[]; //上报通道
extern const uint8_t  init_ImaxProtectValue[]; //继电器拉闸电流门限值
extern const uint8_t  init_ImaxProtectTime[]; //超电流门限保护延时时间

extern const uint8_t  init_IrAuthenTime[];//红外认证时效


extern const uint32_t record_head_t1[];
extern const uint32_t record_head_t0[];


extern void* get_pipeline_ctx(struct instance_t* instance, pid_t pipeline_id);

extern int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, void *ctx);
extern int GET_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, void *ctx);
extern int SET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);

extern int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);

extern int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);

extern int ACTION_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int ACTION_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int ACTION_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int ACTION_8_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *pctx);
extern int ACTION_F_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx);

extern int GET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int SET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_6_Collection(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int SET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_e_Monitor(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);
extern int SET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx);

#define POINTER_EMPTY(a)  if(a == NULL) { return -1; }
struct record_item_t
{
    uint16_t  event_flag_id_start;  // 事件代码
    uint16_t  event_flag_id_end;    // 事件代码2   若无填0
    uint32_t len;                   // 申请e2p时 为字节长度，申请flash时为扇区数
    uint32_t max_depth;             // 默认最大深度
};

typedef struct 
{
    uint16_t paramerer_index;
    uint16_t same;
    uint16_t len;
    const uint8_t  *data;  //默认参数常亮数组地址
}parameter_item_t;

//一个表格只能放在一个存储器上，注册的时候绑定设备
const struct record_item_t  record_item[] =
{
    { OverCurrentAS ,       OverCurrentAE,              630,       10 },// 过流 预留空间待调整
    { PowerOffS     ,       PowerOffE,                 3560,      100 },//  0x3011    电能表掉电事件             
    { MeterWrS      ,       0,                          910,       10 },//  0x3012    电能表编程事件             
    { MeterClrS     ,       0,                          360,       10 },//  0x3013    电能表清零事件             
    { EventClrS     ,       0,                          320,       10 },//  0x3015    电能表事件清零事件         
    { SetTimeS      ,       0,                          360,       10 },//  0x3016    电能表校时事件             
    { DayTabWrS     ,       0,                         1780,       10 },//  0x3017    电能表时段表编程事件       
    { SeasonTabWrS  ,       0,                         1710,       10 },//  0x3018    电能表时区表编程事件       
    { WeekTabWrS    ,       0,                          260,       10 },//  0x3019    电能表周休日编程事件       
    { BillDayWrS    ,       0,                          490,       10 },//  0x301A    电能表结算日编程事件  
    { CoverOpenS    ,       CoverOpenE,                 490,       10 },//  0x301B    电能表开盖事件             
    { SwitchOffS    ,       0,                          360,       10 },//  0x301F    电能表跳闸事件             
    { SwitchOnS     ,       0,                          360,       10 },//  0x3020    电能表合闸事件             
    { HolidayWrS    ,       0,                          430,       10 },//  0x3021    电能表节假日编程事件       
    { ActCombWrS    ,       0,                          270,       10 },//  0x3022    电能表有功组合方式编程事件 
//    { FeeTabWrS     ,       0,                            0,        0 },//  0x3024    电能表费率参数表编程事件   
//    { StepTabWrS    ,       0,                            0,        0 },//  0x3025    电能表阶梯表编程事件       
    { SecretKeyWrS  ,       0,                          100,        2 },//  0x3026    电能表密钥更新事件 
//    { ErrPlugCardS  ,       0,                            0,        0 },//  0x3027    电能表异常插卡事件         
//    { RechargeS     ,       0,                            0,        0 },//  0x3028    电能表购电记录             
//    { RefundS       ,       0,                            0,        0 },//  0x3029    电能表退费记录  
    { SwitchErrS    ,       SwitchErrE,                 710,       10 },//  0x302B    电能表负荷开关误动作事件   
    { PowerAbnormalS,       PowerAbnormalE,             360,       10 },//  0x302C    电能表电源异常事件 
    { RTCErrS       ,       RTCErrE,                    490,       10 },//  0x302E    电能表时钟故障事件 
    { EmuErrS       ,       EmuErrE,                    360,       10 },//  0x302F    电能表计量芯片故障事件
    { BroadcastTimeS,       0,                         3560,      100 },//  0x303C    广播校时事件 
    { ZeroLineErrS  ,       ZeroLineErrE,               330,       10 },//  0x3040    零线电流异常事件 

    { BillFreeze,           0,                         3280,       12 },//  0x5005      结算日冻结 //待调整
    { SeasonTabCutFreeze,   0,                          380,        2 },//  0x5008      时区表切换冻结 
    { DayTabCutFreeze   ,   0,                          380,        2 },//  0x5009      日时段表切换冻结    
//    { FeePriceCutFreeze ,   0,                          100,        2 },//  0x500A      费率电价切换冻结  
//    { StepCutFreeze     ,   0,                            0,        2 },//  0x500B      阶梯切换冻结
//    { StepBillFreeze    ,   0,                            0,        2 },//  0x5011      阶梯结算冻结
    { InstantFreeze     ,   0,                          570,        3 },//  0x5000      瞬时冻结

    { HourFreeze ,          0,                         6610,        254 },//小时冻结
    { MonthFreeze,          0,                         6410,         24 },//月冻结

};



//一个表格只能放在一个存储器上，注册的时候绑定设备   FLASH  分钟冻结
// 计算冻结空间大小处同步修改
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
// 计算冻结空间大小处同步修改
const struct record_item_t  record_item_2[] =
{

    { DayFreeze  , 0,    27 * 4096,    365 },//日冻结

};

const  parameter_item_t  parameter_item[] =
{
    // 管理区
#ifndef WIN32
    //冻结管理区
    { MinuteFreeze1                 , 0x01,   sizeof(struct log_manager_t),0},// 分冻结8个方案，管理区申请8个？
    { MinuteFreeze2                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze3                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze4                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze5                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze6                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze7                 , 0x01,   sizeof(struct log_manager_t),0},
    { MinuteFreeze8                 , 0x01,   sizeof(struct log_manager_t),0},

    { HourFreeze                    , 0x01,   sizeof(struct log_manager_t),0},  //小时冻结  管理区
    { DayFreeze                     , 0x01,   sizeof(struct log_manager_t),0},  //日冻结    管理区
    { BillFreeze                    , 0x01,   sizeof(struct log_manager_t),0},  //结算日冻结 管理区
    { MonthFreeze                   , 0x01,   sizeof(struct log_manager_t),0},  //月冻结 管理区 
    { SeasonTabCutFreeze            , 0x01,   sizeof(struct log_manager_t),0},  //时区表切换冻结 管理区
    { DayTabCutFreeze               , 0x01,   sizeof(struct log_manager_t),0},  //日时段表切换冻结 管理区
    { FeePriceCutFreeze             , 0x01,   sizeof(struct log_manager_t),0},  //费率电价切换冻结 管理区
    { StepCutFreeze                 , 0x01,   sizeof(struct log_manager_t),0},  //阶梯切换冻结 管理区
    { StepBillFreeze                , 0x01,   sizeof(struct log_manager_t),0},  //阶梯结算冻结 管理区
    { InstantFreeze                 , 0x01,   sizeof(struct log_manager_t),0},  //瞬时冻结 管理区
                                                         
    //事件管理区
    { OverCurrentAS                  , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { PowerOffS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { MeterWrS                       , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { MeterClrS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { EventClrS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SetTimeS                       , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { DayTabWrS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SeasonTabWrS                   , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { WeekTabWrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { BillDayWrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { CoverOpenS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SwitchOffS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SwitchOnS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { HolidayWrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { ActCombWrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { FeeTabWrS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { StepTabWrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SecretKeyWrS                   , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { ErrPlugCardS                   , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { RechargeS                      , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { RefundS                        , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { SwitchErrS                     , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { PowerAbnormalS                 , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { RTCErrS                        , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { EmuErrS                        , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { BroadcastTimeS                 , 0x01,   sizeof(struct log_manager_t),0},// 管理区
    { ZeroLineErrS                   , 0x01,   sizeof(struct log_manager_t),0},// 管理区


    //事件关联对象
    { OverCurrentAS_oad             , 0x01,  61, init_OverCurrentAS_oad   }, //oad列表 1+ 4*15     默认8个   电能表A相过流事件
    { PowerOffS_oad                 , 0x01,  61, init_PowerOffS_oad       }, //oad列表 1+ 4*15     默认8个   电能表掉电事件            
    { MeterWrS_oad                  , 0x01,  61, init_MeterWrS_oad        }, //oad列表 1+ 4*15     默认8个   电能表编程事件            
    { MeterClrS_oad                 , 0x01,  61, init_MeterClrS_oad       }, //oad列表 1+ 4*15     默认8个   电能表清零事件            
    { EventClrS_oad                 , 0x01,  61, init_EventClrS_oad       }, //oad列表 1+ 4*15     默认8个   电能表事件清零事件        
    { SetTimeS_oad                  , 0x01,  61, init_SetTimeS_oad        }, //oad列表 1+ 4*15     默认8个   电能表校时事件            
    { DayTabWrS_oad                 , 0x01,  61, init_DayTabWrS_oad       }, //oad列表 1+ 4*15     默认8个   电能表时段表编程事件      
    { SeasonTabWrS_oad              , 0x01,  61, init_SeasonTabWrS_oad    }, //oad列表 1+ 4*15     默认8个   电能表时区表编程事件      
    { WeekTabWrS_oad                , 0x01,  61, init_WeekTabWrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表周休日编程事件      
    { BillDayWrS_oad                , 0x01,  61, init_BillDayWrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表结算日编程事件  
    { CoverOpenS_oad                , 0x01,  61, init_CoverOpenS_oad      }, //oad列表 1+ 4*15     默认8个   电能表开盖事件            
    { SwitchOffS_oad                , 0x01,  61, init_SwitchOffS_oad      }, //oad列表 1+ 4*15     默认8个   电能表跳闸事件            
    { SwitchOnS_oad                 , 0x01,  61, init_SwitchOnS_oad       }, //oad列表 1+ 4*15     默认8个   电能表合闸事件            
    { HolidayWrS_oad                , 0x01,  61, init_HolidayWrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表节假日编程事件      
    { ActCombWrS_oad                , 0x01,  61, init_ActCombWrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表有功组合方式编程事件
    { FeeTabWrS_oad                 , 0x01,  61, init_FeeTabWrS_oad       }, //oad列表 1+ 4*15     默认8个   电能表费率参数表编程事件  
    { StepTabWrS_oad                , 0x01,  61, init_StepTabWrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表阶梯表编程事件      
    { SecretKeyWrS_oad              , 0x01,  61, init_SecretKeyWrS_oad    }, //oad列表 1+ 4*15     默认8个   电能表密钥更新事件 
    { ErrPlugCardS_oad              , 0x01,  61, init_ErrPlugCardS_oad    }, //oad列表 1+ 4*15     默认8个   电能表异常插卡事件        
    { RechargeS_oad                 , 0x01,  61, init_RechargeS_oad       }, //oad列表 1+ 4*15     默认8个   电能表购电记录            
    { RefundS_oad                   , 0x01,  61, init_RefundS_oad         }, //oad列表 1+ 4*15     默认8个   电能表退费记录  
    { SwitchErrS_oad                , 0x01,  61, init_SwitchErrS_oad      }, //oad列表 1+ 4*15     默认8个   电能表负荷开关误动作事件  
    { PowerAbnormalS_oad            , 0x01,  61, init_PowerAbnormalS_oad  }, //oad列表 1+ 4*15     默认8个   电能表电源异常事件 
    { RTCErrS_oad                   , 0x01,  61, init_RTCErrS_oad         }, //oad列表 1+ 4*15     默认8个   电能表时钟故障事件 
    { EmuErrS_oad                   , 0x01,  61, init_EmuErrS_oad         }, //oad列表 1+ 4*15     默认8个   电能表计量芯片故障事件
    { BroadcastTimeS_oad            , 0x01,  61, init_BroadcastTimeS_oad  }, //oad列表 1+ 4*15     默认8个   广播校时事件 
    { ZeroLineErrS_oad              , 0x01,  61, init_ZeroLineErrS_oad    }, //oad列表 1+ 4*15     默认8个   零线电流异常事件 
     

    //冻结关联对象
    { MinuteFreeze_oad              , 0x01,  61, init_MinuteFreeze_oad       }, //oad列表 1+ 4*15     默认7个
    { HourFreeze_oad                , 0x01,  61, init_HourFreeze_oad         }, //oad列表 1+ 4*15     
    { DayFreeze_oad                 , 0x01,  61, init_DayFreeze_oad          }, //oad列表 1+ 4*15     
    { BillFreeze_oad                , 0x01,  61, init_BillFreeze_oad         }, //oad列表 1+ 4*15   
    { MonthFreeze_oad               , 0x01,  61, init_MonthFreeze_oad        }, //oad列表 1+ 4*15     
    { SeasonTabCutFreeze_oad        , 0x01,  61, init_SeasonTabCutFreeze_oad }, //oad列表 1+ 4*15   
    { DayTabCutFreeze_oad           , 0x01,  61, init_DayTabCutFreeze_oad    }, //oad列表 1+ 4*15   
    { FeePriceCutFreeze_oad         , 0x01,  61, init_FeePriceCutFreeze_oad  }, //oad列表 1+ 4*15   
    { StepCutFreeze_oad             , 0x01,  61, init_StepCutFreeze_oad      }, //oad列表 1+ 4*15   
    { StepBillFreeze_oad            , 0x01,  61, init_StepBillFreeze_oad     }, //oad列表 1+ 4*15   
    { InstantFreeze_oad             , 0x01,  61, init_InstantFreeze_oad      }, //oad列表 1+ 4*15   

    //上报标识
    { OverCurrentAS_RptFlag         , 0x01,   0x01,  init_Event_ReportFlag_1 },// 上报标识   电能表A相过流事件
    { PowerOffS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_3 },// 上报标识   电能表掉电事件            
    { MeterWrS_RptFlag              , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表编程事件            
    { MeterClrS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表清零事件            
    { EventClrS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表事件清零事件        
    { SetTimeS_RptFlag              , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表校时事件            
    { DayTabWrS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表时段表编程事件      
    { SeasonTabWrS_RptFlag          , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表时区表编程事件      
    { WeekTabWrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表周休日编程事件      
    { BillDayWrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表结算日编程事件  
    { CoverOpenS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_1 },// 上报标识   电能表开盖事件            
    { SwitchOffS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表跳闸事件            
    { SwitchOnS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表合闸事件            
    { HolidayWrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表节假日编程事件      
    { ActCombWrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表有功组合方式编程事件
    { FeeTabWrS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表费率参数表编程事件  
    { StepTabWrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表阶梯表编程事件      
    { SecretKeyWrS_RptFlag          , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表密钥更新事件 
    { ErrPlugCardS_RptFlag          , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表异常插卡事件        
    { RechargeS_RptFlag             , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表购电记录            
    { RefundS_RptFlag               , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表退费记录  
    { SwitchErrS_RptFlag            , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表负荷开关误动作事件  
    { PowerAbnormalS_RptFlag        , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表电源异常事件 
    { RTCErrS_RptFlag               , 0x01,   0x01,  init_Event_ReportFlag_1 },// 上报标识   电能表时钟故障事件 
    { EmuErrS_RptFlag               , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   电能表计量芯片故障事件
    { BroadcastTimeS_RptFlag        , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   广播校时事件 
    { ZeroLineErrS_RptFlag          , 0x01,   0x01,  init_Event_ReportFlag_0 },// 上报标识   零线电流异常事件 

     //上报方式
    { OverCurrentAS_RptMode         , 0x01,   0x01,  init_Event_ReportMode_0 },// 上报方式   电能表A相过流事件
    { PowerOffS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_0 },// 上报方式   电能表掉电事件            
    { MeterWrS_RptMode              , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表编程事件            
    { MeterClrS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表清零事件            
    { EventClrS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表事件清零事件        
    { SetTimeS_RptMode              , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表校时事件            
    { DayTabWrS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表时段表编程事件      
    { SeasonTabWrS_RptMode          , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表时区表编程事件      
    { WeekTabWrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表周休日编程事件      
    { BillDayWrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表结算日编程事件  
    { CoverOpenS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_0 },// 上报方式   电能表开盖事件            
    { SwitchOffS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表跳闸事件            
    { SwitchOnS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表合闸事件            
    { HolidayWrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表节假日编程事件      
    { ActCombWrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表有功组合方式编程事件
    { FeeTabWrS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表费率参数表编程事件  
    { StepTabWrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表阶梯表编程事件      
    { SecretKeyWrS_RptMode          , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表密钥更新事件 
    { ErrPlugCardS_RptMode          , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表异常插卡事件        
    { RechargeS_RptMode             , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表购电记录            
    { RefundS_RptMode               , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表退费记录  
    { SwitchErrS_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表负荷开关误动作事件  
    { PowerAbnormalS_RptMode        , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表电源异常事件 
    { RTCErrS_RptMode               , 0x01,   0x01,  init_Event_ReportMode_0 },// 上报方式   电能表时钟故障事件 
    { EmuErrS_RptMode               , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   电能表计量芯片故障事件
    { BroadcastTimeS_RptMode        , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   广播校时事件 
    { ZeroLineErrS_RptMode          , 0x01,   0x01,  init_Event_ReportMode_1 },// 上报方式   零线电流异常事件 
    { Follow2015_RptMode            , 0x01,   0x01,  init_Event_ReportMode_1 }, //上报方式   2015跟随上报状态字  
        
    //事件记录序号
    { OverCurrentAS_seq             , 0x01,  4,     0  },//记录序号  电能表A相过流
    { PowerOffS_seq                 , 0x01,  4,     0  },//记录序号  电能表掉电事件            
    { MeterWrS_seq                  , 0x01,  4,     0  },//记录序号  电能表编程事件            
    { MeterClrS_seq                 , 0x01,  4,     0  },//记录序号  电能表清零事件            
    { EventClrS_seq                 , 0x01,  4,     0  },//记录序号  电能表事件清零事件        
    { SetTimeS_seq                  , 0x01,  4,     0  },//记录序号  电能表校时事件            
    { DayTabWrS_seq                 , 0x01,  4,     0  },//记录序号  电能表时段表编程事件      
    { SeasonTabWrS_seq              , 0x01,  4,     0  },//记录序号  电能表时区表编程事件      
    { WeekTabWrS_seq                , 0x01,  4,     0  },//记录序号  电能表周休日编程事件      
    { BillDayWrS_seq                , 0x01,  4,     0  },//记录序号  电能表结算日编程事件  
    { CoverOpenS_seq                , 0x01,  4,     0  },//记录序号  电能表开盖事件            
    { SwitchOffS_seq                , 0x01,  4,     0  },//记录序号  电能表跳闸事件            
    { SwitchOnS_seq                 , 0x01,  4,     0  },//记录序号  电能表合闸事件            
    { HolidayWrS_seq                , 0x01,  4,     0  },//记录序号  电能表节假日编程事件      
    { ActCombWrS_seq                , 0x01,  4,     0  },//记录序号  电能表有功组合方式编程事件
    { FeeTabWrS_seq                 , 0x01,  4,     0  },//记录序号  电能表费率参数表编程事件  
    { StepTabWrS_seq                , 0x01,  4,     0  },//记录序号  电能表阶梯表编程事件      
    { SecretKeyWrS_seq              , 0x01,  4,     0  },//记录序号  电能表密钥更新事件 
    { ErrPlugCardS_seq              , 0x01,  4,     0  },//记录序号  电能表异常插卡事件        
    { RechargeS_seq                 , 0x01,  4,     0  },//记录序号  电能表购电记录            
    { RefundS_seq                   , 0x01,  4,     0  },//记录序号  电能表退费记录  
    { SwitchErrS_seq                , 0x01,  4,     0  },//记录序号  电能表负荷开关误动作事件  
    { PowerAbnormalS_seq            , 0x01,  4,     0  },//记录序号  电能表电源异常事件 
    { RTCErrS_seq                   , 0x01,  4,     0  },//记录序号  电能表时钟故障事件 
    { EmuErrS_seq                   , 0x01,  4,     0  },//记录序号  电能表计量芯片故障事件
    { BroadcastTimeS_seq            , 0x01,  4,     0  },//记录序号  广播校时事件 
    { ZeroLineErrS_seq              , 0x01,  4,     0  },//记录序号  零线电流异常事件 
       
     //冻结记录序号                                                                 
    { MinuteFreeze1_seq             , 0x01,  4,0   },//分钟冻结记录方案1 记录序号
    { MinuteFreeze2_seq             , 0x01,  4,0   },//分钟冻结记录方案2 记录序号
    { MinuteFreeze3_seq             , 0x01,  4,0   },//分钟冻结记录方案3 记录序号
    { MinuteFreeze4_seq             , 0x01,  4,0   },//分钟冻结记录方案4 记录序号
    { MinuteFreeze5_seq             , 0x01,  4,0   },//分钟冻结记录方案5 记录序号
    { MinuteFreeze6_seq             , 0x01,  4,0   },//分钟冻结记录方案6 记录序号
    { MinuteFreeze7_seq             , 0x01,  4,0   },//分钟冻结记录方案7 记录序号
    { MinuteFreeze8_seq             , 0x01,  4,0   },//分钟冻结记录方案8 记录序号
    { HourFreeze_seq                , 0x01,  4,0   },//小时冻结  记录序号
    { DayFreeze_seq                 , 0x01,  4,0   },//日冻结    记录序号
    { BillFreeze_seq                , 0x01,  4,0   },//结算日冻结      记录序号
    { MonthFreeze_seq               , 0x01,  4,0   },//月冻结    记录序号
    { SeasonTabCutFreeze_seq        , 0x01,  4,0   },//时区表切换冻结冻结    记录序号
    { DayTabCutFreeze_seq           , 0x01,  4,0   },//日时段表切换冻结冻结    记录序号
    { FeePriceCutFreeze_seq         , 0x01,  4,0   },//费率电价切换冻结冻结    记录序号
    { StepCutFreeze_seq             , 0x01,  4,0   },//阶梯切换冻结冻结    记录序号
    { StepBillFreeze_seq            , 0x01,  4,0   },//阶梯结算冻结冻结    记录序号
    { InstantFreeze_seq             , 0x01,  4,0   },//瞬时冻结冻结    记录序号
                                             
    //冻结周期                       
    { MinuteFreeze_period           , 0x01, 24, init_MinuteFreeze_period       }, //分钟冻结 周期   3*8  
    { HourFreeze_period             , 0x01,  3, init_HourFreeze_period         }, //小时冻结冻结 周期   3 
    { DayFreeze_period              , 0x01,  3, init_DayFreeze_period          }, //日冻结冻结 周期   3 
    { BillFreeze_period             , 0x01,  3, init_BillFreeze_period         }, //结算日冻结冻结 周期   3 
    { MonthFreeze_period            , 0x01,  3, init_MonthFreeze_period        }, //月冻结冻结 周期   3 
    { SeasonTabCutFreeze_period     , 0x01,  3, init_SeasonTabCutFreeze_period }, //时区表切换冻结冻结 周期   3 
    { DayTabCutFreeze_period        , 0x01,  3, init_DayTabCutFreeze_period    }, //日时段表切换冻结冻结 周期   3 
    { FeePriceCutFreeze_period      , 0x01,  3, init_FeePriceCutFreeze_period  }, //费率电价切换冻结冻结 周期   3 
    { StepCutFreeze_period          , 0x01,  3, init_StepCutFreeze_period      }, //阶梯切换冻结冻结 周期   3 
    { StepBillFreeze_period         , 0x01,  3, init_StepBillFreeze_period     }, //阶梯结算冻结冻结 周期   3 
    { InstantFreeze_period          , 0x01,  3, init_InstantFreeze_period      }, //瞬时冻结冻结 周期   3 
                                     
    //冻结深度                    
    { MinuteFreeze_depth            , 0x01, 30, init_MinuteFreeze_depth       }, //分钟冻结 深度     2*15  
    { HourFreeze_depth              , 0x01,  2, init_HourFreeze_depth         }, //分钟冻结 深度     2   
    { DayFreeze_depth               , 0x01,  2, init_DayFreeze_depth          }, //分钟冻结 深度     2   
    { BillFreeze_depth              , 0x01,  2, init_BillFreeze_depth         }, //分钟冻结 深度     2   
    { MonthFreeze_depth             , 0x01,  2, init_MonthFreeze_depth        }, //分钟冻结 深度     2   
    { SeasonTabCutFreeze_depth      , 0x01,  2, init_SeasonTabCutFreeze_depth }, //分钟冻结 深度     2   
    { DayTabCutFreeze_depth         , 0x01,  2, init_DayTabCutFreeze_depth    }, //分钟冻结 深度     2   
    { FeePriceCutFreeze_depth       , 0x01,  2, init_FeePriceCutFreeze_depth  }, //分钟冻结 深度     2   
    { StepCutFreeze_depth           , 0x01,  2, init_StepCutFreeze_depth      }, //分钟冻结 深度     2   
    { StepBillFreeze_depth          , 0x01,  2, init_StepBillFreeze_depth     }, //分钟冻结 深度     2   
    { InstantFreeze_depth           , 0x01,  2, init_InstantFreeze_depth      }, //分钟冻结 深度     2   

    //记录数据中间缓存
    { EventClr_List                 , 0x01,  7, 0 }, //事件清零列表  array OMD   事件清零事件记录用 
    { SwitchErr_State               , 0x01, 10, 0 }, //负荷开关误动作状态040A0130160?16001600 
    { BeforSetT_Time                , 0x01,  7, 0 }, //校时前时间 秒 分 时 日 月 年(2字节)
    { AfterSetT_Time                , 0x01,  7, 0 }, //校时后时间 秒 分 时 日 月 年(2字节)
    { MeterWr_List                  , 0x01, 52, 0 }, //编程对象列表  array OAD    
    { BeforSet_SeasonT              , 0x01,114, init_BeforSet_SeasonT }, //编程前备用套时区  array num 14*8  
    { Set_DayTab_Oad                , 0x01,  5, 0 }, //编程时段表对象          
    { BeforSet_DayTab               , 0x01,114, 0 }, //编程前备用套时段表  array num 14*8  
    { BeforSet_WeekTab              , 0x01,  2, 0 }, //编程前周休日时段表号   
    { Set_Holiday_Oad               , 0x01,  5, 0 }, //编程时节假日对象         
    { BeforSet_Holiday              , 0x01, 10, 0 }, //编程前节假日
    { BeforSet_Balance              , 0x01, 20, 0 }, //编程前结算日
    { BeforSet_ActCom               , 0x01,  3, 0 }, //编程前有功组合特征字
    { LastBillingTime               , 0x01,  5, 0 }, //最近一次结算时间 YYMMDDhhmm   
    { LastDayFreezTime              , 0x01,  5, 0 }, //最近一次日冻结时间 YYMMDDhhmm           
    { LastBroadcastTimeERR          , 0x01,  3, 0 }, //最近一次广播校时引起时钟故障 年月日       

   
        //待调整
    { Report3320_EnableList         , 0x01, 161, 0 }, // 3320 上报使能列表  1 + 2*80         
    { Report3320_OADList1           , 0x01, 241, 0 }, // 3320 oad上报列表   1 + 4*60     //红外
    { Report3320_OADList2           , 0x01, 241, 0 }, // 3320 oad上报列表   1 + 4*60     //rs485-1
    { Report3320_OADList3           , 0x01, 241, 0 }, // 3320 oad上报列表   1 + 4*60     //PLC  
    { Report3320_OADList4           , 0x01, 241, 0 }, // 3320 oad上报列表   1 + 4*60     //rs485-2
    { ActReport_Info                , 0x01,   4, 0 }, // 主动上报 停电下缓存信息  2 + 1 + 1   主动上报标志（2） + 3320上报次数（1）+2015上报次数（1） 
        

    { MinFrez_Sectors               , 0x01, 18, 0 }, //分钟冻结各方案的扇区偏移 9*2 

    { SecurityMode_En               , 0x01,  1, init_SecurityMode_En }, //安全模式参数使能0-dis   1-en
    { SecurityMode_EXData           , 0x01,101, init_SecurityMode_EXData }, //显式安全模式参数缓存
    
    { AutoDisplayList,     0x64,   2,       init_AutoDisplayList },//100屏*2字节
    { KeyDisplayList,      0x64,   2,       init_KeyDisplayList },//100屏*2字节
    
    //4XXX
//    { Comm_Number,         0x01,   6,       init_Comm_Number },//表地址
    { Meter_Number,        0x01,   6,       init_Meter_Number },//表号
    { User_Number,         0x01,   6,       init_User_Number },//客户编号
    { Meter_Num,           0x01,   1,       init_Meter_Num },//计量元件数
    { SeasonTable_Changetime,   0x01,   7,  init_SeasonTable_Changetime },//两套时区切换时间
    { DayTable_Changetime,      0x01,   7,  init_SeasonTable_Changetime },//两套时段切换时间
    { SeasonDay_Para,      0x05,   1,       init_SeasonDay_Para },//时区时段参数（年时区数、日时段表数、日时段数、费率数、公共假日数）
    { SecretKey_Num,       0x01,   1,       init_SecretKey_Num },//秘钥总条数
    { HoliDay_Table,       0x14,   6,       init_HoliDay_Table },//公共假日表，20张，每张6字节
    { Weekend,             0x01,   1,       init_Weekend },//周休日特征字
    { WeekendFee,          0x01,   1,       init_WeekendFee },//周休日采用费率号
    { Season_Table0,       0x0E,   3,       init_Season_Table0 },//当前套时区表，≤14
    { Season_Table1,       0x0E,   3,       init_Season_Table0 },//备用套时区表，≤14
    { Day_Table00,         0x08,   42,      init_Day_Table00 },//当前套日时段表数据
    { Day_Table10,         0x08,   42,      init_Day_Table00 },//备用套日时段表数据
    { Meter_CT,            0x01,   4,       init_Meter_CT },//CT
    { Meter_PT,            0x01,   4,       init_Meter_PT },//PT
    { Pulse_Width,         0x01,   1,       init_Pulse_Width},//脉冲宽度
    { ASCII_AssetsMan,     0x01,   32,      init_ASCII_AssetsMan},//资产管理编码
    { ASCII_Un,            0x01,   6,       init_ASCII_Un },//额定电压
    { ASCII_Imin,          0x01,   6,       init_ASCII_Imin },  //最小电流
    { ASCII_Itr,           0x01,   6,       init_ASCII_Itr },   //转折电流
    { ASCII_Ib,            0x01,   6,       init_ASCII_Ib },//额定电流
    { ASCII_Imax,          0x01,   6,       init_ASCII_Imax },//最大电流
    { ASCII_AccuracyClass1,0x01,   4,       init_ASCII_AccuracyClass1 },//有功准确度等级
    { Meter_Const1,        0x01,   4,       init_Meter_Const1 },//有功脉冲常数
    { ASCII_MeterType,     0x01,   32,      init_ASCII_MeterType },//电能表型号
    { MeterRunWord,        0x01,   1,       init_MeterRunWord },//电能表运行特征字
    { SoftWareNo,          0x01,   16,      init_SoftWareNo },//软件备案号
    { Measure_Mode,        0x01,   1,       init_Measure_Mode },//有功组合特征字
    { Balance_Time,        0x03,   2,       init_Balance_Time },//结算日
    { Meter_Password,      0x01,   8,       init_Meter_Password },//应用连接密码
    { Meter_Location1,     0x01,   10,      init_Meter_Location1},//设备地理位置经度
    { Meter_Location2,     0x01,   10,      init_Meter_Location2},//设备地理位置纬度
    { Meter_Location3,     0x01,   5,       init_Meter_Location3},//设备地理位置高度
    
    { Device_descriptor,   0x01,   32,      init_Device_descriptor},//设备描述符
    
    { Factory_No,          0x01,    4,      init_Factory_No }, //厂家代码
    { Version_SoftWare,    0x01,    4,      init_Version_SoftWare }, //软件版本
    { Date_SoftWare,       0x01,    6,      init_Date_SoftWare }, //软件版本日期
    { Version_HardWare,    0x01,    4,      init_Version_HardWare }, //硬件版本
    { Date_HardWare,       0x01,    6,      init_Date_HardWare }, //硬件版本日期
    { Extend_Info,         0x01,    8,      init_Extend_Info }, //扩展信息
    
    { Produced_Date,       0x01,    7,      init_Produced_Date }, //生产日期
    { Subset_List,         0x01,    1,      init_Subset_List }, //子设备列表(num+2*oi)
    { Protocol_List,       0x01,   76,      init_Protocol_List }, //协议列表
    { FollowReport_en,     0x01,    1,      init_FollowReport_en }, //允许跟随上报
    { ActiveReport_en,     0x01,    1,      init_ActiveReport_en }, //允许主动上报
    { Comm_Master_en,      0x01,    1,      init_Comm_Master_en }, //允许与主站通讯
    { Report_channel,      0x01,    13,     init_Report_channel }, //上报通道
    
    { ImaxProtectValue,    0x01,    4,      init_ImaxProtectValue }, //继电器拉闸电流门限值
    { ImaxProtectTime,     0x01,    2,      init_ImaxProtectTime }, //超电流门限保护延时时间
    { IrAuthenTime,        0x01,    4,      init_IrAuthenTime }, //红外认证时效
    { AuthMeth,            0x01,    1,      0}, //连接认证机制
    
    
    
    /***************** 计量相关 *****************/
    { Heat_Value_10L,      15,      2,      0 },    //1.0L自热补偿
    { Heat_Value_05L,      15,      2,      0 },    //0.5L自热补偿

    { Un_Cmp_Value,        0x01,    8,      0 },   //80%Un 90%Un 110%Un 115%Un补偿
    { ADJ_Pgain0,          0x01,    8,      0 },    //火线校表参数（功率增益、功率偏移、电流偏移、相位补偿）4*2=8
    { ADJ_Pgain1,          0x01,    8,      0 },    //零线校表参数（功率增益、功率偏移、电流偏移、相位补偿）4*2=8
    { ADJ_Other,           0x01,    4,      0 },    //计量其他参数（启动阀值、HFconst）1 + 2 = 3
    { ADJ_K,               0x01,    16,     0 },    //Kp Ku Ki 零线K系数 4*4 = 16
#else
    {0}
#endif
};



int16_t supply_parameter_info(struct instance_t* instance, uint16_t id, uint32_t* addr, uint16_t* size,uint16_t *offset)
{
    uint16_t i;

    POINTER_EMPTY(addr);
    *addr = 0x00;

    for (i = 0; i < sizeof(parameter_item) / sizeof(parameter_item[0]); i++)
    {        
        if ((parameter_item[i].paramerer_index + (parameter_item[i].same - 0x01) >= id) && 
            (parameter_item[i].paramerer_index <= id))
        {
            POINTER_EMPTY(size);
            *size = parameter_item[i].same * parameter_item[i].len + 0x02;
            
            POINTER_EMPTY(offset);
            *offset = (id - parameter_item[i].paramerer_index) * parameter_item[i].len;

            return 1;
        }
        *addr += parameter_item[i].len * parameter_item[i].same + 0x02;  //加两个字节的校验码；
    }
    *addr = 0x00;
    return -1;
}


int16_t supply_event_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i;

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item) / sizeof(record_item[0]); i++)
    {
        
        if ((event_flag == record_item[i].event_flag_id_start) || 
            (event_flag == record_item[i].event_flag_id_end))
        {
            POINTER_EMPTY(size);
            *size = record_item[i].len;

            POINTER_EMPTY(depth); 
            *depth = record_item[i].max_depth;

            return 1;
        }
        *addr += record_item[i].len;
    }
    *addr = 0x00;
    return -1;
}

int16_t supply_event_flash_1_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i;
    uint32_t addr_index;
    uint16_t sector_size[9] = { 0 };
    uint8_t tempbuf[2*8] = { 0 };
    int ret;
    addr_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MinFrez_Sectors);
    ret = nd_get_index(instance, addr_index, (uint8_t*)&sector_size[0], 18);
    if (ret < 0)
    {
        memset(&sector_size[0], 0, sizeof(sector_size));
    }

    addr_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MinuteFreeze_depth);
    ret = nd_get_index(instance, addr_index, (uint8_t*)&tempbuf[0], 16);
    if (ret < 0)
    {
        memset(&tempbuf[0], 0, sizeof(tempbuf));
    }

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item_1) / sizeof(record_item_1[0]); i++)
    {
        *addr = sector_size[i] * 4096;//record_item_1[i].len;
        if ((event_flag == record_item_1[i].event_flag_id_start) ||
            (event_flag == record_item_1[i].event_flag_id_end))
        {
            POINTER_EMPTY(addr);

            //*size = record_item_1[i].len;
            if (sector_size[i + 1] > sector_size[i])
            {
                *size = (sector_size[i + 1] - sector_size[i]) * 4096;
            }
            else
            {
                *size = 0x00;
            }
            
            POINTER_EMPTY(depth);
            //*depth = record_item_1[i].max_depth;
            *depth = buf_to_uint16_msb(&tempbuf[2 * i]);

            return 1;
        }
        
    }
    *addr = 0x00;
    return -1;
}

int16_t supply_event_flash_2_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint16_t i;

    POINTER_EMPTY(addr);
    *addr = 0x00;
    for (i = 0; i < sizeof(record_item_2) / sizeof(record_item_2[0]); i++)
    {
        
        if ((event_flag == record_item_2[i].event_flag_id_start) ||
            (event_flag == record_item_2[i].event_flag_id_end))
        {
            POINTER_EMPTY(addr);
            *size = record_item_2[i].len;

            POINTER_EMPTY(depth);
            *depth = record_item_2[i].max_depth;

            return 1;
        }
        *addr += record_item_2[i].len;
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

//
//const uint32_t record_head_t2[] = {  };

// 事件清零事件记录头   事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态 、事件清零列表
const uint32_t record_head_t3[] = { 6, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200, 0x330C0206 };

// 编程记录事件记录头  事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态 、编程对象列表
const uint32_t record_head_t4[] = { 6, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200, 0x33020206 };

// 时段表编程事件记录头  事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态 、编程时段表对象、编程前时段表
const uint32_t record_head_t5[] = { 7, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200, 0x33140206, 0x33140207 };

// 节假日编程事件记录头  事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态 、编程节假日对象、编程前节假日内容
const uint32_t record_head_t6[] = { 7, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200, 0x33150206, 0x33150207 };



// 通过oad 获取 事件相关的存储地址信息
int get_record_id_info(struct instance_t* instance, uint32_t oad, struct record_storage_id_info_t* info)
{
    
    uint16_t SaveId = 0, SeqId = 0, CaptureId = 0, PeriodId = 0, DepthId = 0, ReportFlagId = 0, ReportModeId = 0;
    uint8_t SaveDevice, SaveBlock;
//    uint8_t schem = get_OAD_Schem(oad);
    uint8_t temp = 0;
    //uint16_t OI;
    uint8_t /*attr = 0, oadclass = 7, */scheme = 0;

    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    memset((uint8_t*)info, 0, sizeof(struct record_storage_id_info_t));
    
    //OI = get_OAD_OI(oad);
    //attr = get_OAD_Attribute(oad);
    scheme = get_OAD_Schem(oad);

    info->record_head = record_head_t0; //默认事件记录单元

    switch (oad&0xF0000000)
    {
        case 0x20000000: 
        {
            switch (oad&0xFFFF0000)// OI
            {
                case 0x20150000: //2015 //只获取上报方式
                    ReportModeId = Follow2015_RptMode;   //上报方式   
                    break;
                default:
                    return -1;
                    break;
            }
        }
            break;
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
                    ReportFlagId = OverCurrentAS_RptFlag;   //上报标识
                    ReportModeId = OverCurrentAS_RptMode;   //上报方式   
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
                    ReportFlagId = PowerOffS_RptFlag;   //上报标识
                    ReportModeId = PowerOffS_RptMode;   //上报方式   
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->PowerOffS_acc_time;
                }
                break;
                case 0x30120000: //编程事件
                {
                    SaveId = MeterWrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = MeterWrS_oad;          // 关联对象存储地址信息
                    SeqId = MeterWrS_seq;              // 序号存储地址信息
                    ReportFlagId = MeterWrS_RptFlag;   //上报标识
                    ReportModeId = MeterWrS_RptMode;   //上报方式  
                    info->record_head = record_head_t4;     // 记录单元
                }
                break;
                case 0x30130000:
                {
                    SaveId = MeterClrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = MeterClrS_oad;          // 关联对象存储地址信息
                    SeqId = MeterClrS_seq;              // 序号存储地址信息
                    ReportFlagId = MeterClrS_RptFlag;   //上报标识
                    ReportModeId = MeterClrS_RptMode;   //上报方式  
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;

                case 0x301A0000:
                case 0x30190000:
                case 0x30180000:
                case 0x30160000:
                //case 0x30150000://事件清零
                {
                    temp = (((oad & 0xFFFF0000) - 0x30150000) >> 16);
                    SaveId = EventClrS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = EventClrS_oad + temp;          // 关联对象存储地址信息
                    SeqId = EventClrS_seq + temp;              // 序号存储地址信息
                    ReportFlagId = EventClrS_RptFlag + temp;   //上报标识
                    ReportModeId = EventClrS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;
                case 0x30150000://事件清零
                {
                    temp = (((oad & 0xFFFF0000) - 0x30150000) >> 16);
                    SaveId = EventClrS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = EventClrS_oad + temp;          // 关联对象存储地址信息
                    SeqId = EventClrS_seq + temp;              // 序号存储地址信息
                    ReportFlagId = EventClrS_RptFlag + temp;   //上报标识
                    ReportModeId = EventClrS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t3;     // 记录单元
                }
                break;
                case 0x30170000:    //电能表时段表编程事件
                {
                    //temp = (((oad & 0xFFFF0000) - 0x30150000) >> 16);
                    SaveId = DayTabWrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = DayTabWrS_oad;          // 关联对象存储地址信息
                    SeqId = DayTabWrS_seq;              // 序号存储地址信息
                    ReportFlagId = DayTabWrS_RptFlag + temp;   //上报标识
                    ReportModeId = DayTabWrS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t5;     // 记录单元
                }
                break;
                case 0x301B0000:
                {
                    SaveId = CoverOpenS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = CoverOpenS_oad;          // 关联对象存储地址信息
                    SeqId = CoverOpenS_seq;              // 序号存储地址信息
                    ReportFlagId = CoverOpenS_RptFlag + temp;   //上报标识
                    ReportModeId = CoverOpenS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t0;     // 记录单元
                    info->acc_time = &mbase_cb->CoverOpenS_acc_time;
                }
                break;


                case 0x30220000:
                case 0x30200000:
                case 0x301F0000:
                {
                    temp = (((oad & 0xFFFF0000) - 0x301F0000) >> 16);
                    SaveId = SwitchOffS + temp;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = SwitchOffS_oad + temp;          // 关联对象存储地址信息
                    SeqId = SwitchOffS_seq + temp;              // 序号存储地址信息
                    ReportFlagId = SwitchOffS_RptFlag + temp;   //上报标识
                    ReportModeId = SwitchOffS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t0;     // 记录单元
                }
                break;

                case 0x30210000: //电能表节假日编程事件   
                {
                    //temp = (((oad & 0xFFFF0000) - 0x301F0000) >> 16);
                    SaveId = HolidayWrS;                 // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = HolidayWrS_oad;          // 关联对象存储地址信息
                    SeqId = HolidayWrS_seq;              // 序号存储地址信息
                    ReportFlagId = HolidayWrS_RptFlag + temp;   //上报标识
                    ReportModeId = HolidayWrS_RptMode + temp;   //上报方式  
                    info->record_head = record_head_t6;         // 记录单元
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
                    ReportFlagId = FeeTabWrS_RptFlag + temp;   //上报标识
                    ReportModeId = FeeTabWrS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = SwitchErrS_RptFlag + temp;   //上报标识
                    ReportModeId = SwitchErrS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = PowerAbnormalS_RptFlag + temp;   //上报标识
                    ReportModeId = PowerAbnormalS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = RTCErrS_RptFlag + temp;   //上报标识
                    ReportModeId = RTCErrS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = EmuErrS_RptFlag + temp;   //上报标识
                    ReportModeId = EmuErrS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = BroadcastTimeS_RptFlag + temp;   //上报标识
                    ReportModeId = BroadcastTimeS_RptMode + temp;   //上报方式  
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
                    ReportFlagId = ZeroLineErrS_RptFlag + temp;   //上报标识
                    ReportModeId = ZeroLineErrS_RptMode + temp;   //上报方式  
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
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = InstantFreeze_oad;             // 关联对象存储地址信息
                    SeqId = InstantFreeze_seq;                 // 序号存储地址信息
                    PeriodId = InstantFreeze_period;           // 周期存储地址信息
                    DepthId = InstantFreeze_depth;             // 深度存储地址信息
                }
                break;
                case 0x50020000://8方案 待调整
                {
                    SaveId = MinuteFreeze1 + scheme;                 // + schem 记录存储地址信息
                    SaveDevice = RES_DEV_FLASH;
                    SaveBlock = mbase_block_event_flash1;
                    CaptureId = MinuteFreeze_oad;           // 关联对象存储地址信息
                    SeqId = MinuteFreeze1_seq + scheme;              // 序号存储地址信息
                    PeriodId = MinuteFreeze_period;         // 周期存储地址信息
                    DepthId = MinuteFreeze_depth;           // 深度存储地址信息
                    info->period_len = 24;
                    info->depth_len = 30;
                }
                break;

                //case 0x50030000://小时冻结
                case 0x50040000://日冻结
                //case 0x50050000://结算日冻结
                //case 0x50060000://月冻结
                //case 0x50070000://年冻结 （不要求，但需占用位置）
                //case 0x50080000://时区表切换冻结
                //case 0x50090000://日时段表切换冻结
                //case 0x500A0000://费率电价切换冻结
                //case 0x500B0000://阶梯切换冻结
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
                
                case 0x50030000://小时冻结
                case 0x50060000://月冻结
                case 0x50080000://时区表切换冻结
                case 0x50090000://日时段表切换冻结
                case 0x500A0000://费率电价切换冻结
                case 0x500B0000://阶梯切换冻结
                {
                    temp = (((oad & 0xFFFF0000) - 0x50030000) >> 16);
                    SaveId = HourFreeze + temp;                    // 记录存储地址信息
                    SaveDevice = RES_DEV_EEPROM;
                    SaveBlock = mbase_block_event;
                    CaptureId = HourFreeze_oad + temp;             // 关联对象存储地址信息
                    SeqId = HourFreeze_seq + temp;                 // 序号存储地址信息
                    PeriodId = HourFreeze_period + temp;           // 周期存储地址信息
                    DepthId = HourFreeze_depth + temp;             // 深度存储地址信息
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

    if (ReportFlagId)   //上报标识
    {
        info->rpt_flag = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ReportFlagId);
    }

    if (ReportModeId) //上报方式  
    {
        info->rpt_mode = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ReportModeId);
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
    uint8_t schemNo = get_OAD_Schem(oad);
    struct record_storage_id_info_t info;
    int ret = get_record_id_info(instance, (oad&0xFFFF1FFF), &info);
    
    if (ret < 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
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
                if((oad&0xFFFF0000) == 0x50020000) 
                {
                    *p = ((*p)|(oad&0x0000E000)); // 分钟冻结 记录头 先加方案号
                }
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
            if (((oad&0xFFFF0000) == 0x50020000) && (schemNo != get_OAD_Schem( buf_to_uint32_msb(tempbuf+1+4*i)))) //分钟冻结特殊处理
            {
                continue;
            }
            mem_swap_cpy(p, (tempbuf+1+4*i), 4);
            p++;
            rcsd_num += 1;
        }
     }  
    *rcsd_list =  rcsd_num;
    
    return 1;
}



// data_time_s 转4字节
int fc_fun_01(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
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

// 编程列表  按解析取数据长度
int fc_fun_02(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0;
    uint8_t *p = des;
    uint8_t num = 0;
    
    if (mode == RFC_WRITE)
    {
        ndv_memcopy(des, src, 52);
        p += 52;
    }
    else if (mode == RFC_READ)
    {
        //decode_len = axdr_get_len(src); //待调整
        num = *(src+1);
        decode_len = 5*num + 2;
        
        if (decode_len > 52)
        {
            decode_len = 0;
        }
        ndv_memcopy(des, src, decode_len);
        p += decode_len;
    }
    return (p - des);
}

// 按解析取数据长度
int fc_fun_03(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0;
    uint8_t *p = des;
    //uint8_t num = 0;
    
    if (mode == RFC_WRITE)
    {
        if (fc->store_len != 0xFFFF)
        {
            ndv_memcopy(des, src, fc->store_len);
            p += fc->store_len;
        }
    }
    else if (mode == RFC_READ)
    {
        decode_len = axdr_get_len(src); //待调整

        if (decode_len > fc->store_len)
        {
            decode_len = 0;
        }
        ndv_memcopy(des, src, decode_len);
        p += decode_len;
    }
    return (p - des);
}

//电压块 支持单个电压读取
int fc_fun_04(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0;
    uint8_t *p = des;
    //uint8_t num = 0;
    uint8_t index;
    
    index = get_OAD_Index(oad_req);

    if (mode == RFC_WRITE)
    {
        src++;//array
        src++;
        src++;
        ndv_memcopy(des, src, 2);
        p += 2;
    }
    else if (mode == RFC_READ)
    {
        if (index == 0)
        {
            *p++ = TYPE_array;
            *p++ = 1;
            
            
            
            
            
            
            
        }
        else if (index == 1)
        {
        
        }
        else
        {
        
        }

        if (decode_len > fc->store_len)
        {
            decode_len = 0;
        }
        ndv_memcopy(des, src, decode_len);
        p += decode_len;
    }
    return (p - des);
}



// 电能数据 存储和协议转换  //待调整
int fc_fun_Energy(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src,uint8_t *des, uint8_t* src, enum format_conversion_mode mode)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t i, /*type,*/ attr, index, index_src,  /*len, */block_num = (4+1);
    uint16_t oi;
    uint8_t *p = des;
    uint8_t energy_tag;
    uint32_t addr;
    int64_t tmp_64 = 0;
    
    //type = get_OAD_Class(oad_req);
    attr = get_OAD_Attribute(oad_req);
    index = get_OAD_Index(oad_req);
    oi = get_OAD_OI(oad_req);


    index_src = get_OAD_Index(oad_src);

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
    nd_get_index(instance, addr, &block_num, 1); 
    
    block_num += 1;   // 总加 费率个数
    if(block_num > 13)  block_num = 13;

    if (mode == RFC_WRITE)
    {
        if(attr == 2)//普通
        {
            if(index)
            {
                src++;
                ndv_memcopy(p, src, 4);
                p += 4;
            }
            else
            {
                src += 2;
                for (i = 0; i < 13; i++)
                {
                    src++;
                    if (i < block_num)
                    {
                        ndv_memcopy(p, src, 4);
                    }
                    else
                    {
                        memset(p, 0, 4);
                    }
                    src += 4;
                    p += 4;
                }
            } 
        }
        else if(attr == 4) // 高精度
        {
            if(index)
            {
                src++;//tag
                src += 3;
                ndv_memcopy(p, src, 5);
                p += 5;
            }
            else
            {
                src += 2;
                for (i = 0; i < 13; i++)
                {
                    src++;//tag
                    src += 3;
                    if (i < block_num)
                    {
                        ndv_memcopy(p, src, 5);
                    }
                    else
                    {
                        memset(p, 0, 5);
                    }
                    src += 5;
                    p += 5;
                }
            }
        }
        else  //不支持
        {
            
        }


    }
    else if (mode == RFC_READ)
    {
        if (((oi&0x0FF0) == 0x0000) || ((oi&0x0FF0) == 0x0030) || ((oi&0x0FF0) == 0x0040))
        {
            energy_tag = TYPE_double_long;
            if(attr == 4) //
            {
               energy_tag = TYPE_long64;
            }
        }
        else
        {
            energy_tag = TYPE_double_long_unsigned;
            if(attr == 4) //
            {
               energy_tag = TYPE_long64_unsigned;
            }
        }
  
        if(attr == 2)//普通
        {
            if(index)
            {
                if (index_src == 0) //取索引偏移
                {
                    src += (index-1)*5;
                }
                
                *p++ = energy_tag;

                tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                if(src[0] & 0x80)
                {
                    tmp_64 += 0xFFFFFF0000000000;
                }
                tmp_64 /= 100;

                p[0] = (uint8_t)(tmp_64 >> (3*8));
                p[1] = (uint8_t)(tmp_64 >> (2*8));
                p[2] = (uint8_t)(tmp_64 >> (1*8));
                p[3] = (uint8_t)(tmp_64 >> (0*8));

                //mem_swap_cpy(p, src, 4);
                p += 4;
                src += 5;
            }
            else
            {
                *p++ = TYPE_array;
                *p++ = block_num;
                for (i = 0; i < block_num; i++)
                {
                    *p++ = energy_tag;

                    tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                    if(src[0] & 0x80)
                    {
                        tmp_64 += 0xFFFFFF0000000000;
                    }
                    tmp_64 /= 100;
                    
                    p[0] = (uint8_t)(tmp_64 >> (3*8));
                    p[1] = (uint8_t)(tmp_64 >> (2*8));
                    p[2] = (uint8_t)(tmp_64 >> (1*8));
                    p[3] = (uint8_t)(tmp_64 >> (0*8));

                    src += 5;
                    p += 4;
                }
            }
        }
        else if(attr == 4) // 高精度
        {
            if(index)
            {
                if (index_src == 0) //取索引偏移
                {
                    src += (index-1)*5;
                }
                
                *p++ = energy_tag;
                tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                
                if(src[0] & 0x80)
                {
                    tmp_64 += 0xFFFFFF0000000000;
                }

                p[0] = (uint8_t)(tmp_64 >> (7*8));
                p[1] = (uint8_t)(tmp_64 >> (6*8));
                p[2] = (uint8_t)(tmp_64 >> (5*8));
                p[3] = (uint8_t)(tmp_64 >> (4*8));
                p[4] = (uint8_t)(tmp_64 >> (3*8));
                p[5] = (uint8_t)(tmp_64 >> (2*8));
                p[6] = (uint8_t)(tmp_64 >> (1*8));
                p[7] = (uint8_t)(tmp_64 >> (0*8));

                src += 5;
                p += 8;
            }
            else
            {
                *p++ = TYPE_array;
                *p++ = block_num;
                for (i = 0; i < block_num; i++)
                {
                    *p++ = energy_tag;
                    tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                    
                    if(src[0] & 0x80)
                    {
                        tmp_64 += 0xFFFFFF0000000000;
                    }
                    
                    p[0] = (uint8_t)(tmp_64 >> (7*8));
                    p[1] = (uint8_t)(tmp_64 >> (6*8));
                    p[2] = (uint8_t)(tmp_64 >> (5*8));
                    p[3] = (uint8_t)(tmp_64 >> (4*8));
                    p[4] = (uint8_t)(tmp_64 >> (3*8));
                    p[5] = (uint8_t)(tmp_64 >> (2*8));
                    p[6] = (uint8_t)(tmp_64 >> (1*8));
                    p[7] = (uint8_t)(tmp_64 >> (0*8));

                    src += 5;
                    p += 8;
                }
            }
        }
        else  //不支持
        {
            
        }

    }
    return (p - des);
}



// record存储格式特殊处理的 oad
const struct format_conversion_t format_conversion_tab[] =
{
    {   0x201E0200,     8,      5,      fc_fun_01   },
    {   0x20200200,     8,      5,      fc_fun_01   },
    {   0x20210200,     8,      5,      fc_fun_01   },
    {   0x40000200,     8,      5,      fc_fun_01   },
    
    //{   0x20000200,     5,      2,      fc_fun_04   },//电压
    //{   0x20010200,     7,      4,      fc_fun_01   },//电流
    //{   0x20040200,     12,     4,      fc_fun_01   },//功率  
    //{   0x200A0200,     8,      2,      fc_fun_01   },//功率因数
   
    {   0x33020206,   0xFFFF,  52,      fc_fun_02   },  
    {   0x33140207,   0xFFFF,  114,     fc_fun_03   },  
    {   0x40152200,   0xFFFF,  114,     fc_fun_03   },  
    {   0x41162200,   0xFFFF,  20,      fc_fun_03   },  
    
};

// record存储格式特殊处理的 oad 电能



//enum format_conversion_mode // record_change_format
//
//    RFC_WRITE,           //存储写入转换
//    RFC_READ,            //协议读出转换
//    RFC_GET_ORIGINAL_LEN,//获取原协议格式len
//    RFC_GET_STORE_LEN,   //获取存储格式len
//
int record_change_format(struct instance_t* instance, uint8_t *des,  uint8_t* src, uint32_t oad_req, uint32_t oad_src, enum format_conversion_mode mode)
{
    uint16_t i;
    uint8_t type, attr, index, len, block_num = (4+1);
    uint32_t addr;

    type = get_OAD_Class(oad_req);
    attr = get_OAD_Attribute(oad_req);
    index = get_OAD_Index(oad_req);
    switch(type)
    {
        case 0: //电能
        {
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
            nd_get_index(instance, addr, &block_num, 1); 
            
            block_num += 1;   // 总加 费率个数
            if (mode == RFC_GET_ORIGINAL_LEN) 
            {
                if(attr == 2)//普通
                {
                    if(index)
                    {
                        len = (4+1); //待调整
                    }
                    else
                    {
                        len = (4+1)*block_num + 2; //待调整
                    }
                }
                else if(attr == 4) // 高精度
                {
                    if(index)
                    {
                        len = (8+1);//待调整
                    }
                    else
                    {
                        len = (8+1)*block_num + 2;//待调整
                    }
                }
                else  //不支持
                {
                    len = 0;
                }
                return len;
            }
            else if (mode == RFC_GET_STORE_LEN)
            {
                if(attr == 2)//普通
                {
                    if(index)
                    {
                        len = (5);//待调整
                    }
                    else
                    {
                        len = (5)*13;//待调整
                    }
                }
                else if(attr == 4) // 高精度
                {
                    if(index)
                    {
                        len = (5);//待调整
                    }
                    else
                    {
                        len = (5)*13;//待调整
                    }
                }
                else  //不支持
                {
                    len = 0;
                }
                return len;
            }
            else
            {
                return fc_fun_Energy(instance, oad_req, oad_src, des, src, mode);
            }
        }
            break;
        default:
        {
            for (i = 0; i < (sizeof(format_conversion_tab) / sizeof(format_conversion_tab[0])); i++)
            {
                if ((format_conversion_tab[i].oad & (0x0000F000)) != 0 ) //指定方案  全字比对
                {
                    if ((oad_req ) == format_conversion_tab[i].oad)
                    {
                        break;
                    }
                }
                else
                {
                    if ((oad_req & (0xFFFF1FFF)) == format_conversion_tab[i].oad)
                    {
                        break;
                    }
                }

            }
            
            if (i >= (sizeof(format_conversion_tab) / sizeof(format_conversion_tab[0])))
            {
                return -1;
            }
            
            if (mode == RFC_GET_ORIGINAL_LEN)
            {
                if(format_conversion_tab[i].original_len == 0xFFFF) 
                {
                    return axdr_get_len(src); // 待调整  目前解析长度 只有数据域 不带tag
                }
                else
                {
                    return format_conversion_tab[i].original_len;
                }
                
            }
            else if (mode == RFC_GET_STORE_LEN)
            {
                return format_conversion_tab[i].store_len;
            }
            else
            {
                if(format_conversion_tab[i].format_fun)
                {
                    return format_conversion_tab[i].format_fun(instance, oad_req, oad_src, des, src, mode, (struct format_conversion_t *)&format_conversion_tab[i]);
                }
            }

        }
        break;
    }
    return 1;
}


int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx, uint16_t mode)
{
    uint8_t OIA1 = get_OAD_OIA1(id);
    uint8_t h_mode = (uint8_t) (mode >> 8);
    uint8_t l_mode = (uint8_t) mode;
    struct context_698_t* pctx = (struct context_698_t*)ctx;
    
    switch (l_mode)
	{
    case RESOURCE_MAP_GET:
        switch (OIA1)
        {
        case 0x0://电量
            return GET_0_Energy(instance, id, buf, h_mode, ctx);
            break;
        case 0x1://需量
            return GET_1_Demand(instance, id, buf, ctx);
            break;
        case 0x2://变量
            return GET_2_Variable(instance, id, buf, h_mode, ctx);
            break;
        case 0x3://事件
            return GET_3_Event(instance, id, buf, ctx);
            break;
        case 0x4://参变量
            return GET_4_Parameter(instance, id, buf, ctx);
            break;
        case 0x5://冻结
            return GET_5_Frozen(instance, id, buf, ctx);
            break;
        case 0x6://采集
            return GET_6_Collection(instance, id, buf, ctx);
            break;
        case 0x7://集合
            return GET_7_Aggregate(instance, id, buf, ctx);
            break;
        case 0x8://控制
            return GET_8_Control(instance, id, buf, ctx);
            break;
        case 0xe://监控
            return GET_e_Monitor(instance, id, buf, ctx);
            break;
        case 0xf://其他
            return GET_f_Other(instance, id, buf, ctx);
            break;
        default:
            return -DAR_OBJECT_NOT_EXIST;
            break;
        }
        break;
    case RESOURCE_MAP_SET:
      	switch (OIA1)
        {
        case 0x2:// 变量
            return SET_2_Variable(instance, id, buf, ctx);
        case 0x3:// 事件
            return SET_3_Event(instance, id, buf, ctx);
        case 0x4://参变量
            return SET_4_Parameter(instance, id, buf, ctx);
        case 0x5:// 事件
            return SET_5_Frozen(instance, id, buf, ctx);
        case 0x8:// 控制类
            return SET_8_Control(instance, id, buf, ctx);
        case 0xf://其他
            return SET_f_Other(instance, id, buf, ctx);
        default:
            return -DAR_OBJECT_NOT_EXIST;
            break;
        }
        break;
    case RESOURCE_MAP_ACTION:
        if((1 != pctx->timeTag_valid) && (pctx->timeTag[0] == 1)) return -DAR_TIME_TAG_INVALID;
        switch (OIA1)
        {
        case 0x2://变量
            return ACTION_2_Variable(instance, id, buf, ctx);
        case 0x3://事件
            return ACTION_3_Event(instance, id, buf, ctx);
        case 0x4://参变量
            return ACTION_4_Parameter(instance, id, buf, ctx);
        case 0x5://冻结
            return ACTION_5_Frozen(instance, id, buf, ctx);
        case 0x8://控制类
            return ACTION_8_Parameter(instance, id, buf, ctx);
        case 0xf://其他
            return ACTION_F_Other(instance, id, buf, buf_size, ctx);
        default:
            return -DAR_OBJECT_NOT_EXIST;
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
    index = (oad & 0x000000FF);
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
    uint32_t oad;
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
int para_init(struct instance_t* instance )
{
    uint16_t i,num=0,s_len,j,sm=0;
    uint32_t s_index=0;
    struct mbase_control_block_t* mbase_cb;
    int ret;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    num = sizeof(parameter_item)/sizeof(parameter_item_t);
    
    v85xxp_clear_watchdog();
    
    //RAM部分更新
    mbase_cb->lcd_poweron_all_dis_time = 5;            //上电全显时间
    mbase_cb->backlight_key_time = 60;                  //背光点亮时间
    mbase_cb->lcd_checkdis_keep_sec = 10;               //显示查看背光点亮时间
    mbase_cb->lcd_nopower_key_keep_sec = 30;            //无电按键驻留时间
    mbase_cb->energy_point = 2;                        //电能小数显示位
    mbase_cb->power_point = 4;                         //功率小数显示位
    mbase_cb->lcd_12_mean = 0;                         //液晶12字样意义
    mbase_cb->lcd_autodis_list_num = 5;                           //轮显屏数
    mbase_cb->lcd_autodis_list_maxnum = 99;                       //最大轮显屏数
    mbase_cb->lcd_keydis_list_num = 23;                           //键显屏数
    mbase_cb->lcd_keydis_list_maxnum = 99;      //键显最大屏数
    mbase_cb->lcd_autodis_keep_sec = 5;                           /*轮显持续时间 */
    mbase_cb->lcd_keydis_keep_sec = 60;                           /*按键轮显持续时间 */
    mbase_cb->Baut_rs485 = 6;
    mbase_cb->Baut_plc = 6;
    mbase_cb->SafeEn = 1;       //安全模式启用
    mbase_cb->relay_ctl_mode = 0; //脉冲式控制方式
    mbase_cb->AuthOn = 0;       //身份认证启用默认不启用
    mbase_cb->Limit_T_Max = 300;  //广播校时最大允许时间
    mbase_cb->Limit_T_Min = 60;   //广播校时最小允许时间
    
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, backlight_key_time));
    s_len = offsetof(struct mbase_control_block_t, OverCurrent_threshold) - offsetof(struct mbase_control_block_t, backlight_key_time);
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, s_len);
    
    memset(mbase_cb->tsa, 0x11, 6);//表地址
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, tsa));
    nd_set_offset(instance, s_index, mbase_cb->tsa, 6);
    

    mbase_cb->OverCurrent_threshold = 720000;         //参数 电能表过流事件-电流触发下限 
    mbase_cb->ZeroLineErrS_rate_threshold = 5000;   // 电能表零线电流异常事件 限值
    mbase_cb->ZeroLineErrS_Current_threshold = 10000;// 电能表零线电流异常事件-电流触发下限
    mbase_cb->OverCurrent_duration_para = 60;  //参数 判定延时时间  
    mbase_cb->PowerOffS_duration_para = 1;    //参数 判定延时时间  
    mbase_cb->CoverOpenS_duration_para = 3;   //参数 判定延时时间  
    mbase_cb->SwitchErrS_duration_para = 5;   //参数 判定延时时间  
    mbase_cb->EmuErrS_duration_para = 5;      //参数 判定延时时间  
    mbase_cb->RTCErrS_duration_para = 1;      //参数 判定延时时间  
    mbase_cb->ZeroLineErrS_duration_para = 60; //参数 判定延时时间      
       
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, OverCurrent_threshold));
    s_len = offsetof(struct mbase_control_block_t, ZeroLineErrS_duration_para) - offsetof(struct mbase_control_block_t, OverCurrent_threshold);
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->OverCurrent_threshold, s_len);  
     
    v85xxp_clear_watchdog();
    //EE部分更新
    for(i=0; i<num; i++)
    {
        if( parameter_item[i].data != 0 )
        {
#if 0     // test        
            if (parameter_item[i].paramerer_index == Follow2015_RptMode)
            {
                v85xxp_clear_watchdog();
            }
#endif            
            if (((parameter_item[i].paramerer_index >= MinuteFreeze_oad) && (parameter_item[i].paramerer_index <= ZeroLineErrS_seq))
                || ((parameter_item[i].paramerer_index >= __INITFLAG2_Start) && (parameter_item[i].paramerer_index <= __INITFLAG2_End)))
            {

                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + parameter_item[i].paramerer_index);
                nd_set_index(instance, s_index, (uint8_t*)(parameter_item[i].data), parameter_item[i].len); 
            }
            else
            {
                if((parameter_item[i].paramerer_index == Day_Table00) || (parameter_item[i].paramerer_index == Day_Table10))
                {
                    sm = parameter_item[i].same;
                }
                else
                {
                    sm = 1;
                }
                for(j=0; j<sm; j++)
                {
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + parameter_item[i].paramerer_index + j);
                    nd_set_index(instance, s_index, (uint8_t*)(parameter_item[i].data+1), (*parameter_item[i].data)); 
                }
            }
        }
    }
    
    ret = freez_size_sectors_init(instance, 0x50020200);//分钟冻结 空间分配
    if(ret < 0)
    {
        return -1;
    }
    
    return 0;
}



//编程记录 判断  协议set和 action 调用
//set (OAD)
//action   类7  24 的 方法 4/5(OMD)；    类9 的 方法 4/5/7/8(OMD);    类21 的方法4（OAD）/7(OMD = 0xF1000700)/9（OMD = 0xF1000900）
void fr_698_MeterWr_program_judge(struct instance_t* instance, struct pipeline_t* pipeline,  struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cilent_index = 0;
    uint32_t id = mesg->id;
    uint8_t tag = TYPE_OAD;
    uint16_t oi;
    uint8_t attribute, result = 0;
   
    if ((ctx->service == 7) && (ctx->service == 6))
    {
        return;
    }

    oi = (unsigned short)((id & 0xFFFF0000) >> 16);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8); // FF

    if (ctx->service == 7) //action
    {
        tag = TYPE_OMD;
        if (((oi & 0xF000) == 0x3000) && ((attribute == 4) || (attribute == 5)) 
         || ((oi & 0xF000) == 0x5000) && ((attribute == 4) || (attribute == 5) || (attribute == 7) || (attribute == 8))
         || (oi == 0xF100) && ((attribute == 4) || (attribute == 7) || (attribute == 9))
         || (oi == 0xF101) && ((attribute == 127) || (attribute == 128) || (attribute == 129)))
        {
            if ((oi == 0xF100))
            {
                if (attribute == 4)
                {
                    tag = TYPE_OAD;  // 填解密后具体oad
                    //id =    ;   // 待调整
                }
            }

            result = 1;
        }
    }
    else if (ctx->service == 6) //set
    {
        result = 1;
    }

    
    if (1 == result)
    {
        if (CA_TYPE_TER(ctx->CA))//终端
        {
            cilent_index = 1;
        }
        else
        {
            cilent_index = 0;
        }

        if (true == fr_698_current_connect_status_get(instance, pipeline, cilent_index))
        {
            MeterWr_obj_list_add(instance, pipeline, mesg, id, tag);
        }
    }
    
}


// 协议相关 、应用链接相关----------------start

//更新（重新计数）应用连接超时时间
void update_connect_timeout_cnt(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t cilent_index = 0;

    if (CA_TYPE_TER(ctx->CA))//from 终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }


    if (mbase_cb->connect_status[ctx->channel][cilent_index] == true)
    {
        mbase_cb->connect_timeout_cnt[ctx->channel][cilent_index] = 0;
    }
}

// 698协议 秒任务
void fr_dl698_sec_task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cilent_index, channel;

    if(mbase_cb->irauthen_tmr > 0)              mbase_cb->irauthen_tmr--;
    if(mbase_cb->authen_tmr > 0)                mbase_cb->authen_tmr--;
    if (mbase_cb->infrared_auth_timeout > 0)    mbase_cb->infrared_auth_timeout--;
    if (mbase_cb->term_auth_timeout > 0)        mbase_cb->term_auth_timeout--;
    if (mbase_cb->poweron_active_report_dly < 10)     mbase_cb->poweron_active_report_dly++;


    for(cilent_index=0; cilent_index<2; cilent_index++)
    {
        //待调整
#if 0
        if ((s->block_timeout[cilent_index] > 0) && ((req->block[cilent_index].data != 0xffff) || (res->block[cilent_index].data != 0xffff)))
        {
            para = dl698_cfg_para_get(channel);
            tick = hfp_get_tick_diff(s->block_timeout[cilent_index]);
    
            if (tick > ((req->do_then_get_timeout * 1000ul) + para->block_timeout))
            {
                aprint("block timeout\r\n");
                sprint("warning: block timer is timeout\r\n");
                s->block_timeout[cilent_index] = 0;
                s->block_error_count[cilent_index] = 0;
                req->block[cilent_index].data = 0xffff;
                res->block[cilent_index].data = 0xffff;
            }
        }
#endif
        for(channel=0; channel<4; channel++)
        {
            if (mbase_cb->connect_status[channel][cilent_index])
            {
                if (mbase_cb->esam_connect_timeout[channel][cilent_index] > 0)
                {
                    mbase_cb->esam_connect_timeout[channel][cilent_index]--;
                }

                mbase_cb->connect_timeout_cnt[channel][cilent_index]++;

                if ((mbase_cb->connect_timeout_cnt[channel][cilent_index] >  mbase_cb->connect_timeout[channel][cilent_index]) || (mbase_cb->esam_connect_timeout[channel][cilent_index] == 0))
                {
                    //待调整    释放链接通知
                    //return release_notification(channel, s,cilent_index);
                    fr_handle_disconnect2(instance,  pipeline, cilent_index,  channel);
                    
                }
            }

        }
    }
    
}

// 698协议相关的认证、上报等数据上电初始化
void fr_dl698_power_on(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);


    mbase_cb->infrared_auth_timeout = 0;
    mbase_cb->term_auth_timeout = 0;
    mbase_cb->poweron_active_report_dly = 0;   
    mbase_cb->active_report_retry_dly[R_3320] = 0;  //R_3011_END
    mbase_cb->active_report_retry_dly[R_2015] = 0;
    mbase_cb->active_report_retry_dly[R_3011_END] = 0;
    mbase_cb->active_report_retry_dly[R_3011_START] = 0;

    mbase_cb->active_report_retry_cnt[R_3011_END] = 0;
    mbase_cb->active_report_retry_cnt[R_3011_START] = 0;

    //mbase_cb->active_report_retry_cnt[R_3320] = 0; //从 e2p中获取      待调整
    //mbase_cb->active_report_retry_cnt[R_2015] = 0;

    mbase_cb->report_piid = 0;
    

#if 0 // 待调整
    //report_poweron_second = 0;
    //report_time_second = 0;
    //report_timeout_count_3011= 0;
    //report_time_second_3011= 0;
    //report_count_3011_start = 0;
    reort_piid = 0;
    current_connect_channel[0] = 0;        
    current_connect_channel[1] = 0;






    uint16_t active_report_CMDtype;         //主动上报 命令类别     需掉电保存   //待调整

    uint8_t active_report_retry_dly[4];        //主动上报 重试等待时间 //待调整
    uint8_t active_report_retry_cnt[4];        //主动上报 重试次数 //待调整
    uint8_t report_piid;                       //主动上报帧的PIID缓存 挪到协议  //待调整
    uint8_t report_random_data[12];            //主动上报 随机数 缓存 //待调整

    //跟随上报2015状态字
    uint32_t report_2015state[4];           //跟随上报状态字  chl_NUM



    
#endif 

    fr_handle_disconnect_all_channel(instance,  pipeline, 0);
    fr_handle_disconnect_all_channel(instance,  pipeline, 1);


    EventOut_698_check(instance);  
}


// 应用链接相关----------------end


