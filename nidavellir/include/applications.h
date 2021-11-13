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
    float k_u;
    float k_il;
    float k_in;
    float k_p;    
    uint32_t meter_const;
    static_end     
    //运行期更改 && 掉电不保存 && 上电不需要IO请求的区域 && 初始化时直接清零
    dyn_cache_start   
    I_data_fields
          
    unsigned char   DayTableNo;//日表序号
    unsigned char   TOUNo;//费率序号
    unsigned char   STEPNo;//阶梯序号
    unsigned long   TOUPrice;//费率电价
    unsigned long   STEPPrice;//阶梯电价
    unsigned char   SeasonTable;//时区表主副表序号
    unsigned char   DayTable;//日表主副表序号
    unsigned char   TouPriceTable;//费率电价表主副表序号
    unsigned char   StepTable;//阶梯表主副表序号
    unsigned char   StepSeason;//阶梯时区表主副表序号
    unsigned long   Price;//当前电价
    unsigned char   StepNum;//阶梯数
          
    uint32_t voltage_ac;
    uint32_t current_l;
    uint32_t active_power;
    uint32_t current_n;
    uint32_t frequency;
    uint32_t power_factor;
    uint32_t temperature;
    uint32_t voltage_battery;    
    

  
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
    uint8_t lcd_autodis_keep_sec;    /*轮显持续时间 */
    uint16_t lcd_checkdis_keep_sec;/*显示查看显示时间 */
    uint16_t lcd_checkdis_all_keep_sec;/*显示查看全屏显示时间 */
    uint8_t under_un68; //所有相电压均低于68% 
    uint8_t auxiliary_power;//辅助电源供电标志
    //uint8_t remote_alarm_flag; //远程告警命令true-触发告警
    //uint8_t battery1_under2v8; //时钟电池欠压
    //uint8_t battery2_under4v8; //停抄电池欠压

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
};

uint8_t scond_execute_for_demand(struct mbase_control_block_t* mbase_block);
uint8_t minute_execute_for_demand(struct mbase_control_block_t* mbase_block);
int8_t add_demand_action(struct mbase_control_block_t* mbase_block, uint32_t* import, uint32_t output, uint8_t max_slide, uint8_t mode);


void Electrical_measurement_poweron(struct instance_t* instance, struct pipeline_t* pipeline);
void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerON(struct instance_t* instance, struct pipeline_t* pipeline);
void Energy_Meter_PowerOFF(struct instance_t* instance, struct pipeline_t* pipeline);


extern const struct dev_info_t eeprom_dev;
extern const struct dev_info_t nor_flash_dev;

#endif
