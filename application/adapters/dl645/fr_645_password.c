#include "application/adapters/dl645/fr_645_password.h"

uint8_t fr_645_password_matches(const uint8_t entered[4],
                                const uint8_t stored[4])
{
    uint8_t i;

    for (i = 1; i < 4; i++) {
        if (entered[i] != stored[i]) {
            return 1;
        }
    }
    return 0;
}
