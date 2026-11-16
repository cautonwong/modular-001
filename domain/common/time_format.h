#ifndef ND_DOMAIN_COMMON_TIME_FORMAT_H
#define ND_DOMAIN_COMMON_TIME_FORMAT_H

#include <stdint.h>

extern const uint8_t MonTab[13];

/* Byte-array layouts used by the legacy protocol frames. Return 1 when the
 * value is invalid, matching the existing call sites. */
unsigned char Date_Format_Check(unsigned char *Addr);
unsigned char Time_Format_Check(unsigned char *Addr);
unsigned char GetMonMaxDay(unsigned char *Addr);

#endif
