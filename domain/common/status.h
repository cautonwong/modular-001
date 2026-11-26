#ifndef ND_DOMAIN_COMMON_STATUS_H
#define ND_DOMAIN_COMMON_STATUS_H

/* Semantic status values. The 0xAA/0x55 customer wire codes live in the
 * target's meter_wire_status.h and are mapped by the meter-base port. */
#define ND_STATUS_RESET 0
#define ND_STATUS_SET 1

#define ND_POWER_OFF 0
#define ND_POWER_ON 1

#endif
