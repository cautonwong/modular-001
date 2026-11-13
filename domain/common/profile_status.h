#ifndef ND_DOMAIN_COMMON_PROFILE_STATUS_H
#define ND_DOMAIN_COMMON_PROFILE_STATUS_H

#include <stdint.h>

/* IDIS P3-IP-PROF Table 20 profile status bits. */
#define ND_PROFILE_STATUS_PDN 0x80u
#define ND_PROFILE_STATUS_CAD 0x20u
#define ND_PROFILE_STATUS_DST 0x08u
#define ND_PROFILE_STATUS_DNV 0x04u
#define ND_PROFILE_STATUS_CIV 0x02u
#define ND_PROFILE_STATUS_ERR 0x01u
#define ND_PROFILE_STATUS_RESERVED 0x50u

void nd_profile_status_set(uint8_t *status, uint8_t bits);
void nd_profile_status_clear(uint8_t *status, uint8_t bits);
int nd_profile_status_has(uint8_t status, uint8_t bits);
uint8_t nd_profile_status_normalize(uint8_t status);

#endif
