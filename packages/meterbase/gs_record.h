#ifndef __GS_RECORD_H__
#define __GS_RECORD_H__


struct event_duration_t
{
    uint8_t duration_para;
    uint8_t* duration;
    uint8_t state;
};




uint8_t get_activeflag(struct pipeline_t* task, uint16_t RecordId);

void set_activeflag(struct pipeline_t* task, uint16_t RecordId);
void clr_activeflag(struct pipeline_t* task, uint16_t RecordId);

void set_recordflag(struct pipeline_t* task, uint16_t RecordId);

void clr_recordflag(struct pipeline_t* task, uint16_t RecordId);

int event_duration_pro(uint8_t duration_para,uint8_t *duration,uint8_t state, struct pipeline_t* task, uint16_t RecordId);
void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint16_t FlagId, uint8_t* payload);
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint16_t RecordId, uint8_t* payload, int mode);
int get_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf,  uint8_t* dar);

int ia_over_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);
int CoverOpen_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg);

#endif //__GS_RECORD_H__