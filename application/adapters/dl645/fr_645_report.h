#ifndef __FR_645_REPORT_H__
#define __FR_645_REPORT_H__

extern void report_645state_judge(struct instance_t* instance, uint32_t OAD_State698);
extern void report_3320flag_set_from_698(struct instance_t* instance, uint32_t oad, uint8_t value);
extern void report_2015mode_set_from_698(struct instance_t* instance);
extern void report_645state_reset_sec_judge(struct instance_t* instance);
extern int report_645stat_recover(struct instance_t* instance);
extern int report_645stat_save(struct instance_t* instance);
extern uint8_t report_stat_reset_from_698(struct instance_t* instance, uint32_t State2015_698);
extern void report_state_read_pro(struct instance_t* instance, uint8_t *buff, uint8_t *length);
extern void report_mode_set_from_645(struct instance_t* instance, uint8_t* report_mode);
extern uint8_t report_645stat_reset_from_645(struct instance_t* instance, uint8_t state_cnt645);
extern uint8_t report_stat_reset_from_3320_698(struct instance_t* instance, uint32_t OAD_State698);
extern void event_out_check(struct instance_t* instance);
extern int report_push_data_judge(struct instance_t* instance, uint8_t* databuf,  struct context_645_t* ctx);
#endif//__FR_645_REPORT_H__
