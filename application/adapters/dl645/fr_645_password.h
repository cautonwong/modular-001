#ifndef ND_APPLICATION_ADAPTERS_DL645_PASSWORD_H
#define ND_APPLICATION_ADAPTERS_DL645_PASSWORD_H

#include <stdint.h>

uint8_t fr_645_password_matches(const uint8_t entered[4],
                                const uint8_t stored[4]);

#endif
