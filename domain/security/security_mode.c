#include "domain/security/security_mode.h"

int nd_security_mode_check(uint16_t mode, uint8_t cmd, uint8_t security_choice,
                           uint8_t verification_choice)
{
    uint16_t word = mode;

    if (cmd == ND_SECURITY_CMD_SET) {
        word = (uint16_t)(word << 4);
    } else if (cmd == ND_SECURITY_CMD_ACTION) {
        word = (uint16_t)(word << 8);
    }

    if (security_choice != 0xFFu) {
        if (security_choice == ND_SECURITY_CHOICE_CIPHER) {
            if (verification_choice == ND_SECURITY_AUTH_SID_MAC) {
                return ND_SECURITY_ALLOW;
            }
            if (verification_choice == ND_SECURITY_AUTH_SID) {
                return (word & 0x1000u) ? ND_SECURITY_DENY_AUTH
                                        : ND_SECURITY_ALLOW;
            }
        } else {
            if ((verification_choice == ND_SECURITY_AUTH_RN_MAC)
                || (verification_choice == ND_SECURITY_AUTH_SID_MAC)) {
                return (word & 0x3000u) ? ND_SECURITY_DENY_AUTH
                                        : ND_SECURITY_ALLOW;
            }
            if (verification_choice == ND_SECURITY_AUTH_RN) {
                if (cmd == ND_SECURITY_CMD_GET) {
                    return (word & 0xC000u) ? ND_SECURITY_ALLOW
                                            : ND_SECURITY_DENY_AUTH;
                }
                return (word & 0x7000u) ? ND_SECURITY_DENY_AUTH
                                        : ND_SECURITY_ALLOW;
            }
        }
        return ND_SECURITY_DENY_AUTH;
    }

    if (cmd == ND_SECURITY_CMD_GET) {
        if ((word & 0x8000u) != 0x8000u) {
            return ND_SECURITY_DENY_AUTH;
        }
        return ND_SECURITY_NEED_PLAIN_CHECK;
    }

    if (word & 0x7000u) {
        return ND_SECURITY_DENY_AUTH;
    }
    return ND_SECURITY_ALLOW;
}
