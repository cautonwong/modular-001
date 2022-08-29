#ifndef __GS_RECORD_H__
#define __GS_RECORD_H__




struct event_duration_t
{
    uint8_t duration_para;
    uint8_t* duration;
    uint8_t state;
};

extern const uint32_t event_oad[];
extern const uint8_t event_oad_num;


uint8_t get_activeflag(struct instance_t* instance, uint16_t RecordId);

void set_activeflag(struct instance_t* instance, uint16_t RecordId);
void clr_activeflag(struct instance_t* instance, uint16_t RecordId);

void set_recordflag(struct instance_t* instance, uint16_t RecordId);

void clr_recordflag(struct instance_t* instance, uint16_t RecordId);

int event_duration_pro(uint8_t duration_para,uint8_t *duration,uint8_t state, struct instance_t* instance, uint16_t RecordId);
void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload);
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode);
int get_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf,  uint8_t* dar);

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

//外部接口
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
#endif //__GS_RECORD_H__