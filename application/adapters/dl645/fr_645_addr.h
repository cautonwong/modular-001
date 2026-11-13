#ifndef ND_APPLICATION_ADAPTERS_DL645_ADDR_H
#define ND_APPLICATION_ADAPTERS_DL645_ADDR_H

#include <stdint.h>

uint8_t fr_645_addr_all(const uint8_t addr[6], uint8_t value);
uint8_t fr_645_addr_match(uint8_t addr[6], const uint8_t tsa[6],
                          uint8_t wildcard);

#endif
