#ifndef ND_DOMAIN_SECURITY_SECURITY_MODE_H
#define ND_DOMAIN_SECURITY_SECURITY_MODE_H

#include <stdint.h>

/* Permission results. The adapter maps DENY_AUTH to its DAR response; the
   plain-read check stays in the adapter because it needs connection state. */
enum {
    ND_SECURITY_DENY_AUTH = -1,
    ND_SECURITY_ALLOW = 1,
    ND_SECURITY_NEED_PLAIN_CHECK = 2
};

enum {
    ND_SECURITY_CMD_GET = 0,
    ND_SECURITY_CMD_SET = 1,
    ND_SECURITY_CMD_ACTION = 2
};

enum {
    ND_SECURITY_CHOICE_PLAIN = 0,
    ND_SECURITY_CHOICE_CIPHER = 1,
    ND_SECURITY_CHOICE_NONE = 0xFF
};

enum {
    ND_SECURITY_AUTH_SID_MAC = 0,
    ND_SECURITY_AUTH_RN = 1,
    ND_SECURITY_AUTH_RN_MAC = 2,
    ND_SECURITY_AUTH_SID = 3
};

int nd_security_mode_check(uint16_t mode, uint8_t cmd, uint8_t security_choice,
                           uint8_t verification_choice);

#endif
