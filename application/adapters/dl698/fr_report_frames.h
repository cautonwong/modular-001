#ifndef ND_APPLICATION_DL698_FR_REPORT_FRAMES_H
#define ND_APPLICATION_DL698_FR_REPORT_FRAMES_H

#include "fr_698.h"
#include "domain/report/report_retry.h"

uint8_t report_channel_check(struct instance_t* instance, uint16_t channel);
uint8_t active_report_enable_check(struct instance_t* instance,
                                   struct pipeline_t* pipeline,
                                   nd_dl698_msg_t* mesg);
uint8_t active_report_check_3011(struct instance_t* instance,
                                 struct pipeline_t* pipeline,
                                 nd_dl698_msg_t* mesg, uint8_t* buf,
                                 uint8_t eventtype);
uint8_t active_report_3320list_get(struct instance_t* instance,
                                   struct pipeline_t* pipeline,
                                   nd_dl698_msg_t* mesg, uint8_t* buf);
uint32_t report_2015_get(struct instance_t* instance,
                         struct pipeline_t* pipeline, nd_dl698_msg_t* mesg,
                         uint16_t chn, enum report_mode_t mode);
int active_report_3011_pro(struct instance_t* instance,
                           struct pipeline_t* pipeline, nd_dl698_msg_t* mesg,
                           uint8_t eventtype);
int active_report_normal_pro(struct instance_t* instance,
                             struct pipeline_t* pipeline, nd_dl698_msg_t* mesg,
                             uint8_t get_data, nd_report_retry_t* retry);

#endif
