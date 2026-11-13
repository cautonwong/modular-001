#ifndef ND_DOMAIN_SECURITY_ESAM_RESULT_H
#define ND_DOMAIN_SECURITY_ESAM_RESULT_H

#include <stdint.h>

enum {
    ND_ESAM_RESULT_FAIL = -1,
    ND_ESAM_RESULT_OK = 1
};

int nd_esam_result(int err_no, uint8_t verification_type, int mac_matches,
                   int enforce_mac);
int nd_esam_mac_optional_matches(const uint8_t *expected,
                                 uint16_t expected_len,
                                 const uint8_t *actual, uint16_t actual_len);

#endif
