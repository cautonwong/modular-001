#ifndef __APPLICATIONS_H__
#define __APPLICATIONS_H__

//当前系统中所有的application注册表容器
#include "port.h"
#ifndef WIN32
#include "cpu_surport.h"
#endif // !WIN32

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
    //上电需要发起IO请求的参数区  运行期不更改
    static_start  
    uint8_t tsa[16];

    float k_u;
    float k_il;
    float k_in;
    float k_p;    
    uint32_t meter_const;
    uint8_t  active_energy_cal_word;
    static_end     
    //运行期更改 && 掉电不保存 && 上电不需要IO请求的区域 && 初始化时直接清零
    dyn_cache_start   
    I_data_fields         
    uint8_t   DayTableNo;//日表序号
    uint8_t   TOUNo;//费率序号
    uint8_t   STEPNo;//阶梯序号
    uint8_t   SeasonTable;//时区表主副表序号
    uint8_t   DayTable;//日表主副表序号
    uint8_t   TouPriceTable;//费率电价表主副表序号
    uint8_t   StepTable;//阶梯表主副表序号
    uint8_t   StepSeason;//阶梯时区表主副表序号
    uint32_t   Price;//当前电价
    uint32_t   TOUPrice;//费率电价
    uint32_t   STEPPrice;//阶梯电价
    uint8_t   StepNum;//阶梯数
          
    uint32_t voltage_ac;
    int32_t current_l;
    int32_t active_power;
    int32_t current_n;
    uint32_t frequency;
    int32_t power_factor;
    uint32_t temperature;
    uint32_t voltage_battery;
    
    uint8_t  p_pluse;          
    uint8_t  n_pluse;
    uint8_t  power_dir;
    uint8_t  power_qn;
    dyn_cache_end
    //运行期更改&&掉电或定时保存&&上电需要IO请求的数据分区
    dyn_persistent_start
        //电能数据
    uint32_t active_p[MAX_FEE_NO+1];
    uint32_t active_n[MAX_FEE_NO+1];
    uint32_t month_active_p[MAX_FEE_NO+1];
    uint32_t month_active_n[MAX_FEE_NO+1];
    uint16_t active_p_dec[MAX_FEE_NO+1];
    uint16_t active_n_dec[MAX_FEE_NO+1];
        //需量数据
    dyn_persistent_end

    int id;
    //os相关的字段
    os_fields

    struct energy_ctx_t* energy_ctx;
    struct demand_list_t* demand_list;

    uint32_t backlight_switchOn_Hpriority_sec;/*上电或插卡等高优先级 背光持续打开时间,          赋值后每秒递减           */
    uint32_t backlight_switchOn_Lpriority_sec;/*背光打开持续时间          赋值后每秒递减 */
    uint32_t backlight_control_word;/*背光报警配置字 */
    // para
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
    uint16_t lcd_checkdis_keep_sec;/*显示查看显示时间 */
    uint16_t lcd_nopower_key_keep_sec;/*显示查看显示时间 */
    uint16_t lcd_checkdis_all_keep_sec;/*显示查看全屏显示时间 */
    uint8_t AutoPtr;    //轮显指针
    uint8_t AutoInv;
    uint8_t KeyPtr;
    uint8_t KeyInv;
    uint8_t Key;        //按键标志
    uint16_t Page_ID;   //显示ID
    uint8_t under_un68; //所有相电压均低于68%   
    uint8_t auxiliary_power;//辅助电源供电标志
    //uint8_t remote_alarm_flag; //远程告警命令true-触发告警
    //uint8_t battery1_under2v8; //时钟电池欠压
    //uint8_t battery2_under4v8; //停抄电池欠压

    uint8_t record_flag[40];  //事件或冻结记录存储标志 
    uint8_t active_flag[40];  //事件或冻结记录存储标志 
    uint32_t minute_freeze_index[8]; //分钟冻结8个序号 




    uint8_t cover_event_duration_para;
    uint8_t cover_event_duration;
    uint8_t cover_flag; 
    uint8_t cover_event_activeflag; 
    uint8_t cover_event_recordflag; 
    uint8_t cover_event_resetflag;  

    uint32_t cover_accumulate_time;
    //static:电能表芯片故障事件-判定延时时间
    uint8_t emu_err_event_duration_para;
    //dyn:当前芯片故障状态持续时间
    uint8_t emu_err_event_duration;    
    uint8_t emu_err_flag; // 当前计量芯片故障标志
    uint8_t emu_err_event_activeflag; /*发生标识*/
    uint8_t emu_err_event_recordflag; /*记录标识*/
    uint8_t emu_err_event_resetflag;  /*复位标识*/
    uint32_t emu_err_accumulate_time; /*累计时间*/
    uint8_t i_over_event_report_type; //过流事件上报标识
    uint8_t i_over_event_report_mode; //过流事件上报方式
    uint8_t i_over_event_duration_para;//static:电能表过流事件-判定延时时间 */ 
    uint32_t i_over_event_threshold;//static:电能表过流事件-电流触发下限 */ 
    uint8_t ia_over_event_duration;    //dyn:当前过流状态持续时间*/
    uint8_t ia_over_event_activeflag; // 电能表过流事件的有效标识
    uint8_t ia_over_event_recordflag; // 电能表过流事件的记录标识
    uint8_t ia_over_event_resetflag;  // 电能表过流事件的清零标识
    uint32_t ia_over_accumulate_time; //电能表过流事件的累计时间*/ 
    uint8_t mac_err_cnt; // 当日mac错误次数
    uint8_t mac_suspend_date[3];// mac错误 挂起日期 yymmdd

    uint8_t mac_suspend_state;  // mac挂起状态 协议或显示模块可调用。 true为挂起

    uint8_t meterclr_event_recordflag; //记录标识*/ // 通过fifo传递
    uint8_t meterclr_event_resetflag; //复位标识
    uint8_t factor_mode; //工厂模式
    
    //RELAY
    uint8_t cmd;	    //控制命令  0x1A , 0x1B....
    uint8_t mode;     //拉合闸方式（直接、允许）
	uint8_t sta;      //命令状态
    uint32_t alarmmin; //跳闸延时(min)
    uint32_t limitmin; //限电时间(min)
    uint32_t imaxmin;  //大电流延时跳闸（min）
};

enum resourenc_block_name
{
    resource_block_name = 0x01,
    control_block_cache,
    control_block_698_name,
    mbase_control_block_name,
    mbase_block_static,
    mbase_block_cache,
    mbase_block_persistent,
    mbase_block_persistent_eprom,
    mbase_block_event,
    mbase_block_parameter
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
void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline);
int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* , uint8_t index);

int16_t supply_parameter_info(uint16_t id, uint32_t* addr, uint16_t* size,uint16_t *offset);
int16_t supply_event_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth);
struct resources_obj_t;
int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode);
int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

void dev_init(void);
void irq_system_init(void);
void irq_transform_to_instance(struct instance_t* instance);
void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , uint8_t chl_type);

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
    uint32_t save_index;  // 记录存储地址信息
    uint32_t capture_index; // 关联对象存储地址信息
    uint32_t sequence_index;// 序号存储地址信息
    const uint32_t *record_head;  // 记录头
};
extern int get_record_id_info(uint32_t oad, struct record_storage_id_info_t* info);
extern int clr_record(struct instance_t* instance, uint32_t oad);

//GS----end



#endif
