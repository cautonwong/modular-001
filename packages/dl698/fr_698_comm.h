#ifndef __FR_698_COMM_H__
#define __FR_698_COMM_H__


#ifndef WIN32_U_TEST
#define fr_698_get nd_get
#define fr_698_set nd_set
#define fr_698_action nd_action
#else
#define fr_698_get __nd_get
#define fr_698_set __nd_set
#define fr_698_action __nd_action

int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size);
int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size);
int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size);
int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size);
int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size);

#endif

uint8_t dl_data_time_check(uint8_t* date_time, uint8_t type);
uint32_t dl_timeTag_interval(uint8_t* date, uint8_t type);
uint32_t dl_datetime_to_second(uint8_t* date, uint8_t type);

#endif //__FR_698_COMM_H__