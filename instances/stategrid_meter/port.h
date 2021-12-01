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

#define I_common_extension

#define I_common_fields         I_over_fields \
                                I_common_extension

#define I_data_fields           I_common_fields\
                                Ia_data_fields\
                                Ib_data_fields\
                                Ic_data_fields\

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
#define    OverCurrentL1             99  // A相电能表过流事件 
#define    OverCurrentL2             100 // B相电能表过流事件 
#define    OverCurrentL3             101 // C相电能表过流事件 
#define    OverCurrentEndL1          102 // A相电能表过流事件结束 
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
  
//冻结     需连续
#define    MinuteFreeze1                 170     //分钟冻结记录方案1
#define    MinuteFreeze2                 171     //分钟冻结记录方案2
#define    MinuteFreeze3                 172     //分钟冻结记录方案3
#define    MinuteFreeze4                 173     //分钟冻结记录方案4
#define    MinuteFreeze5                 174     //分钟冻结记录方案5
#define    MinuteFreeze6                 175     //分钟冻结记录方案6
#define    MinuteFreeze7                 176     //分钟冻结记录方案7
#define    MinuteFreeze8                 177     //分钟冻结记录方案8
#define    HourFreeze                    178     //小时冻结
//-----------------------------------------------------------------------


//存储EE块号
        //-----关联对象列表 存储EE块号
#define    OverCurrentL1_oad            200  // A相电能表过流事件 

#define    MinuteFreeze_oad             201     // 分钟冻结关联对象


//-----记录序号
#define    OverCurrentL1_index          230  // A相电能表过流事件 记录序号
#define    MinuteFreeze_period          231  //分钟冻结 周期   3*8   每种方案3个字节，第一个字节为0，表示该周期值无效，非零值表示该周期值有效，后面2个字节表示周期值
#define    MinuteFreeze_depth           232  //分钟冻结 深度   2*8 
#define    HourFreeze_period            233  //小时冻结 周期   3
#define    HourFreeze_depth             234  //小时冻结 深度   2

#define         MAX_FEE_NO                 12                                  

#endif

