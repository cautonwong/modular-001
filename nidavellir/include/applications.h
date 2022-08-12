#ifndef __APPLICATIONS_H__
#define __APPLICATIONS_H__

//当前系统中所有的application注册表容器
#include "port.h"
#ifndef WIN32
#include "cpu_surport.h"
#endif // !WIN32


#define RESET_INFO    1 // 记录 复位信息


 

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

struct mbase_control_block_t
{
    //上电需要发起IO请求的参数区  运行期不更改   //带备份校验
    static_start            //------------------------------------------------------
    uint8_t tsa[6];         //表地址
    uint8_t meterNo[6];     //表号

    uint32_t meter_const;
    uint8_t  active_energy_cal_word;
    
    uint32_t backlight_key_time; /*按键背光点亮时间 */
    uint8_t lcd_poweron_all_dis_time;  /*上电全显时间 */
    uint8_t lcd_autodis_list_num;    /*轮显屏数 */
    uint8_t lcd_autodis_list_maxnum;    /*轮显最大屏数 */
    uint8_t lcd_keydis_list_num;    /*键显屏数 */
    uint8_t lcd_keydis_list_maxnum;    /*键显最大屏数 */
    uint8_t energy_point; //电能显示小数
    uint8_t power_point;    //功率显示小数
    uint8_t lcd_12_mean;    //液晶12字样意义
    uint16_t lcd_autodis_keep_sec;    /*轮显持续时间 */
    uint16_t lcd_keydis_keep_sec;    /*按键轮显持续时间 */
    uint16_t lcd_nopower_key_keep_sec;/*显示查看显示时间 */
    
    uint8_t Baut_rs485; //485波特率
    uint8_t Baut_plc;   //载波波特率(EEPROM)
    uint8_t Baut_plc_neg;   //协商波特率（RAM）
    
    uint8_t SafeEn; //安全模式  1：启用，0：不启用
    
    uint8_t relay_ctl_mode; //继电器控制方式（0：脉冲式，1：保持式）

    
    uint16_t Limit_T_Max;   //广播校时最大允许范围
    uint16_t Limit_T_Min;   //广播校时最小允许范围

        //事件参数
    uint32_t OverCurrent_threshold;         //参数 电能表过流事件-电流触发下限 
    uint16_t ZeroLineErrS_rate_threshold;   // 电能表零线电流异常事件 限值
    uint32_t ZeroLineErrS_Current_threshold;// 电能表零线电流异常事件-电流触发下限
    uint8_t OverCurrent_duration_para;  //参数 判定延时时间  
    uint8_t PowerOffS_duration_para;    //参数 判定延时时间  
    uint8_t CoverOpenS_duration_para;   //参数 判定延时时间  
    uint8_t SwitchErrS_duration_para;   //参数 判定延时时间  
    uint8_t EmuErrS_duration_para;      //参数 判定延时时间  
    uint8_t RTCErrS_duration_para;      //参数 判定延时时间  
    uint8_t ZeroLineErrS_duration_para; //参数 判定延时时间  
    
    uint32_t report_2015mode;               //跟随上报模式字   
    uint16_t active_report_sec_limit;   //掉电 主动上报 上电时间阈值 默认3600 

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
    uint8_t   StepNum;//阶梯数
    uint8_t    Fee;
    uint8_t TouBefNo;
    
    uint8_t MulPortMode;    //多功能口模式
          
    uint32_t volt_60;   //60%电压
    uint32_t volt_78;   //78%电压
    uint32_t volt_125;  //125%电压
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
    uint32_t temperature;
    uint32_t voltage_battery;
    
    uint8_t  p_pluse;          
    uint8_t  n_pluse;
    uint8_t  power_dir;
    uint8_t  power_qn;
    
    uint8_t  meter_cover_sta;
    uint8_t  realy_err_move;
    uint8_t  power_abnormal;
    uint8_t  rtc_chip_abnormal;
    uint8_t  meter_chip_abnormal;
    uint8_t  esam_chip_abnormal;    
    
    uint16_t lcd_checkdis_keep_sec;/*显示查看显示时间 */
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
    
    uint8_t baud_chg;   //波特率改变，BIT0(485),BIT1(PLC)
uint16_t plc_baut_tick; //1小时不通讯需要复位载波通道，波特率恢复9600

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

    //显示相关
    uint8_t commdisp_plc;    //通讯符号显示
    uint8_t commdisp_ir485;    //通讯符号显示
    uint8_t lcd_poweron_all_dis_time_tick;//全显倒计时
    
    //运行状态字
    uint16_t run_state[8];
   
    uint8_t relay_true_sta;//继电器实际状态
    
    uint8_t clientaddr; //客户机地址
    
//    uint16_t send_frm_size;             //客户机发送帧最大尺寸 long-unsigned
//    uint16_t recv_frm_size;             //客户机接收帧最大尺寸 long-unsigned
//    uint32_t connect_time;              //期望的应用连接超时时间 double-long-unsigned，
    uint8_t  connect_mode;              //认证模式enum connect_mode_e

    //事件相关
     uint8_t CoverOpenS_status;          //当前状态标志 电能表开盖事件
    uint8_t SwitchErrS_status;          //当前状态标志 电能表负荷开关误动作事件
    uint8_t SwitchErrS_Record_En;       //当前状态标志 负荷开关 误动作 使能记录 标志 
    uint8_t PowerAbnormalS_status;      //当前状态标志 电能表电源异常事件
    uint8_t EmuErrS_status;             //当前状态标志 计量芯片故障事件
    uint8_t RTCErrS_status;             //当前状态标志 时钟故障事件

    uint8_t MeterWr_SetTime_flag;       //当前状态标志 5A: 编程事件由校时产生

    uint8_t OverCurrentAS_duration;     //当前状态持续时间 电能表过流事件
    uint8_t PowerOffS_duration;         //当前状态持续时间 电能表掉电事件
    uint8_t CoverOpenS_duration;        //当前状态持续时间 电能表开盖事件
    uint8_t SwitchErrS_duration;        //当前状态持续时间 电能表负荷开关误动作事件
    uint8_t PowerAbnormalS_duration;    //当前状态持续时间 电能表电源异常事件 
    uint8_t EmuErrS_duration;           //当前状态持续时间 计量芯片故障事件
    uint8_t RTCErrS_duration;           //当前状态持续时间 时钟故障事件
    uint8_t ZeroLineErrS_duration;      //当前状态持续时间 零线电流异常事件  

    uint8_t first_MbillDay_flag;

    uint8_t meterclr_event_recordflag; //记录标识*/ // 通过fifo传递
    uint8_t meterclr_event_resetflag; //复位标识
    
    uint32_t battery_used_time; //电池使用时间   
    
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
    uint32_t alarmmin; //跳闸延时(min)
    uint32_t limitmin; //限电时间(min)
    uint32_t imaxmin;  //大电流延时跳闸（min）
    uint16_t cmd_dostate;  //控制命令执行状态字
    uint16_t cmd_errstate;  //控制命令错误状态字
    
    uint8_t mac_suspend_state;  // mac挂起状态 协议或显示模块可调用。 true为挂起
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
    //uint32_t RTCErrS_acc_time;          //累计时间 时钟故障事件
    uint32_t ZeroLineErrS_acc_time;     //累计时间 零线电流异常事件  

    //上报相关
    uint16_t active_report_CMDtype;             //主动上报 命令类别     需掉电保存   
    uint8_t active_report_retry_cnt[4];         //主动上报 重试次数 //R_3320
    uint32_t power_on_sec_accum;                //上电累计时间 sec  上电处理时清零，上电运行累加  
    //跟随上报2015状态字
    uint32_t report_2015state[4];           //跟随上报状态字  chl_NUM

    uint8_t mac_err_cnt; // 当日mac错误次数
    uint8_t mac_suspend_date[3];// mac错误 挂起日期 yymmdd


    struct tm v_powerdown_tm;                   // 模拟停电时间  补结算测试用 
    
    uint8_t persistent_blank_space[4*10];   //  空白区

    dyn_persistent_end

    int id;
    //os相关的字段
    os_fields

};

enum resourenc_block_name
{
    resource_block_name = 0x01,
    control_block_cache,
    control_block_698_name,
    mbase_control_block_name,
    mbase_block_static,
    mbase_block_static_eprom,           //主区  
    mbase_block_static_eprom_bp,        //备份 需跟在主区后面 
    mbase_block_cache,
    mbase_block_persistent,
    mbase_block_persistent_eprom,
    mbase_block_event,
    mbase_block_parameter,              //不带备份 
    mbase_block_parameter_1,            //主区 
    mbase_block_parameter_1_bp,         //备份 需跟在主区后面
    mbase_block_event_flash1,
    mbase_block_event_flash2,
    public_block_flash,
};

enum resourenc_map_name
{
    res_map_698 = 0x01,
};

uint8_t scond_execute_for_demand(struct mbase_control_block_t* mbase_block);
uint8_t minute_execute_for_demand(struct mbase_control_block_t* mbase_block);
int8_t add_demand_action(struct mbase_control_block_t* mbase_block, uint32_t* import, uint32_t output, uint8_t max_slide, uint8_t mode);

extern void Rtc_Prossecc(struct instance_t* instance, struct pipeline_t* pipeline);
extern void TouFeePro(struct instance_t* instance, struct pipeline_t* pipeline);

void Electrical_measurement_poweron(struct instance_t* instance, struct pipeline_t* pipeline);
void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Pluse_Count(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline);
int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t type);
int32_t Get_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t type , uint8_t pn);
int32_t Get_Month_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index , uint8_t pn);
void Energy_clr(struct instance_t* instance);

int16_t supply_parameter_info(struct instance_t* instance, uint16_t id, uint32_t* addr, uint32_t* size,uint32_t *offset);
int16_t supply_parameter_1_info(struct instance_t* instance, uint16_t id, uint32_t* addr, uint32_t* size,uint32_t *offset);
int16_t supply_event_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth);
int16_t supply_event_flash_1_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth);
int16_t supply_event_flash_2_info(struct instance_t* instance, uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth);

int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx, uint16_t mode);
int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int app_rx_server_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);


int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int app_meter_lowpower_wakeup_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

void dev_init(struct instance_t* instance, uint8_t power_mode);
void irq_system_init(void);
void irq_transform_to_instance(struct instance_t* instance);
void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , uint8_t chl_type , struct instance_t* p_instance);
void IOStateCheck(struct instance_t* instance, struct pipeline_t* pipeline);

extern const struct dev_info_t eeprom_dev;
extern const struct dev_info_t nor_flash_dev;

enum device_id_t
{
    DEV_EEPROM        = 1,
    DEV_FLASH         ,
    DEV_EMU           ,
    DEV_RAM           ,
};





// GS-----start
struct record_storage_id_info_t
{
    uint32_t save_index;    // 记录存储地址
    uint32_t sequence_index;// 序号存储地址
    uint32_t capture_index; // 关联对象存储地址
    uint32_t period_index;  // 周期存储地址
    uint32_t depth_index;   // 深度存储地址
    uint16_t capture_len;   // 关联对象存储长度
    uint16_t period_len;    // 周期存储长度
    uint16_t depth_len;     // 深度存储长度
    uint32_t rpt_flag;      // 上报标识
    uint32_t rpt_mode;      // 上报方式
    uint32_t *acc_time;     // 事件累计时间   使用前需判断是否位null
    const uint32_t *record_head;  // 记录头
};

enum format_conversion_mode // record_change_format
{
    RFC_WRITE,           //存储写入转换
    RFC_READ,            //协议读出转换
    RFC_GET_ORIGINAL_LEN,//获取原协议格式len
    RFC_GET_STORE_LEN,   //获取存储格式len
};

struct format_conversion_t
{
    uint32_t oad;
    uint16_t original_len; // ffff 按照tag格式解析        或  特殊处理
    uint16_t store_len;    // ffff 按照tag格式解析        或  特殊处理 
    int (*format_fun)(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc);
};


#define     MaxSeasonNum    		14
#define 	MaxTouNum     			14
#define 	MaxFeeNum     			12
#define     MaxHolidayNum   		10
#define		MaxTouTabNum			8

//Fee
#define		F_TOUChange			0x01
#define     F_DAYTABChange      0x02
#define		F_TOUFeeChangeTm    0x04
#define		F_ChangeTmOn        0x08

//relay
#define     Relay_Ck_Time       100    //1秒

//key
#define     keydlymax           100     

#define     MAX_VOLTAGE     4000 //400.0v
#define     MIN_VOLTAGE     500 //50.0v
#define     UN_VOLTAGE      2200 //220.0v
#define     MAX_CURRENT     120000 //120.000A
#define     MIN_CURRENT     17     //10mA

#define     AS_Un      2200
#define     AS_Ib      5000
#define     AS_P       11000


extern int get_record_id_info(struct instance_t* instance, uint32_t oad, struct record_storage_id_info_t* info);
extern int record_change_format(struct instance_t* instance, uint8_t *des,  uint8_t* src, uint32_t oad_req,uint32_t oad_src, enum format_conversion_mode mode);
extern int clr_record(struct instance_t* instance, uint32_t oad);
extern int para_init(struct instance_t* instance );
extern int sync_data_record(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
//GS----end




extern int period_check1(uint16_t oi, uint16_t* capture_period);
extern int period_check2(uint8_t scheme, uint16_t period, uint8_t* period_buf);
extern uint8_t capture_check(uint32_t oad, uint8_t* oad_buf);

extern int read_esam_data(struct instance_t* instance,uint32_t addr, uint8_t *buf, uint16_t len);
extern int write_esam(struct instance_t* instance, uint32_t addr, uint8_t*buf, uint16_t len);
extern int read_esam_no(struct instance_t* instance, uint8_t *esam_info);
extern int read_esam_info(struct instance_t* instance, uint8_t *esam_info, uint8_t index);
extern int esam_consult_key(struct instance_t* instance, uint8_t cmd, uint8_t *p);

extern int backlight_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);

extern void Meter_Run_State(struct instance_t* instance, struct pipeline_t* pipeline);

extern int freeze_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);

extern int event_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

extern void Display(struct instance_t* instance, struct pipeline_t* pipeline);

extern void do_relay(struct instance_t* instance,  struct pipeline_t* pipeline);

extern void Key_Display(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t ctr);

extern void AllDisplayLCD(struct instance_t* instance, struct pipeline_t* pipeline);

extern uint8_t Magnetic_hold_relay_Init(void);

extern uint8_t lcd_init(uint8_t power_mode);

extern void rtc_get_from_tm(struct instance_t* instance, uint8_t* p);

extern uint8_t get_key_status(struct instance_t* instance);

extern unsigned short display_oad_to_id(unsigned char *oad, unsigned char *oad_son, unsigned char Sequ, unsigned char len);
extern int general_set_4_para(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode);

extern uint8_t Date_Format_Check( unsigned char* Addr );
extern uint8_t Time_Format_Check( unsigned char* Addr );
extern uint8_t display_id_to_oad(unsigned short id, unsigned char *oad);
extern void relay_led_ctl(struct instance_t* instance,  struct pipeline_t* pipeline);
extern signed char Comm_Read(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern signed char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern signed char Comm_Other(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);

extern uint16_t software_esam_check(struct instance_t* instance, uint8_t *key, uint8_t *rn, uint8_t *data, uint8_t p2, uint16_t datalen, uint8_t *outbuf);
extern int esam_key_updata(struct instance_t* instance, uint8_t *buf, uint16_t len);
extern uint32_t fr_698_current_esam_timeout_get(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t client_index);

extern int record_rollback_after_set_time(struct instance_t* instance, uint8_t finish_check);
extern int cal_record_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad);
extern int modify_capture_clr_pro(struct instance_t* instance, uint32_t oad);
extern int set_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t mode);
extern uint8_t get_activeflag(struct instance_t* instance, uint16_t RecordId);
extern int meter_clr(struct instance_t* instance);
extern void meteric_para_clr(struct instance_t* instance, uint8_t *p_rx);
extern int event_clear_omd_add(struct instance_t* instance, uint32_t omd, uint16_t chn);
extern int get_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t mode);
extern int freez_size_sectors_init(struct instance_t* instance, uint32_t oad);
extern int MeterWr_obj_list_add(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint32_t oad, uint8_t tag);
extern int event_PowerOn_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
extern void fr_dl698_power_on(struct instance_t* instance,  struct pipeline_t* pipeline);
extern int event_LowPower_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern void fr_dl698_sec_task(struct instance_t* instance,  struct pipeline_t* pipeline);
//extern int GET_record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info,  void *ctx);
//extern int GET_record_sel_2(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info,  void *ctx);
//extern int GET_record_sel_1(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info,  void *ctx);
extern int security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad);
extern void fr_698_MeterWr_program_judge(struct instance_t* instance, struct pipeline_t* pipeline,  struct message_t* mesg);

extern int fr_handle_disconnect_all_channel(struct instance_t* instance,  struct pipeline_t* pipeline,  uint8_t cilent_index);
extern int fr_handle_disconnect2(struct instance_t* instance,  struct pipeline_t* pipeline,  uint8_t cilent_index, uint8_t channel);
extern int check_oad(uint32_t oad);


extern void ndv_timer_init(void);

extern void MComp1Init(void);
extern void esam_698_init(struct instance_t* instance, uint8_t power_mode);
extern void  v85xxp_clear_watchdog(void);

#if RESET_INFO
extern  __no_init uint32_t resetinfo_file; //复位信息 
extern  __no_init uint32_t resetinfo_line; //复位信息
#endif 
#endif
