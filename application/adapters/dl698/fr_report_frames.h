#ifndef ND_APPLICATION_DL698_FR_REPORT_FRAMES_H
#define ND_APPLICATION_DL698_FR_REPORT_FRAMES_H

#include "fr_698.h"
#include "domain/report/report_retry.h"

uint8_t report_channel_check(void * instance, uint16_t channel);
void report_port_open(void * instance);
void report_2015state_judge(void * instance);
uint32_t follow_report_3320List_Latest_oad_get(void * instance, uint8_t chn);
uint8_t active_report_enable_check(void * instance,
                                   void * pipeline,
                                   nd_dl698_msg_t* mesg);
uint8_t active_report_check_3011(void * instance,
                                 void * pipeline,
                                 nd_dl698_msg_t* mesg, uint8_t* buf,
                                 uint8_t eventtype);
uint8_t active_report_3320list_get(void * instance,
                                   void * pipeline,
                                   nd_dl698_msg_t* mesg, uint8_t* buf);
uint32_t report_2015_get(void * instance,
                         void * pipeline, nd_dl698_msg_t* mesg,
                         uint16_t chn, enum report_mode_t mode);
int active_report_3011_pro(void * instance,
                           void * pipeline, nd_dl698_msg_t* mesg,
                           uint8_t eventtype);
int active_report_normal_pro(void * instance,
                             void * pipeline, nd_dl698_msg_t* mesg,
                             uint8_t get_data, nd_report_retry_t* retry);

#endif
