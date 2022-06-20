#ifndef __INSTANCE_PORT_H__
#define __INSTANCE_PORT_H__
//!!!移植时的两种做法之设想!!!
//1. 静态移植  
//删减功能时的步骤  a.去掉下面的某些字段(难点在于还得去掉该字段对应的外设资源)  b. 编译   c. 去掉编译不过的源文件
//添加功能时的步骤  a.在extension中添加字段  b. 增加处理该字段的api源文件   c. 添加该源文件到工程中 d.编译
//2，动态移植
//大量使用链表和环境变量   
#define instance_info           uint32_t            pt;\
                                uint32_t            ct;\

#define U_data_extension1       

#define U_data_extension0       uint32_t  d_time[4];   /**< 日总检测时间 */ \
                                uint16_t  d_ok_rate[4]; /**< 日合格率 */ \
                                uint16_t  d_bad_rate[4];/**< 日超限率 */ \
                                uint32_t  d_up_time[4];/**< 日超上限时间 */ \
                                uint32_t  d_low_time[4];/**< 日超下限时间 */ \
                                uint32_t  m_time[4];/**< 月总检测时间 */ \
                                uint16_t  m_ok_rate[4];/**< 月合格率 */ \
                                uint16_t  m_bad_rate[4];/**< 月超限率 */ \
                                uint32_t  m_up_time[4]; /**< 月超上限时间 */ \
                                uint32_t  m_low_time[4];/**< 月超下限时间 */ \
                                uint16_t  U0;\
                                uint16_t  U1;\
                                uint16_t  U2;\
                                uint16_t  U3;\

#define U_data_fields           uint16_t    Ua;\
                                uint16_t    Ub;\
                                uint16_t    Uc; \
                                U_data_extension0 \
                                U_data_extension1

//过流事件控制块
#define I_over_fields           uint8_t    Ia_over_duration;\
                                int8_t     Ib_over_duration;\
                                int8_t     Ic_over_duration;/*dyn:当前相的过流状态持续时间*/\
                                int32_t    I_over;/*static:电能表过流事件-电流触发下限 */ \
                                uint8_t    I_over_duration;/*static:电能表过流事件-判定延时时间 */\

#define Ia_data_fields          int32_t    Ia;
#define Ib_data_fields          int32_t    Ib;
#define Ic_data_fields          int32_t    Ic;//dyn:c相电流*/
#define In_data_fields          int32_t    In;

#define I_common_extension

#define I_common_fields         I_over_fields \
                                I_common_extension

#define I_data_fields           I_common_fields\
                                Ia_data_fields\
                                Ib_data_fields\
                                Ic_data_fields\
                                In_data_fields\

#define alert_ctrl_extension    uint8_t     alert_status[4];  \
#define local_ctrl_extension    
//报警相关字段  报警继电器  报警灯  背光
#define alert_ctrl_fields       uint32_t    alert_duration;   \
                                uint32_t    alert_relay_ctrl_word;  \
                                uint32_t    backlight_ctrl_word;   \
                                alert_ctrl_extension
//远程费控字段
#define remote_ctrl_fields      uint8_t      reason; 
//本地费控字段
#define local_ctrl_fields       uint8_t      status; /* 当前状态  0正常 1 达到报警门限1 2 达到报警门限2  3 透支 */  \
                                uint32_t     rate_price;  /* 当前费率电价 */   \
                                uint32_t     stair_price; /* 当前阶梯电价 */    \
                                uint32_t     current_price; /* 当前电价 */        \
                                int32_t      threshold1;    /* 账户告警阀值1 */ \
                                int32_t      threshold2;    /* 账户告警阀值2 */  \
                                int32_t      credit_quota;  /* 透支额度 */  \
                                int32_t      hoard_quota;   /* 囤积额度 */   \
                                int32_t      close_quota;   /* 合闸允许额度 */   \
                                uint8_t      stair_mode;     /* 当前运行阶梯模式 0: 月阶梯模式; 1:年阶梯模式 */  \
                                uint8_t      stair_num_running; /* 当前运行阶梯 */ \
                                int32_t      balance_int;  /* XXXXXX.XX 当前余额的整数部分 */   \
                                int32_t      balance_decimal; /* XXXXXX 厘数部分 */  \
//负荷控制相关字段
#define load_ctrl_fields        uint8_t     load_ctrl_status; \
                                remote_ctrl_fields  \
                                local_ctrl_fields 


//os相关的字段  比如信号量 队列等
#define os_fields

//!!!设想!!!
//每个app的control block分区,每个分区在存储介质中存在独立的实际分区，以便支持页对齐地整块读写。分别是
//1.掉电保存区且上电需要读出初始化的区域  如电量数据     persistent段
//2.上电需要初始化但掉电不需要保存的区域  如参数         static段
//3.上电需要清零的区域                  如中间状态数据  cache段 

#define static_start                 uint8_t static_section_start;
#define static_end                   uint8_t static_section_end;

#define dyn_cache_start              uint8_t cache_section_start;
#define dyn_cache_end                uint8_t cache_section_end;

#define dyn_persistent_start         uint8_t persistent_section_start;
#define dyn_persistent_end           uint8_t persistent_section_end;

//event_flag_id  event_chain
#define  event_flags_group_id  0
#define  event_flags_group_size  16
#define  max_event_flag    16*8

#define  event_flags_xxx_start  0
                                    
#define    PowerDown                 9
#define    PowerUp                   10
#define    ClockAdjust1              11
#define    ClockAdjust2              12
#define    ClockInvalid              13
#define    BatteryOk                 14
#define    ReplaceBattery            15
#define    WatchdogError             16
#define    FirmwareReady             17
#define    FirmwareActivated         18
#define    Parameterchanged          19
#define    Securitykeychanged        20
#define    MeterClear                21  //电能表清零事件   
#define    MeterCoverOpened          22  //电能表开盖事件 
#define    MeterCoverClosed          23  //电能表开盖事件结束 
#define    TerminalCoverOpened       24  //电能表开端钮盒事件 
#define    TerminalCoverClosed       25  //电能表开端钮盒事件结束 
#define    SwitchOut                 26  //电能表跳闸事件    
#define    SwitchOn                  27  //电能表合闸事件 
#define    TimingRecord              28  //电能表校时事件  
#define    MeterReadingRecord        29  //电能表结算日编程事件 
#define    WeekdayRecord             30  //电能表周休日编程事件 
#define    DCDetected                31  //电能表电源异常事件      
#define    DCRemoved                 32  //电能表电源异常事件结束      
#define    SeasontablePro            33  //电能表时区表编程事件      
#define    DaytablePro               34  //电能表时段表编程事件     
#define    DemandClear               35  //电能表需量清零事件    
#define    VoltageLostAll            36  //电能表全失压事件      
#define    VoltageLostAllEnd         37  //电能表全失压事件结束    
#define    Lpfallphases              38  //电能表掉电事件   
#define    LpfallphasesEnd           39  //电能表掉电事件结束   
#define    AssistPowerlost           40  //电能表辅助电源掉电事件     
#define    AssistPowerlostEnd        41  //电能表辅助电源掉电事件结束  
#define    EventClear                42  //电能表事件清零事件    
#define    MeterWritePara            43  //电能表编程事件     
#define    ActivePowerBytePro        44  //电能表有功组合方式编程事件   
#define    ReactivePowerBytePro      45  //电能表无功组合方式编程事件   
#define    TariffPricePro            46  //电能表费率参数表编程事件   
#define    StairPricePro             47  //电能表阶梯表编程事件      
#define    SecretkeyPro              48  //电能表密钥更新事件        
#define    PlugCardErr               49  //电能表异常插卡事件        
#define    RechargeRecord            50  //电能表购电记录            
#define    RefundRecord              51  //电能表退费记录    
#define    VQualityAllL              52  //总电压超下限 
#define    VQualityAllH              53  //总电压超上限 
#define    VQualityAllRange          54  //总电压考核范围内 
#define    VQualityL1L               55  //A相电压超下限 
#define    VQualityL1H               56  //A相电压超上限 
#define    VQualityL1Range           57  //A相电压考核范围内 
#define    VQualityL2L               58  //B相电压超下限 
#define    VQualityL2H               59  //B相电压超上限 
#define    VQualityL2Range           60  //B相电压考核范围内 
#define    VQualityL3L               61  //C相电压超下限 
#define    VQualityL3H               62  //C相电压超上限 
#define    VQualityL3Range           63  //C相电压考核范围内 
#define	   HolidayPro                64  //电能表节假日编程事件 
#define	   BroadcastTime             65  //电能表广播校时事件 
#define    RtcError                  66  //时钟故障  
#define    RtcErrorEnd               67  //时钟故障结束 
#define    BroadcastTimeErr          68  //广播校时异常事件 
#define    LostVoltageL1             69  //A相电能表失压事件 
#define    LostVoltageL2             70  //B相电能表失压事件 
#define    LostVoltageL3             71  //C相电能表失压事件 
#define    LostVoltageEndL1          72  //A相电能表失压事件结束 
#define    LostVoltageEndL2          73  //B相电能表失压事件结束 
#define    LostVoltageEndL3          74  // C相电能表失压事件结束
#define    UnderVoltageL1            75  //A相电能表欠压事件 
#define    UnderVoltageL2            76  //B相电能表欠压事件 
#define    UnderVoltageL3            77  //C相电能表欠压事件 
#define    UnderVoltageEndL1         78  //A相电能表欠压事件结束 
#define    UnderVoltageEndL2         79  //B相电能表欠压事件结束 
#define    UnderVoltageEndL3         80  //C相电能表欠压事件结束 
#define    OverVoltageL1             81  //A相电能表过压事件 
#define    OverVoltageL2             82  //B相电能表过压事件 
#define    OverVoltageL3             83  //C相电能表过压事件 
#define    OverVoltageEndL1          84  //A相电能表过压事件结束 
#define    OverVoltageEndL2          85  //B相电能表过压事件结束 
#define    OverVoltageEndL3          86  //C相电能表过压事件结束 
#define    MissVoltageL1             87  //A相电能表断相事件
#define    MissVoltageL2             88  //B相电能表断相事件
#define    MissVoltageL3             89  //C相电能表断相事件
#define    MissVoltageEndL1          90  //A相电能表断相事件结束 
#define    MissVoltageEndL2          91  //B相电能表断相事件结束 
#define    MissVoltageEndL3          92  //C相电能表断相事件结束 
#define    LostCurrentL1             93  //A相电能表失流事件 
#define    LostCurrentL2             94  //B相电能表失流事件 
#define    LostCurrentL3             95  //C相电能表失流事件 
#define    LostCurrentEndL1          96  //A相电能表失流事件结束 
#define    LostCurrentEndL2          97  // B相电能表失流事件结束 
#define    LostCurrentEndL3          98  // C相电能表失流事件结束 
//#define    OverCurrentL1             99  // A相电能表过流事件 
#define    OverCurrentL2             100 // B相电能表过流事件 
#define    OverCurrentL3             101 // C相电能表过流事件 
//#define    OverCurrentEndL1          102 // A相电能表过流事件结束 
#define    OverCurrentEndL2          103 // B相电能表过流事件结束 
#define    OverCurrentEndL3          104 // C相电能表过流事件结束 
#define    MissCurrentL1             105 // A相电能表断流事件 
#define    MissCurrentL2             106 // B相电能表断流事件 
#define    MissCurrentL3             107 // C相电能表断流事件 
#define    MissCurrentEndL1          108 // A相电能表断流事件结束 
#define    MissCurrentEndL2          109 // B相电能表断流事件结束 
#define    MissCurrentEndL3          110 // C相电能表断流事件结束 
#define    PowerReverseL1            111 // A相电能表功率反向事件 
#define    PowerReverseL2            112 // B相电能表功率反向事件 
#define    PowerReverseL3            113 // C相电能表功率反向事件 
#define    PowerReverseEndL1         114 // A相电能表功率反向事件结束 
#define    PowerReverseEndL2         115 // B相电能表功率反向事件结束 
#define    PowerReverseEndL3         116 // C相电能表功率反向事件结束 
#define    OverLoadL1                117 // A相电能表过载事件 
#define    OverLoadL2                118 // B相电能表过载事件 
#define    OverLoadL3                119 // C相电能表过载事件 
#define    OverLoadEndL1             120 // A相电能表过载事件结束 
#define    OverLoadEndL2             121 // B相电能表过载事件结束 
#define    OverLoadEndL3             122 // C相电能表过载事件结束 
#define    LoadUnbalance             123 // 电流不平衡事件    
#define    LoadUnbalanceEnd          124 // 电流不平衡事件结束  
#define    LoadUnbalanceSeverit      125 // 电流严重不平衡事件  
#define    LoadUnbalanceSeveritEnd   126 // 电流严重不平衡事件结束
#define    VoltageUnbalance          127 // 电压不平衡事件    
#define    VoltageUnbalanceEnd       128 // 电压不平衡事件结束  
#define    Powerdirectionchanged     129 // 潮流反向事件   
#define    BadPowerFactor            130 // 功率因数超下限事件  
#define    BadPowerFactorEnd         131 // 功率因数超下限事件结束
#define    PositiveDemandOver        132 // 正向有功需量超限事件   
#define    PositiveDemandOverEnd     133 // 正向有功需量超限事件结束 
#define    ReverseDemandOver         134 // 反向有功需量超限事件  
#define    ReverseDemandOverEnd      135 // 反向有功需量超限事件结束 
#define    Quadrant1DemandOver       136 // 1象限无功需量超限事件  
#define    Quadrant1DemandOverEnd    137 // 1象限无功需量超限事件结束
#define    Quadrant2DemandOver       138 // 2象限无功需量超限事件  
#define    Quadrant2DemandOverEnd    139 // 2象限无功需量超限事件结束
#define    Quadrant3DemandOver       140 // 3象限无功需量超限事件  
#define    Quadrant3DemandOverEnd    141 // 3象限无功需量超限事件结束
#define    Quadrant4DemandOver       142 // 4象限无功需量超限事件  
#define    Quadrant4DemandOverEnd    143 //4象限无功需量超限事件结束
#define    Voltagesequencereversal   144 //电压逆相序事件
#define    VoltagesequencereversalEnd   145 //电压逆相序事件结束 
#define    CurrentReversal              146 //电流逆相序事件 
#define    CurrentReversalEnd           147 //电流逆相序事件结束  
#define    MagneticDisturb              148 //恒定磁场干扰事件  
#define    MagneticDisturbEnd           149 //恒定磁场干扰事件结束   
#define    ReplayError                  150 //负荷开关误动作事件   
#define    ReplayErrorEnd               151 //负荷开关误动作事件结束   
#define    EMUError                     152 //计量芯片记录事件 
#define    EMUErrorEnd                  153 //计量芯片记录事件结束 
#define    PowerReverseL4               154 //合相功率反向事件 
#define    PowerReverseEndL4            155 //合相功率反向事件结束
#define    BadPowerFactorL1             156 //A相功率超下限事件 
#define    BadPowerFactorEndL1          157 //A相功率超下限事件结束
#define    BadPowerFactorL2             158 //B相功率超下限事件 
#define    BadPowerFactorEndL2          159 //B相功率超下限事件结束
#define    BadPowerFactorL3             160 //C相功率超下限事件 
#define    BadPowerFactorEndL3          161 //C相功率超下限事件结束
#define    NeutralCurrentErr            162 //零线电流异常事件 
#define    NeutralCurrentErrEnd         163 //零线电流异常事件结束                                    
//
#define    remote_alarm_flag            164 //远程告警命令true-触发告警
#define    battery1_under2v8            165 //时钟电池欠压
#define    battery2_under4v8            166 //停抄电池欠压
//------------------------事件ID 新定义
enum record_flag_id_t
{
    //顺序间隔不能改变
    //class24
    OverCurrentAS = 11,             //  0x3005    A过流发生 
    OverCurrentAE = 12,             //  0x3005    A过流结束 

    PowerOffS                    ,  //  0x3011    电能表掉电事件                                               
    PowerOffE                    ,  //  0x3011    电能表掉电事件                                               
    MeterWrS                     ,  //  0x3012    电能表编程事件       
    MeterWrR                     ,  //  0x3012    电能表编程事件 特殊 用于更新编程列表   
    MeterClrS                    ,  //  0x3013    电能表清零事件                                                        
    EventClrS                    ,  //  0x3015    电能表事件清零事件                                
    SetTimeS                     ,  //  0x3016    电能表校时事件                                    
    DayTabWrS                    ,  //  0x3017    电能表时段表编程事件                                        
    SeasonTabWrS                 ,  //  0x3018    电能表时区表编程事件                                
    WeekTabWrS                   ,  //  0x3019    电能表周休日编程事件                                              
    BillDayWrS                   ,  //  0x301A    电能表结算日编程事件  
    CoverOpenS                   ,  //  0x301B    电能表开盖事件                                      
    CoverOpenE                   ,  //  0x301B    电能表开盖事件 
    SwitchOffS                   ,  //  0x301F    电能表跳闸事件                                                                           
    SwitchOnS                    ,  //  0x3020    电能表合闸事件                                          
    HolidayWrS                   ,  //  0x3021    电能表节假日编程事件                                                
    ActCombWrS                   ,  //  0x3022    电能表有功组合方式编程事件                                            
    FeeTabWrS                    ,  //  0x3024    电能表费率参数表编程事件                                    
    StepTabWrS                   ,  //  0x3025    电能表阶梯表编程事件                                           
    SecretKeyWrS                 ,  //  0x3026    电能表密钥更新事件 
    ErrPlugCardS                 ,  //  0x3027    电能表异常插卡事件                                           
    RechargeS                    ,  //  0x3028    电能表购电记录                                       
    RefundS                      ,  //  0x3029    电能表退费记录  
    SwitchErrS                   ,  //  0x302B    电能表负荷开关误动作事件                                               
    SwitchErrE                   ,  //  0x302B    电能表负荷开关误动作事件                                            
    PowerAbnormalS               ,  //  0x302C    电能表电源异常事件 
    PowerAbnormalE               ,  //  0x302C    电能表电源异常事件 
    RTCErrS                      ,  //  0x302E    电能表时钟故障事件 
    RTCErrE                      ,  //  0x302E    电能表时钟故障事件 
    EmuErrS                      ,  //  0x302F    电能表计量芯片故障事件
    EmuErrE                      ,  //  0x302F    电能表计量芯片故障事件
    BroadcastTimeS               ,  //  0x303C    广播校时事件 
    ZeroLineErrS                 ,  //  0x3040    零线电流异常事件  
    ZeroLineErrE                 ,  //  0x3040    零线电流异常事件   
};

//冻结     需连续
#define    MinuteFreeze1                170     //分钟冻结记录方案1 
#define    MinuteFreeze2                171     //分钟冻结记录方案2
#define    MinuteFreeze3                172     //分钟冻结记录方案3
#define    MinuteFreeze4                173     //分钟冻结记录方案4
#define    MinuteFreeze5                174     //分钟冻结记录方案5
#define    MinuteFreeze6                175     //分钟冻结记录方案6
#define    MinuteFreeze7                176     //分钟冻结记录方案7
#define    MinuteFreeze8                177     //分钟冻结记录方案8
#define    HourFreeze                   178     //小时冻结
#define    DayFreeze                    179     //日冻结
#define    BillFreeze                   180     //结算日冻结
#define    MonthFreeze                  181     //月冻结
#define    YearFreeze                   182     //年冻结 （不要求，但需占用位置）
#define    SeasonTabCutFreeze           183     //时区表切换冻结
#define    DayTabCutFreeze              184     //日时段表切换冻结
#define    FeePriceCutFreeze            185     //费率电价切换冻结
#define    StepCutFreeze                186     //阶梯切换冻结
#define    StepBillFreeze               187     //阶梯结算冻结
#define    InstantFreeze                188     //瞬时冻结

#define    RECORD_ID_END                200   
//-----------------------------------------------------------------------

//mbase_block_parameter  E2P存储ID------开始
//从201后开始  前200用于冻结或事件记录的存储id，e2p管理区id 
enum block_parameter_item_id_t
{
    //-----关联对象列表 存储EE块号
    //冻结关联对象  冻结相关所有顺序不随意改变
    MinuteFreeze_oad        =        300, // 分钟冻结关联对象             // 初始化特殊处理起始
    HourFreeze_oad          =        301, // 小时冻结关联对象
    DayFreeze_oad           =        302, // 日冻结关联对象
    BillFreeze_oad          =        303, //结算日冻结   
    MonthFreeze_oad         =        304, //月冻结    
    YearFreeze_oad          =        305, //年冻结 （不要求，但需占用位置） 
    SeasonTabCutFreeze_oad  =        306, //时区表切换冻结 
    DayTabCutFreeze_oad     =        307, //日时段表切换冻结 
    FeePriceCutFreeze_oad   =        308, //费率电价切换冻结 
    StepCutFreeze_oad       =        309, //阶梯切换冻结 
    StepBillFreeze_oad      =        310, //阶梯结算冻结 
    InstantFreeze_oad       =        311, //瞬时冻结 
    //冻结周期   顺序不随意改变  
    MinuteFreeze_period     =        313, //分钟冻结 周期   3*8   每种方案3个字节，第一个字节为0，表示该周期值无效，非零值表示该周期值有效，后面2个字节表示周期值
    HourFreeze_period       =        314, //小时冻结 周期   3
    DayFreeze_period        =        315, //日冻结 周期   3
    BillFreeze_period       =        316, //结算日冻结 周期 3
    MonthFreeze_period      =        317, //月冻结 周期   3
    YearFreeze_period       =        318, //年冻结 （不要求，但需占用位置）冻结 周期   3
    SeasonTabCutFreeze_period =      319, //时区表切换冻结 冻结 周期   3
    DayTabCutFreeze_period  =        320, //日时段表切换冻结 冻结 周期   3
    FeePriceCutFreeze_period=        321, //费率电价切换冻结 冻结 周期   3
    StepCutFreeze_period    =        322, //阶梯切换冻结 冻结 周期   3
    StepBillFreeze_period   =        323, //阶梯结算冻结 冻结 周期   3
    InstantFreeze_period    =        324, //瞬时冻结 冻结 周期   3
    //冻结深度    顺序不随意改变
    MinuteFreeze_depth      =        326, //分钟冻结 深度   2*8 
    HourFreeze_depth        =        327, //小时冻结 深度   2
    DayFreeze_depth         =        328, //日冻结 深度   2
    BillFreeze_depth        =        329, //结算日 深度     2
    MonthFreeze_depth       =        330, //月冻结 深度   2
    YearFreeze_depth        =        331, //年冻结 （不要求，但需占用位置）冻结 深度   2
    SeasonTabCutFreeze_depth=        332, //时区表切换冻结冻结 深度   2
    DayTabCutFreeze_depth   =        333, //日时段表切换冻结冻结 深度   2
    FeePriceCutFreeze_depth =        334, //费率电价切换冻结冻结 深度   2
    StepCutFreeze_depth     =        335, //阶梯切换冻结冻结 深度   2
    StepBillFreeze_depth    =        336, //阶梯结算冻结冻结 深度   2
    InstantFreeze_depth     =        337, //瞬时冻结冻结 深度   2
    //冻结记录序号  顺序不随意改变 
    MinuteFreeze1_seq       =        339, //分钟冻结记录方案1 记录序号
    MinuteFreeze2_seq       =        340, //分钟冻结记录方案2 记录序号
    MinuteFreeze3_seq       =        341, //分钟冻结记录方案3 记录序号
    MinuteFreeze4_seq       =        342, //分钟冻结记录方案4 记录序号
    MinuteFreeze5_seq       =        343, //分钟冻结记录方案5 记录序号
    MinuteFreeze6_seq       =        344, //分钟冻结记录方案6 记录序号
    MinuteFreeze7_seq       =        345, //分钟冻结记录方案7 记录序号
    MinuteFreeze8_seq       =        346, //分钟冻结记录方案8 记录序号
    HourFreeze_seq          =        347, //小时冻结  记录序号
    DayFreeze_seq           =        348, //日冻结    记录序号
    BillFreeze_seq          =        349, //结算日冻结    记录序号
    MonthFreeze_seq         =        350, //月冻结    记录序号
    YearFreeze_seq          =        351, //年冻结 冻结    记录序号 （不要求，但需占用位置）
    SeasonTabCutFreeze_seq  =        352, //时区表切换冻结冻结    记录序号
    DayTabCutFreeze_seq     =        353, //日时段表切换冻结冻结    记录序号
    FeePriceCutFreeze_seq   =        354, //费率电价切换冻结冻结    记录序号
    StepCutFreeze_seq       =        355, //阶梯切换冻结冻结    记录序号
    StepBillFreeze_seq      =        356, //阶梯结算冻结冻结    记录序号
    InstantFreeze_seq       =        357, //瞬时冻结冻结    记录序号



    //事件关联对象 顺序不随意改变 
    OverCurrentAS_oad                   ,   //关联对象    电能表A相过流事件
    PowerOffS_oad                       ,   //关联对象    电能表掉电事件            
    MeterWrS_oad                        ,   //关联对象    电能表编程事件            
    MeterClrS_oad                       ,   //关联对象    电能表清零事件            
    EventClrS_oad                       ,   //关联对象    电能表事件清零事件        
    SetTimeS_oad                        ,   //关联对象    电能表校时事件            
    DayTabWrS_oad                       ,   //关联对象    电能表时段表编程事件      
    SeasonTabWrS_oad                    ,   //关联对象    电能表时区表编程事件      
    WeekTabWrS_oad                      ,   //关联对象    电能表周休日编程事件      
    BillDayWrS_oad                      ,   //关联对象    电能表结算日编程事件  
    CoverOpenS_oad                      ,   //关联对象    电能表开盖事件            
    SwitchOffS_oad                      ,   //关联对象    电能表跳闸事件            
    SwitchOnS_oad                       ,   //关联对象    电能表合闸事件            
    HolidayWrS_oad                      ,   //关联对象    电能表节假日编程事件      
    ActCombWrS_oad                      ,   //关联对象    电能表有功组合方式编程事件
    FeeTabWrS_oad                       ,   //关联对象    电能表费率参数表编程事件  
    StepTabWrS_oad                      ,   //关联对象    电能表阶梯表编程事件      
    SecretKeyWrS_oad                    ,   //关联对象    电能表密钥更新事件 
    ErrPlugCardS_oad                    ,   //关联对象    电能表异常插卡事件        
    RechargeS_oad                       ,   //关联对象    电能表购电记录            
    RefundS_oad                         ,   //关联对象    电能表退费记录  
    SwitchErrS_oad                      ,   //关联对象    电能表负荷开关误动作事件  
    PowerAbnormalS_oad                  ,   //关联对象    电能表电源异常事件 
    RTCErrS_oad                         ,   //关联对象    电能表时钟故障事件 
    EmuErrS_oad                         ,   //关联对象    电能表计量芯片故障事件
    BroadcastTimeS_oad                  ,   //关联对象    广播校时事件 
    ZeroLineErrS_oad                    ,   //关联对象    零线电流异常事件 

    //上报标识  顺序不随意改变 
    OverCurrentAS_RptFlag               ,   //上报标识   电能表A相过流事件
    PowerOffS_RptFlag                   ,   //上报标识   电能表掉电事件            
    MeterWrS_RptFlag                    ,   //上报标识   电能表编程事件            
    MeterClrS_RptFlag                   ,   //上报标识   电能表清零事件            
    EventClrS_RptFlag                   ,   //上报标识   电能表事件清零事件        
    SetTimeS_RptFlag                    ,   //上报标识   电能表校时事件            
    DayTabWrS_RptFlag                   ,   //上报标识   电能表时段表编程事件      
    SeasonTabWrS_RptFlag                ,   //上报标识   电能表时区表编程事件      
    WeekTabWrS_RptFlag                  ,   //上报标识   电能表周休日编程事件      
    BillDayWrS_RptFlag                  ,   //上报标识   电能表结算日编程事件  
    CoverOpenS_RptFlag                  ,   //上报标识   电能表开盖事件            
    SwitchOffS_RptFlag                  ,   //上报标识   电能表跳闸事件            
    SwitchOnS_RptFlag                   ,   //上报标识   电能表合闸事件            
    HolidayWrS_RptFlag                  ,   //上报标识   电能表节假日编程事件      
    ActCombWrS_RptFlag                  ,   //上报标识   电能表有功组合方式编程事件
    FeeTabWrS_RptFlag                   ,   //上报标识   电能表费率参数表编程事件  
    StepTabWrS_RptFlag                  ,   //上报标识   电能表阶梯表编程事件      
    SecretKeyWrS_RptFlag                ,   //上报标识   电能表密钥更新事件 
    ErrPlugCardS_RptFlag                ,   //上报标识   电能表异常插卡事件        
    RechargeS_RptFlag                   ,   //上报标识   电能表购电记录            
    RefundS_RptFlag                     ,   //上报标识   电能表退费记录  
    SwitchErrS_RptFlag                  ,   //上报标识   电能表负荷开关误动作事件  
    PowerAbnormalS_RptFlag              ,   //上报标识   电能表电源异常事件 
    RTCErrS_RptFlag                     ,   //上报标识   电能表时钟故障事件 
    EmuErrS_RptFlag                     ,   //上报标识   电能表计量芯片故障事件
    BroadcastTimeS_RptFlag              ,   //上报标识   广播校时事件 
    ZeroLineErrS_RptFlag                ,   //上报标识   零线电流异常事件 

    //上报方式 顺序不随意改变
    OverCurrentAS_RptMode               ,   //上报方式   电能表A相过流事件
    PowerOffS_RptMode                   ,   //上报方式   电能表掉电事件            
    MeterWrS_RptMode                    ,   //上报方式   电能表编程事件            
    MeterClrS_RptMode                   ,   //上报方式   电能表清零事件            
    EventClrS_RptMode                   ,   //上报方式   电能表事件清零事件        
    SetTimeS_RptMode                    ,   //上报方式   电能表校时事件            
    DayTabWrS_RptMode                   ,   //上报方式   电能表时段表编程事件      
    SeasonTabWrS_RptMode                ,   //上报方式   电能表时区表编程事件      
    WeekTabWrS_RptMode                  ,   //上报方式   电能表周休日编程事件      
    BillDayWrS_RptMode                  ,   //上报方式   电能表结算日编程事件  
    CoverOpenS_RptMode                  ,   //上报方式   电能表开盖事件            
    SwitchOffS_RptMode                  ,   //上报方式   电能表跳闸事件            
    SwitchOnS_RptMode                   ,   //上报方式   电能表合闸事件            
    HolidayWrS_RptMode                  ,   //上报方式   电能表节假日编程事件      
    ActCombWrS_RptMode                  ,   //上报方式   电能表有功组合方式编程事件
    FeeTabWrS_RptMode                   ,   //上报方式   电能表费率参数表编程事件  
    StepTabWrS_RptMode                  ,   //上报方式   电能表阶梯表编程事件      
    SecretKeyWrS_RptMode                ,   //上报方式   电能表密钥更新事件 
    ErrPlugCardS_RptMode                ,   //上报方式   电能表异常插卡事件        
    RechargeS_RptMode                   ,   //上报方式   电能表购电记录            
    RefundS_RptMode                     ,   //上报方式   电能表退费记录  
    SwitchErrS_RptMode                  ,   //上报方式   电能表负荷开关误动作事件  
    PowerAbnormalS_RptMode              ,   //上报方式   电能表电源异常事件 
    RTCErrS_RptMode                     ,   //上报方式   电能表时钟故障事件 
    EmuErrS_RptMode                     ,   //上报方式   电能表计量芯片故障事件
    BroadcastTimeS_RptMode              ,   //上报方式   广播校时事件 
    ZeroLineErrS_RptMode                ,   //上报方式   零线电流异常事件 
    //以上不能改变顺序

    Follow2015_RptMode                  ,   //上报方式   2015跟随上报状态字  

    //事件记录序号 顺序不随意改变               
    OverCurrentAS_seq                   ,   //记录序号  电能表A相过流事件
    PowerOffS_seq                       ,   //记录序号  电能表掉电事件            
    MeterWrS_seq                        ,   //记录序号  电能表编程事件            
    MeterClrS_seq                       ,   //记录序号  电能表清零事件            
    EventClrS_seq                       ,   //记录序号  电能表事件清零事件        
    SetTimeS_seq                        ,   //记录序号  电能表校时事件            
    DayTabWrS_seq                       ,   //记录序号  电能表时段表编程事件      
    SeasonTabWrS_seq                    ,   //记录序号  电能表时区表编程事件      
    WeekTabWrS_seq                      ,   //记录序号  电能表周休日编程事件      
    BillDayWrS_seq                      ,   //记录序号  电能表结算日编程事件  
    CoverOpenS_seq                      ,   //记录序号  电能表开盖事件            
    SwitchOffS_seq                      ,   //记录序号  电能表跳闸事件            
    SwitchOnS_seq                       ,   //记录序号  电能表合闸事件            
    HolidayWrS_seq                      ,   //记录序号  电能表节假日编程事件      
    ActCombWrS_seq                      ,   //记录序号  电能表有功组合方式编程事件
    FeeTabWrS_seq                       ,   //记录序号  电能表费率参数表编程事件  
    StepTabWrS_seq                      ,   //记录序号  电能表阶梯表编程事件      
    SecretKeyWrS_seq                    ,   //记录序号  电能表密钥更新事件 
    ErrPlugCardS_seq                    ,   //记录序号  电能表异常插卡事件        
    RechargeS_seq                       ,   //记录序号  电能表购电记录            
    RefundS_seq                         ,   //记录序号  电能表退费记录  
    SwitchErrS_seq                      ,   //记录序号  电能表负荷开关误动作事件  
    PowerAbnormalS_seq                  ,   //记录序号  电能表电源异常事件 
    RTCErrS_seq                         ,   //记录序号  电能表时钟故障事件 
    EmuErrS_seq                         ,   //记录序号  电能表计量芯片故障事件
    BroadcastTimeS_seq                  ,   //记录序号  广播校时事件 
    ZeroLineErrS_seq                    ,   //记录序号  零线电流异常事件              // 初始化特殊处理结束
    //以上不能改变顺序



                         
//    DisplayPara             =        600,  //显示参数    22
    AutoDisplayList         =        700,  //轮显项目700-800
    KeyDisplayList          =        800,  //键显项目800-900
};               



//mbase_block_parameter E2P存储ID------结束                           
                                  
#define     Comm_Number                 900 //表地址
#define     Meter_Number                901 //表号
#define     User_Number                 902 //客户编号
#define     Meter_Num                   903 //计量元件数
#define     SeasonTable_Changetime      904 //两套时区切换时间
#define     DayTable_Changetime         905 //两套时段切换时间
#define     SeasonDay_Para              906 //时区时段参数（年时区数、日时段表数、日时段数、费率数、公共假日数）
#define     SecretKey_Num               911 //秘钥总条数

#define     Weekend                     922 //周休日特征字
#define     WeekendFee                  923 //周休日采用费率号
#define     Season_Table0               924 //当前套时区表，≤14
#define     Season_Table1               938 //备用套时区表，≤14
#define     Day_Table00                 952 //当前套日时段表数据 ≤8
#define     Day_Table10                 960 //备用套日时段表数据 ≤8
#define     Meter_CT                    968 //CT    1
#define     Meter_PT                    969 //PT    1
#define     Pulse_Width                 970 //脉冲宽度  1
#define     ASCII_AssetsMan             971 //资产管理编码    1
#define     ASCII_Un                    972 //额定电压  1
#define     ASCII_Imin                  973 //最新电流
#define     ASCII_Itr                   974 //转折电流
#define     ASCII_Ib                    975 //额定电流  1
#define     ASCII_Imax                  976 //最大电流  1
#define     ASCII_AccuracyClass1        977 //有功准确度等级   1
#define     Meter_Const1                978 //有功脉冲常数    1
#define     ASCII_MeterType             979 //电能表型号 1
#define     MeterRunWord                980 //电能表运行特征字  1
#define     SoftWareNo                  981 //软件备案号 1
#define     Measure_Mode                982 //有功组合特征字   1
#define     Balance_Time                983 //结算日   3
#define     Meter_Password              986 //应用连接密码    1     
                                  
#define     Meter_Location1             987 //设备地理位置经度
#define     Meter_Location2             988 //设备地理位置纬度
#define     Meter_Location3             989 //设备地理位置高度    
                                  
#define     Device_descriptor           990 //设备描述符
#define     Factory_No                  991 //厂家代码
#define     Version_SoftWare            992 //软件版本
#define     Date_SoftWare               993 //软件版本日期
#define     Version_HardWare            994 //硬件版本
#define     Date_HardWare               995 //硬件版本日期
#define     Extend_Info                 996 //扩展信息
#define     Produced_Date               997 //生产日期
#define     Subset_List                 998 //子设备列表
#define     Protocol_List               999 //协议列表
#define     FollowReport_en             1000 //允许跟随上报
#define     ActiveReport_en             1001 //允许主动上报
#define     Comm_Master_en              1002 //允许与主站通讯
#define     Report_channel              1003 //上报通道

#define     ImaxProtectValue            1004 //继电器拉闸电流门限值
#define     ImaxProtectTime             1005 //超电流门限保护延时时间
#define     IrAuthenTime                1006 //红外认证时效
#define     AuthMeth                    1007 //连接认证机制
#define     HoliDay_Table               1008 //公共假日表，20张，每张6字节

/***************** 计量相关 *****************/
#define     Heat_Value_10L              1100    //1.0L自热补偿  15*2=30
#define     Heat_Value_05L              1101    //0.5L自热补偿  15*2=30
#define     Un_Cmp_Value                1102    //电压补偿 80%，90%，110%，115%    4*2=8

#define     ADJ_Pgain0                  1103    //火线校表参数（功率增益、功率偏移、电流偏移、相位补偿）4*2=8
#define 	ADJ_Pgain1                  1104    //零线功率增益（功率增益、功率偏移、电流偏移、相位补偿）4*2=8
#define     ADJ_Other                   1105    //计量其他参数（启动阀值、HFconst）2 + 2 = 4
#define     ADJ_K                       1106    //Ku Kil Kin零线K系数 Kp 4*4 = 16

/*
#define     KeyDisp_Num                 695 //按键显示屏数
#define     KeyDisp_MaxNum              696 //按键显示屏数
#define     KeyDisp_Inv                 697 //按键显示等待时间
#define     AutoDisp_Num                698 //循环显示屏数
#define     AutoDisp_MaxNum             699 //循环显示屏数
#define     AutoDisp_Inv                700 //循环显示等待时间

#define		AllDisp_Tmr                 701 //全显时间
#define     BlkLed_Key_Tmr              702 //按键背光点亮时间
#define     BlkLed_Com_Tmr              703 //液晶查看背光点亮时间
#define     DispOn_Pd_Tmr               704 //无电按键显示驻留时间
#define		Disp_EnergyDot			    705 //电能小数
#define		Disp_PowerDot               706 //功率小数
#define     DispMean12Show              707 //液晶12字符意义
*/

#define     __INITFLAG2_Start           1400 //初始化 特殊处理标记  

#define     MinFrez_Sectors             1400 //分钟冻结各方案的扇区偏移 9*2   // 待调整 不易改和易改的区分开
#define     SecurityMode_En             1401 //安全模式参数使能0-dis   1-en
#define     SecurityMode_EXData         1402 //显式安全模式参数缓存    
#define     EventClr_List               1500 //事件清零列表  array OMD   事件清零事件记录用
#define     SwitchErr_State             1501 //负荷开关误动作状态040A0130160?16001600
#define     BeforSetT_Time              1502 //校时前时间 秒 分 时 日 月 年(2字节)
#define     AfterSetT_Time              1503 //校时后时间 秒 分 时 日 月 年(2字节)
#define     MeterWr_List                1504 //编程对象列表        
#define     BeforSet_SeasonT            1505 //编程前备用套时区表 
#define     Set_DayTab_Oad              1506 //编程时段表对象
#define     BeforSet_DayTab             1507 //编程前备用套时段表
#define     BeforSet_WeekTab            1508 //编程前备用套时段表
#define     Set_Holiday_Oad             1509 //编程时节假日对象
#define     BeforSet_Holiday            1510 //编程前节假日
#define     BeforSet_Balance            1511 //编程前结算日
#define     BeforSet_ActCom             1512 //编程前有功组合特征字
#define     LastBillingTime             1513 //最近一次结算时间 
#define     LastDayFreezTime            1514 //最近一次日冻结时间 
#define     LastBroadcastTimeERR        1515 //最近一次广播校时引起时钟故障 年月日
#define     ResetCount                  1516 //复位次数


#define     Report3320_EnableList       1521 // 3320 上报使能列表  1 + 2*80
#define     Report3320_OADList1         1522 // 3320 oad上报列表   1 + 4*60     //红外
#define     Report3320_OADList2         1523 // 3320 oad上报列表   1 + 4*60     //rs485-1
#define     Report3320_OADList3         1524 // 3320 oad上报列表   1 + 4*60     //PLC  
#define     Report3320_OADList4         1525 // 3320 oad上报列表   1 + 4*60     //rs485-2
#define     ActReport_Info              1526 // 主动上报 停电下缓存信息   //待调整  现是否存两份？

#define     __INITFLAG2_End             ActReport_Info //初始化 特殊处理标记  


#define     MAX_FEE_NO                  12                                  

#endif

