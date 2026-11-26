#ifndef ND_TARGET_STATEGRID_METER_WIRE_STATUS_H
#define ND_TARGET_STATEGRID_METER_WIRE_STATUS_H

#include <stdint.h>

/* Customer wire codes in the legacy control block. Application code uses the
 * semantic ND_STATUS_* and ND_POWER_* values from domain/common/status.h; the
 * meter-base port maps between the two. Target code that touches the control
 * block directly compares these codes. */
#define ND_WIRE_STATUS_RESET 0x55
#define ND_WIRE_STATUS_SET 0xAA
#define ND_WIRE_POWER_OFF 0x55
#define ND_WIRE_POWER_ON 0xAA

static inline int32_t nd_wire_status_to_semantic(uint8_t value)
{
    return value == ND_WIRE_STATUS_SET ? 1 : 0;
}

static inline uint8_t nd_wire_status_from_semantic(int32_t value)
{
    return (uint8_t)(value == 1 ? ND_WIRE_STATUS_SET : ND_WIRE_STATUS_RESET);
}

static inline int32_t nd_wire_power_to_semantic(uint8_t value)
{
    return value == ND_WIRE_POWER_ON ? 1 : 0;
}

static inline uint8_t nd_wire_power_from_semantic(int32_t value)
{
    return (uint8_t)(value == 1 ? ND_WIRE_POWER_ON : ND_WIRE_POWER_OFF);
}

#endif
