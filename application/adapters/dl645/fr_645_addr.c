#include "application/adapters/dl645/fr_645_addr.h"

uint8_t fr_645_addr_all(const uint8_t addr[6], uint8_t value)
{
    uint8_t i;

    for (i = 0; i < 6; i++) {
        if (addr[i] != value) {
            return 1;
        }
    }
    return 0;
}

uint8_t fr_645_addr_match(uint8_t addr[6], const uint8_t tsa[6],
                          uint8_t wildcard)
{
    uint8_t i;

    if (wildcard) {
        for (i = 0; i < 6; i++) {
            if (addr[5 - i] == 0xAA) {
                addr[5 - i] = tsa[5 - i];
            } else {
                break;
            }
        }
    }
    for (i = 0; i < 6; i++) {
        if (addr[i] != tsa[i]) {
            return 1;
        }
    }
    return 0;
}
