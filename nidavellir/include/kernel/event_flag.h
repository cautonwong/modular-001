#ifndef __KERNEL_EVENT_FLAG_H__
#define __KERNEL_EVENT_FLAG_H__

int event_flags_set(struct instance_t* instance, int event_flag_id);
int event_flags_get(struct instance_t* instance, int event_flag_id);
int event_flags_clr(struct instance_t* instance, int event_flag_id);

#endif