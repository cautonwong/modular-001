#ifndef ND_TARGET_STATEGRID_LEGACY_CONTROL_BLOCK_H
#define ND_TARGET_STATEGRID_LEGACY_CONTROL_BLOCK_H

#include <stdint.h>
#include <time.h>

#include "port.h"

struct energy_ctx_t
{
    uint32_t pa;
};

struct demand_list_t
{
    struct demand_info_t* info;
    uint32_t* import;
    uint32_t output_id;
    uint8_t mode;
    struct demand_list_t* next;
};

struct demand_info_t
{
    uint32_t  cumu_demand;  //当前需量(需量周期到了更新)
    uint32_t  cumu_min;     //每分钟累计 滑差周期到了清0
    uint32_t  cumu_sec;     //每秒钟累计 分钟到了清0
    uint32_t* slide;        //滑差寄存器
    uint32_t  sec_cnt;      //秒计数器，计算每分钟的平均值，每个滑差周期到了清0
    uint8_t   min_cnt;      //分钟计数器，计数每个滑差周期内的平均值
    uint8_t   slide_max;    //滑差寄存器最大值,初始化的时候赋值
    uint8_t   slide_num;    //滑差个数
    uint8_t   sliding;      //滑差周期，单位是分钟
    uint8_t   slide_index;  //滑差寄存器的索引
    uint8_t   slide_cnt;    //滑差寄存器的个数
    uint8_t   fee_cnt;      //费率需量的滑差寄存器的计数器；
    uint8_t   cur_fee;      //当前的费率，这里需要去掉，直接去instance拿，todo
    uint8_t   pre_fee;      //上一个费率
};

#pragma pack(push, 8)
struct mbase_control_block_t
{
    //上电需要发起IO请求的参数区  运行期不更改   //带备份校验
    static_start            //------------------------------------------------------
    uint8_t tsa[6];         //表地址
    uint8_t meterNo[6];     //表号

    uint32_t meter_const;
    uint8_t  active_energy_cal_word;
    
    
    uint8_t lcd_poweron_all_dis_time;  /*上电全显时间 */
    uint16_t backlight_key_time; /*按键背光点亮时间 */
    uint16_t lcd_checkdis_keep_sec;/*显示查看背光点亮时间 */
    uint16_t lcd_nopower_key_keep_sec;/*显示查看显示时间 */
    uint8_t energy_point; //电能显示小数
    uint8_t power_point;    //功率显示小数
    uint8_t lcd_12_mean;    //液晶12字样意义
    
    uint16_t lcd_keydis_keep_sec;    /*按键轮显持续时间 */
    uint8_t lcd_autodis_list_num;    /*轮显屏数 */
    uint8_t lcd_autodis_list_maxnum;    /*轮显最大屏数 */
    uint8_t lcd_keydis_list_num;    /*键显屏数 */
    uint8_t lcd_keydis_list_maxnum;    /*键显最大屏数 */
    uint16_t lcd_autodis_keep_sec;    /*轮显持续时间 */
    
    uint8_t Baut_rs485; //485波特率
    uint8_t Baut_plc;   //载波波特率(EEPROM)

    uint8_t SafeEn; //安全模式  1：启用，0：不启用
    
    uint8_t relay_ctl_mode; //继电器控制方式（0：脉冲式，1：保持式）

    uint16_t Limit_T_Max;   //广播校时最大允许范围
    uint16_t Limit_T_Min;   //广播校时最小允许范围

    //事件参数

    uint32_t OverCurrent_threshold;         //参数 电能表过流事件-电流触发下限   //事件参数起始--------
    
    uint16_t ZeroLineErrS_rate_threshold;   // 电能表零线电流异常事件 限值
    uint32_t ZeroLineErrS_Current_threshold;// 电能表零线电流异常事件-电流触发下限
    uint8_t OverCurrent_duration_para;  //参数 判定延时时间  
    uint8_t PowerOffS_duration_para;    //参数 判定延时时间  //固定0
    uint8_t CoverOpenS_duration_para;   //参数 判定延时时间  
    uint8_t SwitchErrS_duration_para;   //参数 判定延时时间  
    uint8_t EmuErrS_duration_para;      //参数 判定延时时间  
    uint8_t RTCErrS_duration_para;      //参数 判定延时时间  
    
    uint8_t ZeroLineErrS_duration_para; //参数 判定延时时间                             //事件参数末尾------------

    uint32_t report_2015mode;               //跟随上报模式字   
    uint16_t active_report_sec_limit;       //掉电 主动上报 上电时间阈值 默认3600 
    uint8_t report_mode_645[8];             //645主动上报模式字
    uint8_t factor_mode; //工厂模式
    
    uint32_t backlight_control_word;//背光报警配置字 

    uint8_t static_blank_space[4*10];   //  空白区
    
    uint32_t static_chksum; //以上带备份校验
    
    static_end          //------------------------------------------------------
        
    //运行期更改 && 掉电不保存 && 上电不需要IO请求的区域 && 初始化时直接清零
    dyn_cache_start     //------------------------------------------------------   
    I_data_fields   
    U_data_fields
    uint8_t   DayTableNo;//日表序号
    uint8_t   TOUNo;//费率序号
    uint8_t   STEPNo;//阶梯序号
    uint8_t   StepSeason;//阶梯时区表主副表序号
    uint32_t   Price;//当前电价
    uint32_t   TOUPrice;//费率电价
    uint32_t   STEPPrice;//阶梯电价
    uint8_t   StepNum;//阶梯数    // uint8_t stairnum;   // 400D 阶梯数
    uint8_t    Fee;
    uint8_t TouBefNo;
    uint8_t PortTouCut;     // 时段改变需投切
    uint8_t MulPortMode;    //多功能口模式
          
    uint32_t volt_60;   //60%电压
    uint32_t volt_78;   //78%电压
    uint32_t volt_120;  //120%电压
    uint32_t curt_33;  //0.3%电流
    uint32_t power_33;  //启动功率0.33%*P
    
    uint32_t voltage_ac;
    int32_t current_l;
    int32_t active_power;
    int32_t active_avg_power;
    uint32_t apparent_power;
    int32_t current_n;
    uint32_t frequency;
    int32_t power_factor;
    int32_t temperature;
    uint32_t voltage_battery;
    
    uint32_t I_Prot;    //拉闸最大电流限制
    uint16_t Prot_time; //拉闸最大电流限制时间
    
    uint8_t  p_pluse;          
    uint8_t  n_pluse;
    uint8_t  power_dir;
    uint8_t  power_qn;
    
    uint8_t  meter_start_cnt;   //上电计量延时时间3秒    
    
    uint16_t lcd_checkdis_all_keep_sec;/*显示查看全屏显示时间 */
    uint8_t AutoPtr;    //轮显指针
    uint16_t AutoInv;
    uint8_t KeyPtr;
    uint8_t KeyInv;
    uint8_t Key;        //按键标志
    uint16_t BackLightTmr;  //背光显示时间
    uint16_t Page_ID;   //显示ID
    uint8_t under_un68; //所有相电压均低于68%   
    uint8_t auxiliary_power;//辅助电源供电标志
    
    uint8_t disp_borrow;    //显示借位
    
    uint8_t baud_chg;   //波特率改变，BIT0(485),BIT1(PLC)
    uint16_t plc_baut_tick; //1小时不通讯需要复位载波通道，波特率恢复115200
    
    uint8_t Baut_plc_neg;   //协商波特率（RAM）
    uint8_t avg_cal_start_flag; //平均值开始计算标志

    uint8_t clientaddr[4]; //客户机地址
    
//    uint16_t send_frm_size;             //客户机发送帧最大尺寸 long-unsigned
//    uint16_t recv_frm_size;             //客户机接收帧最大尺寸 long-unsigned
//    uint32_t connect_time;              //期望的应用连接超时时间 double-long-unsigned，
//    uint8_t  connect_mode[4];              //认证模式enum connect_mode_e


    // ESAM 部分
    //认证相关
    uint8_t current_connect_channel[2]; //最近一次应用连接通道 0 主站， 1终端
    uint8_t connect_status[4][2];        //应用连接状态 [通道][主站或终端]，   通道顺序 :红外、485-1，载波，485-2；
    uint32_t esam_connect_timeout[4][2]; //ESAM 会话剩余时间[主站或终端]，
    uint32_t connect_timeout[4][2];      // 应用连接超时时间
    uint32_t connect_timeout_cnt[4][2]; // 应用连接超时时间 计数器

    uint32_t term_auth_timeout;//终端身份认证剩余时间
    uint32_t infrared_auth_timeout;//红外身份认证剩余时间 
    uint8_t AuthOn; //身份认证启用标志，1启用
    uint8_t esam_power_status;      // esam电源状态   1: on  0：off
    uint8_t esam_power_close_dly;    //esam电源关闭延时 (秒)
    //主动上报相关  active_report
    uint8_t poweron_active_report_dly;      //主动上报上电延迟时间 (秒)

    
    uint8_t active_report_retry_dly[4];        //主动上报 重试等待时间 

    uint8_t report_piid;                       //主动上报帧的PIID缓存 挪到协议  
    uint8_t report_random_data[12];            //主动上报 随机数 缓存 
    
    uint32_t report_2015state_cache;        //跟随上报状态字缓存


    //645协议相关
    uint8_t rand8_645[8];                       //645随机数缓存 
    uint32_t auth_timeout_645;                  //645身份认证剩余时间   
    uint8_t flag_event_clear;                   //00分相事件清零，0x5a事件总清
    uint8_t report_stat_already_read645;        //645上报状态字已读
    uint8_t report_stat_read_cache645[12];      //645上报状态字

    //显示相关
    uint8_t commdisp_plc;    //通讯符号显示
    uint8_t commdisp_ir485;    //通讯符号显示
    uint8_t lcd_poweron_all_dis_time_tick;//全显倒计时
    
    //运行状态字
    uint16_t run_state[8];
   
    uint8_t relay_true_sta;//继电器实际状态
    

    //事件相关
    uint8_t power_abnormal_tick;    //电压异常延时
    uint8_t power_abnormal_tick1;
    
//    uint8_t  power_abnormal;        //电源异常状态
//    uint8_t  rtc_chip_abnormal;
//    uint8_t  meter_chip_abnormal;
    uint8_t  esam_chip_abnormal;    //esam故障状态



    uint8_t CoverOpenS_status;          //当前状态标志 电能表开盖事件
    uint8_t SwitchErrS_status;          //当前状态标志 电能表负荷开关误动作事件
    uint8_t SwitchErrS_Record_En;       //当前状态标志 负荷开关 误动作 使能记录 标志 
    uint8_t PowerAbnormalS_status;      //当前状态标志 电能表电源异常事件
    uint8_t EmuErrS_status;             //当前状态标志 计量芯片故障事件
    uint8_t RTCErrS_status;             //当前状态标志 时钟故障事件

    uint8_t MeterWr_SetTime_flag;       //当前状态标志 5A: 编程事件由校时产生

    uint8_t OverCurrentAS_duration;     //当前状态持续时间 电能表过流事件    //手动结束时要清零
    uint8_t PowerOffS_duration;         //当前状态持续时间 电能表掉电事件
    uint8_t CoverOpenS_duration;        //当前状态持续时间 电能表开盖事件
    uint8_t SwitchErrS_duration;        //当前状态持续时间 电能表负荷开关误动作事件
    uint8_t PowerAbnormalS_duration;    //当前状态持续时间 电能表电源异常事件 
    uint8_t EmuErrS_duration;           //当前状态持续时间 计量芯片故障事件
    uint8_t RTCErrS_duration;           //当前状态持续时间 时钟故障事件
    uint8_t ZeroLineErrS_duration;      //当前状态持续时间 零线电流异常事件  

    uint8_t pd_cover_EndRecord_flag;   //停电下 开盖已执行过结束标志           ：5A       掉电时清零
    uint8_t pd_active_flag_update;     //停电下 保存事件 active_flag   ：5A


    uint8_t first_MbillDay_flag;

    uint8_t frz_replenish_end;      //补冻结 完成标志  ,写停电时间清零 补冻结完成置 0x5A
    
    uint8_t mac_suspend_state;      // mac挂起状态 协议或显示模块可调用。 true为挂起 
    uint8_t handler_code[4];        // 645操作者代码
    uint8_t RtcBat_UnderV_state;    // 时钟电池欠压状态 0：正常  1：欠压
    uint32_t UpGrade_max_block_count;// 升级已接收过的最大包数
    uint32_t UpGrade_CheckFlag;     // 需要查验升级时间           其他 无需查验
    
    
    uint16_t metertype; //表多功能配置字
    uint8_t no_filt;    //启动值过滤开关
    uint32_t blt_alarm; //背光报警配置字
    
    uint16_t EC_SaveDiff;// 电能定量保存阈值hex       --0.01kwh     

    uint32_t battery_used_time_cnt; //电池使用时间   累加器   sec     停电或复位后清零，低功耗下累加，上电后保存
    uint32_t PowerOffS_time_low_power_cnt;      //停电事件累计时间   累加器     sec        停电或复位后清零，低功耗下累加，上电保存
    uint32_t CoverOpenS_time_low_power_cnt;     //开盖事件累计时间   累加器     sec        停电或复位后清零，低功耗下累加，上电保存
    uint32_t PowerAbnormalS_time_low_power_cnt; //电源异常事件累计时间   累加器     sec        停电或复位后清零，低功耗下累加，上电保存


    uint8_t report_stat_645[108];       //645上报状态字 手动保存e2p

    struct tm v_powerdown_tm;                   // 模拟停电时间  补结算测试用     不存e2p
    dyn_cache_end  //------------------------------------------------------
    //运行期更改&&掉电或定时保存&&上电需要IO请求的数据分区
    dyn_persistent_start
    //电能数据块       排序不应随意改变
    uint32_t active_p[MAX_FEE_NO+1];
    uint32_t active_n[MAX_FEE_NO+1];
    //uint32_t month_active_p[MAX_FEE_NO+1];
    //uint32_t month_active_n[MAX_FEE_NO+1];
    //uint32_t month_active_cs;
    uint16_t active_p_dec[MAX_FEE_NO+1];
    uint16_t active_n_dec[MAX_FEE_NO+1];
    uint32_t Energy_chksum;     //电量 累加和

    uint16_t MonthEnegery_dec; //用电量
    uint32_t MonthEnegery;     //当前月度用电量 
    uint32_t UseEnergy_chksum;     //用电量 累加和

    dyn_persistent_EC_label     // 电能数据 末尾标签

    uint16_t ec_save_success;   // 电能掉电存储成功标志  0x5AA
    
    uint8_t  record_rollback_flag;          //记录回滚标志 0x5A 存在未完成的回滚
    struct tm powerdown_tm;                     // 停电时间

    //RELAY
    uint8_t cmd;	    //控制命令  0x1A , 0x1B....
    uint8_t mode;     //拉合闸方式（直接、允许）
	uint8_t sta;      //命令状态
    uint32_t alarmmin; //跳闸延时(min)     SEC？  没必要存e2p？
    uint32_t limitmin; //限电时间(min)      SEC？  没必要存e2p？ 
    uint32_t imaxmin;  //大电流延时跳闸（min）SEC？  没必要存e2p？
    uint16_t cmd_dostate;  //控制命令执行状态字
    uint16_t cmd_errstate;  //控制命令错误状态字
    uint8_t relayerr_record;    //误动作记录开始
    uint8_t relay_do;   //继电器操作   5A 为补动作。   A5为远程命令
    

    uint8_t key_state;  //秘钥状态，显示调用，true为公钥
    
    uint8_t boardadjtime;   //广播校时标志位
    
    uint8_t switch_on_succees;          //0x5A 表示成功      由2015上报清零
    uint8_t switch_off_succees;         //0x5A 表示成功       由2015上报清零
    
//    uint8_t   SeasonTable;//时区表主副表序号
//    uint8_t   DayTable;//日表主副表序号
    uint8_t   TouPriceTable;//费率电价表主副表序号
//    uint8_t   StepTable;//阶梯表主副表序号
    uint8_t   Disp12;   //
    
    uint16_t  trigger_insfreeze_delay_sec; //触发瞬时冻结 延时时间 sec  FFFF无效

    //事件相关
    //掉电保存
    uint8_t active_flag[50];  //事件或冻结记录存储标志    
    uint8_t record_flag[50];  //事件或冻结记录存储标志 

    uint32_t minute_freeze_seq[8]; //分钟冻结8个序号     
    uint32_t OverCurrentAS_acc_time;    //累计时间 电能表过流事件
    uint32_t PowerOffS_acc_time;        //累计时间 电能表掉电事件
    uint32_t CoverOpenS_acc_time;       //累计时间 电能表开盖事件
    uint32_t SwitchErrS_acc_time;       //累计时间 电能表负荷开关误动作事件
    uint32_t PowerAbnormalS_acc_time;   //累计时间 电能表电源异常事件 
    uint32_t EmuErrS_acc_time;          //累计时间 计量芯片故障事件
    //uint32_t RTCErrS_acc_time;        //累计时间 时钟故障事件
    uint32_t ZeroLineErrS_acc_time;     //累计时间 零线电流异常事件  

    //上报相关
    uint16_t active_report_CMDtype;             //主动上报 命令类别     需掉电保存   
    uint8_t active_report_retry_cnt[4];         //主动上报 重试次数 //R_3320
    uint32_t power_on_sec_accum;                //上电累计时间 sec  上电处理时清零，上电运行累加  
    //跟随上报2015状态字
    uint32_t report_2015state[4];           //跟随上报状态字  chl_NUM   // uint32_to_buf_msb

    uint8_t mac_err_cnt; // 当日mac错误次数
    uint8_t mac_suspend_date[3];// mac错误 挂起日期 yymmdd
    uint8_t password_err_cnt;   //密码锁错误次数
    uint8_t Pass02Err_Cnt;//通讯02级密码错误计数器
	uint8_t Pass04Err_Cnt;//通讯04级密码错误计数器
	uint8_t Pass02Lock_Tmr;//通讯02级密码闭锁时间
	uint8_t Pass04Lock_Tmr;//通讯04级密码闭锁时间
    
    uint8_t meterclear; //电表清零

    uint16_t report_reset_sec_cnt_645;  //645上报状态字复位延时递减计数器
    uint32_t frz_min_start_dlysec;   // 升级过程中  分钟冻结 禁止执行时间   -单相 

    
    uint8_t persistent_blank_space[4*9];   //  空白区

    dyn_persistent_end

    int id;
    //os相关的字段
    os_fields

};
#pragma pack(pop)

#endif
