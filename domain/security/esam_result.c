#include "domain/security/esam_result.h"

#include <string.h>

#define ESAM_STATUS_OK 0x9000

int nd_esam_result(int err_no, uint8_t verification_type, int mac_matches,
                   int enforce_mac)
{
    if (err_no != ESAM_STATUS_OK) {
        return ND_ESAM_RESULT_FAIL;
    }
    if (verification_type == 0u) {
        return ND_ESAM_RESULT_OK;
    }
    if (verification_type == 1u) {
        if (!enforce_mac || mac_matches) {
            return ND_ESAM_RESULT_OK;
        }
        return ND_ESAM_RESULT_FAIL;
    }
    return ND_ESAM_RESULT_FAIL;
}

static int mac_present(const uint8_t *data, uint16_t length)
{
    return (data != NULL) && (length > 0u);
}

int nd_esam_mac_optional_matches(const uint8_t *expected,
                                 uint16_t expected_len,
                                 const uint8_t *actual, uint16_t actual_len)
{
    int expected_present = mac_present(expected, expected_len);
    int actual_present = mac_present(actual, actual_len);

    if (!expected_present && !actual_present) {
        return 1;
    }
    if (!expected_present || !actual_present || (expected_len < 4u)
        || (actual_len < 4u)) {
        return 0;
    }
    return memcmp(expected, actual, 4) == 0;
}
