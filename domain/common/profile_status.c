#include <stddef.h>
#include "domain/common/profile_status.h"

void nd_profile_status_set(uint8_t *status, uint8_t bits)
{
    if (status != NULL) {
        *status |= (uint8_t)(bits & (uint8_t)~ND_PROFILE_STATUS_RESERVED);
    }
}

void nd_profile_status_clear(uint8_t *status, uint8_t bits)
{
    if (status != NULL) {
        *status &= (uint8_t)~(bits & (uint8_t)~ND_PROFILE_STATUS_RESERVED);
    }
}

int nd_profile_status_has(uint8_t status, uint8_t bits)
{
    return (status & bits) == bits;
}

uint8_t nd_profile_status_normalize(uint8_t status)
{
    return (uint8_t)(status & (uint8_t)~ND_PROFILE_STATUS_RESERVED);
}
