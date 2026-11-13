#ifndef __GS_RECORD_H__
#define __GS_RECORD_H__

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct message_t;
struct tm;

#define _value(a) a = ((int32_t)a < 0)? (int32_t)(-a) : (int32_t)a


#define SINGLE_CAPTURE_MAXLEN 200

/* These records cross module boundaries. Pin their layout so an include-order
 * pack pragma cannot change what one translation unit writes and another
 * reads. */
#pragma pack(push, 8)

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

enum package_type_645 //
{
    PACKAGE645_NORMAL = 0,   //事件&结算&冻结  不打包的
    PACKAGE645_FREEZE,      //冻结   //AA打包
    PACKAGE645_LOAD,        //负荷
};



struct record_storage_645_id_info_t
{
    struct record_storage_id_info_t id_info;
    uint32_t oad698;
    uint8_t package_type;// package_type:  0  表示事件;  1   冻结;  2 负荷记录
    uint8_t package_num; // package_type为1时,package_num表示AA打包个数。
                         // package_type为其他时，package_num表示是否支持块内单个数据项抄读等信息，  0，不支持，1:支持 且 子项id从1开始。  0xFF:支持,子项id从0开始
    uint8_t mark_para;  //模式字
    uint8_t mark_bit;   //类/通道 如负荷记录   0-5类
    const uint32_t *rd_645obj;  //关联对象 //首字节个数
    const uint16_t *rd_645obj_len;     //关联对象数据内容长度，首字节总长           
    const uint16_t *rd_645obj_package_len;   //冻结AA打包长度，   BIT15: 长度变长      BIT14:单相不支持
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
    uint16_t original_len; // ffff 由format_fun处理  按照tag格式解析        或  特殊处理    
    uint16_t store_len;    // ffff 由format_fun处理     按照tag格式解析        或  特殊处理 
    int (*format_fun)(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc);
};

int get_record_id_info(struct instance_t* instance, uint32_t oad, struct record_storage_id_info_t* info);
int record_change_format(struct instance_t* instance, uint8_t *des, uint8_t* src, uint32_t oad_req, uint32_t oad_src, enum format_conversion_mode mode);
int clr_record(struct instance_t* instance, uint32_t oad);
int para_init(struct instance_t* instance);
int sync_data_record(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int period_check1(uint16_t oi, uint16_t* capture_period);
int period_check2(uint8_t scheme, uint16_t period, uint8_t* period_buf);
uint8_t capture_check(uint32_t oad, uint8_t* oad_buf);
void meteric_para_clr(struct instance_t* instance, uint8_t *p_rx);
int event_clear_omd_add(struct instance_t* instance, uint32_t omd, uint16_t chn);
int MeterWr_obj_list_add(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint32_t oad, uint8_t tag);


struct event_duration_t
{
    uint8_t duration_para;
    uint8_t* duration;
    uint8_t state;
};

extern const uint32_t event_oad_list[];
extern const uint8_t event_oad_num;

void record_event_no_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode);
uint8_t get_activeflag(struct instance_t* instance, uint16_t RecordId);

void set_activeflag(struct instance_t* instance, uint16_t RecordId);
void clr_activeflag(struct instance_t* instance, uint16_t RecordId);

void set_recordflag(struct instance_t* instance, uint16_t RecordId);

void clr_recordflag(struct instance_t* instance, uint16_t RecordId);
void clr_event_duration(struct instance_t* instance, uint16_t RecordId);
void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload);
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode);
int get_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf,  uint8_t* dar);
void record_event_no_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode);

int ia_over_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int CoverOpen_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int PowerOff_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
void PowerOff_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int EmuErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int RTCErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int PowerAbnormal_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
void PowerAbnormal_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);

int ZeroLineErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int SwitchErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);

int event_clr(struct instance_t* instance);
int freeze_clr(struct instance_t* instance);
int meter_clr(struct instance_t* instance);
int freez_size_sectors_init(struct instance_t* instance, uint32_t oad);
int record_rollback_after_set_time(struct instance_t* instance, uint8_t finish_check);
int modify_capture_clr_pro(struct instance_t* instance, uint32_t oad);

int MeterWr_obj_list_clear(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int clr_last_bill_time(struct instance_t* instance, uint32_t ID);
int event_poweroff_save(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
void write_powerdown_time(struct instance_t* instance);
int trigger_MeterWr645_record(struct instance_t* instance, uint32_t DI645, uint8_t tag);

//ⲿӿ
extern void report_2015state_judge(struct instance_t* instance);
extern void rtc_format_change_from_tm(struct tm *time, uint8_t* p);
extern int recv_record_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* payload);
extern int get_capture_list(struct instance_t* instance, uint32_t oad, uint32_t *rcsd_list, uint8_t mode);
extern uint16_t nd_log_append_rollback(struct instance_t* instance, uint32_t index, uint8_t public_flash_name, uint32_t last_time,uint32_t curr_time);

extern uint32_t dt_buf_to_second(uint8_t* dt);
extern void mac_suspend_reset(struct instance_t* instance);
extern void mac_suspend_sec_pro(struct instance_t* instance, struct pipeline_t* task);
extern void esam_698_power_on(struct instance_t* instance);
extern int rtc_check_poweron(struct instance_t* instance);
extern void esam_sec_pro(struct instance_t* instance, struct pipeline_t* pipeline);
extern void updata_powerdown_time(struct instance_t* instance);
extern int check_record_item_support(struct instance_t* instance, uint32_t oad);
extern int check_record_table_oad__support(struct instance_t* instance, uint32_t oad);
extern int updata_EC_checksum(struct instance_t* instance, uint8_t type);
extern int MeterWr645_obj_list_clear(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
extern int16_t Save_EC2(struct instance_t* instance, uint8_t type);
extern int CoverOpen_event_LowPower_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
extern int after_Time_set_hook(struct instance_t* instance);
extern int set_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t value, uint8_t mode);
extern void EventOut_698_check(struct instance_t* instance);
extern int save_RtcBat_work_time(struct instance_t* instance);
#pragma pack(pop)
#endif //__GS_RECORD_H__