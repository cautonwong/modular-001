#ifndef ND_DOMAIN_COMMON_CLOCK_VALUE_H
#define ND_DOMAIN_COMMON_CLOCK_VALUE_H

#include "domain/common/calendar.h"

typedef struct {
    nd_datetime_t time;
    uint8_t day_of_week; /* 1 = Monday ... 7 = Sunday */
    uint8_t hundredths;
    int16_t deviation; /* minutes from UTC */
    uint8_t status;
} nd_clock_value_t;

#endif
