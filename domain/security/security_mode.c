#include <stddef.h>
#include "domain/security/security_mode.h"

#define TEST_RIGHT 0x0000u
#define GET_PLAIN 0x8000u
#define GET_PLAIN_MAC 0x4000u
#define GET_CIPHER 0x2000u
#define GET_CIPHER_MAC 0x1000u
#define SET_PLAIN 0x0800u
#define SET_PLAIN_MAC 0x0400u
#define SET_CIPHER 0x0200u
#define SET_CIPHER_MAC 0x0100u
#define ACTION_PLAIN 0x0080u
#define ACTION_PLAIN_MAC 0x0040u
#define ACTION_CIPHER 0x0020u
#define ACTION_CIPHER_MAC 0x0010u
#define PROXY_GET 0x0004u
#define PROXY_SET 0x0002u
#define PROXY_ACTION 0x0001u

typedef struct {
    uint16_t oi;
    uint16_t mode;
} security_mode_entry_t;

static const security_mode_entry_t security_mode_exact[] = {
    {0x2015u, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN},
    {0x202Cu, TEST_RIGHT | GET_PLAIN | PROXY_GET},
    {0x202Fu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN},
    {0x4000u, TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_PLAIN | PROXY_GET},
    {0x4001u, TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | PROXY_GET},
    {0x4002u, TEST_RIGHT | GET_PLAIN | SET_PLAIN_MAC | PROXY_GET},
    {0x4003u, TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC},
    {0x400Au, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC},
    {0x400Bu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC},
    {0x401Cu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC},
    {0x401Du, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC},
    {0x401Eu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC},
    {0x4018u, TEST_RIGHT | GET_PLAIN_MAC},
    {0x4019u, TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC},
    {0x401Au, TEST_RIGHT | GET_PLAIN_MAC},
    {0x401Bu, TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC},
    {0x4111u, TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | PROXY_GET},
    {0x5000u, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN_MAC},
    {0xF000u, TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
    {0xF001u, TEST_RIGHT | GET_PLAIN | SET_CIPHER | ACTION_CIPHER},
    {0xF002u, TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN},
    {0xF100u, TEST_RIGHT | GET_PLAIN | SET_PLAIN_MAC | ACTION_CIPHER_MAC | PROXY_GET},
    {0xF101u, TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_CIPHER | PROXY_GET},
    {0xFFFEu, TEST_RIGHT | GET_CIPHER_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
    {0xFFFFu, TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN},
    {0x4500u, TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN},
};

static const security_mode_entry_t security_mode_high_byte[] = {
    {0x50FFu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
    {0x60FFu, TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_SET | PROXY_ACTION},
    {0x70FFu, TEST_RIGHT | GET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_ACTION},
    {0x80FFu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
    {0xF2FFu, TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_SET | PROXY_ACTION},
};

static const security_mode_entry_t security_mode_high_half_byte[] = {
    {0x0FFFu, TEST_RIGHT | GET_PLAIN | PROXY_GET},
    {0x1FFFu, TEST_RIGHT | GET_PLAIN_MAC},
    {0x2FFFu, TEST_RIGHT | GET_PLAIN_MAC | ACTION_PLAIN_MAC},
    {0x3FFFu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
    {0x4FFFu, TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC},
};

#define SECURITY_DEFAULT_MODE \
    (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC)

static const uint32_t security_plain_read_oads[] = {
    0x202C0200u,
    0x40000200u,
    0x40010200u,
    0x40020200u,
    0x41110200u,
};

static uint16_t find_entry(const security_mode_entry_t *table, size_t count,
                           uint16_t oi)
{
    size_t i;

    for (i = 0; i < count; i++) {
        if (table[i].oi == oi) {
            return table[i].mode;
        }
    }
    return 0;
}

uint16_t nd_security_mode_lookup(uint16_t oi)
{
    uint16_t mode;

    mode = find_entry(security_mode_exact,
                      sizeof(security_mode_exact) / sizeof(security_mode_exact[0]),
                      oi);
    if (mode != 0u) {
        return mode;
    }

    mode = find_entry(security_mode_high_byte,
                      sizeof(security_mode_high_byte) / sizeof(security_mode_high_byte[0]),
                      (uint16_t)(oi | 0x00FFu));
    if (mode != 0u) {
        return mode;
    }

    mode = find_entry(security_mode_high_half_byte,
                      sizeof(security_mode_high_half_byte) / sizeof(security_mode_high_half_byte[0]),
                      (uint16_t)(oi | 0x0FFFu));
    if (mode != 0u) {
        return mode;
    }

    return SECURITY_DEFAULT_MODE;
}

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

static int plain_read_listed(uint32_t oad)
{
    size_t i;

    for (i = 0; i < sizeof(security_plain_read_oads)
                            / sizeof(security_plain_read_oads[0]);
         i++) {
        if ((oad & 0xFFFFFF00u) == security_plain_read_oads[i]) {
            return 1;
        }
    }
    return 0;
}

int nd_security_plain_read_allowed(uint32_t oad)
{
    uint16_t oi = (uint16_t)(oad >> 16);

    if (((oi & 0xFF00u) == 0u) || (oi == 0xF100u)) {
        return 1;
    }
    return plain_read_listed(oad);
}

int nd_security_infrared_oad_allowed(uint32_t oad)
{
    return plain_read_listed(oad);
}
