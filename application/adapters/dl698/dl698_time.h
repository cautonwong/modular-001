#ifndef ND_APPLICATION_ADAPTERS_DL698_TIME_H
#define ND_APPLICATION_ADAPTERS_DL698_TIME_H

#include <stdint.h>

#define TIME_WILDCARD 1

uint8_t dl_data_time_check(uint8_t *date_time, uint8_t type);
uint32_t dl_timeTag_interval(uint8_t *date, uint8_t type);
uint32_t dl_datetime_to_second(uint8_t *date, uint8_t type);

#endif
