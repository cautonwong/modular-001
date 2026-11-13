#include "components/protocol/dlms/include/nd_dlms_apdu.h"

#include <string.h>

static const uint8_t app_context_ln[] = {0x60, 0x85, 0x74, 0x05,
                                         0x08, 0x01, 0x01};

static int append(uint8_t *out, size_t capacity, size_t *used,
                  const uint8_t *data, size_t length)
{
    if (length > capacity - *used) {
        return -1;
    }
    memcpy(&out[*used], data, length);
    *used += length;
    return 0;
}

static int append_byte(uint8_t *out, size_t capacity, size_t *used,
                       uint8_t value)
{
    return append(out, capacity, used, &value, 1);
}

static int auth_mechanism(uint8_t auth)
{
    switch (auth) {
    case ND_DLMS_AUTH_LLS:
        return 1;
    case ND_DLMS_AUTH_HLS_GMAC:
        return 5;
    case ND_DLMS_AUTH_HLS_SHA256:
        return 6;
    case ND_DLMS_AUTH_HLS_ECDSA:
        return 7;
    default:
        return -1;
    }
}

int nd_dlms_apdu_encode_aarq(const nd_dlms_aarq_t *aarq, uint8_t *buffer,
                             size_t capacity, size_t *length)
{
    if (aarq == NULL || buffer == NULL || length == NULL
        || aarq->max_rx_pdu == 0) {
        return -1;
    }

    uint8_t content[128];
    size_t used = 0;

    if (append_byte(content, sizeof(content), &used, 0xA1) != 0
        || append_byte(content, sizeof(content), &used, 9) != 0
        || append_byte(content, sizeof(content), &used, 0x06) != 0
        || append_byte(content, sizeof(content), &used, 7) != 0
        || append(content, sizeof(content), &used, app_context_ln,
                  sizeof(app_context_ln)) != 0) {
        return -1;
    }

    if (aarq->auth != ND_DLMS_AUTH_NONE) {
        int mechanism = auth_mechanism(aarq->auth);
        if (mechanism < 0 || aarq->auth_value_len != 8) {
            return -1;
        }
        const uint8_t acse[] = {0x8A, 0x02, 0x07, 0x80};
        const uint8_t mechanism_oid[] = {0x8B, 0x07, 0x60, 0x85,
                                         0x74, 0x05, 0x08, 0x02,
                                         (uint8_t)mechanism};
        const uint8_t auth_tag[] = {0xAC, 0x0A, 0x80, 0x08};
        if (append(content, sizeof(content), &used, acse, sizeof(acse)) != 0
            || append(content, sizeof(content), &used, mechanism_oid,
                      sizeof(mechanism_oid)) != 0
            || append(content, sizeof(content), &used, auth_tag,
                      sizeof(auth_tag)) != 0
            || append(content, sizeof(content), &used, aarq->auth_value, 8)
                   != 0) {
            return -1;
        }
    }

    uint8_t user_info[] = {0xBE, 0x10, 0x04, 0x0E, 0x01, 0x00, 0x00,
                           0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00,
                           0x7E, 0x1F, 0x04, 0x00};
    user_info[16] = (uint8_t)(aarq->max_rx_pdu >> 8);
    user_info[17] = (uint8_t)aarq->max_rx_pdu;
    if (append(content, sizeof(content), &used, user_info,
               sizeof(user_info)) != 0
        || used > 127) {
        return -1;
    }

    size_t total = 0;
    if (append_byte(buffer, capacity, &total, 0x60) != 0
        || append_byte(buffer, capacity, &total, (uint8_t)used) != 0
        || append(buffer, capacity, &total, content, used) != 0) {
        return -1;
    }
    *length = total;
    return 0;
}

static int read_tlv(const uint8_t **cursor, size_t *remaining, uint8_t *tag,
                    const uint8_t **value, size_t *value_length)
{
    if (*remaining < 2) {
        return -1;
    }
    const uint8_t *p = *cursor;
    *tag = p[0];
    size_t len = p[1];
    if (len == 0x80 || (len & 0x80) != 0 || len + 2 > *remaining) {
        return -1;
    }
    *value = p + 2;
    *value_length = len;
    *cursor += 2 + len;
    *remaining -= 2 + len;
    return 0;
}

int nd_dlms_apdu_decode_aarq(const uint8_t *buffer, size_t length,
                             nd_dlms_aarq_t *aarq)
{
    if (buffer == NULL || aarq == NULL || length < 2 || buffer[0] != 0x60) {
        return -1;
    }
    size_t content_length = buffer[1];
    if ((content_length & 0x80) != 0 || content_length + 2 != length) {
        return -1;
    }

    memset(aarq, 0, sizeof(*aarq));
    const uint8_t *cursor = buffer + 2;
    size_t remaining = content_length;
    int have_context = 0;
    int have_auth_value = 0;

    while (remaining > 0) {
        uint8_t tag;
        const uint8_t *value;
        size_t value_length;
        if (read_tlv(&cursor, &remaining, &tag, &value, &value_length) != 0) {
            return -1;
        }
        switch (tag) {
        case 0xA1:
            if (value_length != 9 || value[0] != 0x06 || value[1] != 7
                || memcmp(&value[2], app_context_ln, sizeof(app_context_ln))
                       != 0) {
                return -1;
            }
            have_context = 1;
            break;
        case 0x8A:
            break;
        case 0x8B: {
            if (value_length != 7 || value[0] != 0x60 || value[1] != 0x85
                || value[2] != 0x74 || value[3] != 0x05
                || value[4] != 0x08 || value[5] != 0x02) {
                return -1;
            }
            if (value[6] == 1) {
                aarq->auth = ND_DLMS_AUTH_LLS;
            } else if (value[6] == 5) {
                aarq->auth = ND_DLMS_AUTH_HLS_GMAC;
            } else if (value[6] == 6) {
                aarq->auth = ND_DLMS_AUTH_HLS_SHA256;
            } else if (value[6] == 7) {
                aarq->auth = ND_DLMS_AUTH_HLS_ECDSA;
            } else {
                return -1;
            }
            break;
        }
        case 0xAC:
            if (value_length != 10 || value[0] != 0x80 || value[1] != 8) {
                return -1;
            }
            memcpy(aarq->auth_value, &value[2], 8);
            aarq->auth_value_len = 8;
            have_auth_value = 1;
            break;
        case 0xBE:
            if (value_length >= 16 && value[0] == 0x04 && value[1] == 0x0E) {
                aarq->max_rx_pdu =
                    (uint16_t)(((uint16_t)value[14] << 8) | value[15]);
                aarq->max_tx_pdu = aarq->max_rx_pdu;
            }
            break;
        default:
            break;
        }
    }

    if (!have_context) {
        return -1;
    }
    if (aarq->auth != ND_DLMS_AUTH_NONE && !have_auth_value) {
        return -1;
    }
    return 0;
}

int nd_dlms_apdu_encode_aare(const nd_dlms_aare_t *aare, uint8_t *buffer,
                             size_t capacity, size_t *length)
{
    if (aare == NULL || buffer == NULL || length == NULL
        || aare->result > 1 || aare->diagnostic > 0x0E) {
        return -1;
    }

    uint8_t content[128];
    size_t used = 0;
    const uint8_t app_context[] = {0xA1, 0x09, 0x06, 0x07, 0x60, 0x85,
                                   0x74, 0x05, 0x08, 0x01, 0x01};
    const uint8_t result[] = {0xA2, 0x03, 0x02, 0x01, aare->result};
    const uint8_t diagnostic[] = {0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01,
                                  aare->diagnostic};
    if (append(content, sizeof(content), &used, app_context,
               sizeof(app_context)) != 0
        || append(content, sizeof(content), &used, result, sizeof(result)) != 0
        || append(content, sizeof(content), &used, diagnostic,
                  sizeof(diagnostic)) != 0) {
        return -1;
    }

    if (aare->auth != ND_DLMS_AUTH_NONE) {
        int mechanism = auth_mechanism(aare->auth);
        if (mechanism < 0 || aare->auth_value_len != 8) {
            return -1;
        }
        const uint8_t acse[] = {0x88, 0x02, 0x07, 0x80};
        const uint8_t mechanism_oid[] = {0x89, 0x07, 0x60, 0x85,
                                         0x74, 0x05, 0x08, 0x02,
                                         (uint8_t)mechanism};
        const uint8_t auth_tag[] = {0xAA, 0x0A, 0x80, 0x08};
        if (append(content, sizeof(content), &used, acse, sizeof(acse)) != 0
            || append(content, sizeof(content), &used, mechanism_oid,
                      sizeof(mechanism_oid)) != 0
            || append(content, sizeof(content), &used, auth_tag,
                      sizeof(auth_tag)) != 0
            || append(content, sizeof(content), &used, aare->auth_value, 8)
                   != 0) {
            return -1;
        }
    }

    const uint8_t user_info[] = {0xBE, 0x10, 0x04, 0x0E, 0x08, 0x00, 0x06,
                                 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x50, 0x1F,
                                 0x01, 0xF4, 0x00, 0x07};
    if (append(content, sizeof(content), &used, user_info,
               sizeof(user_info)) != 0
        || used > 127) {
        return -1;
    }

    size_t total = 0;
    if (append_byte(buffer, capacity, &total, 0x61) != 0
        || append_byte(buffer, capacity, &total, (uint8_t)used) != 0
        || append(buffer, capacity, &total, content, used) != 0) {
        return -1;
    }
    *length = total;
    return 0;
}

int nd_dlms_apdu_decode_aare(const uint8_t *buffer, size_t length,
                             nd_dlms_aare_t *aare)
{
    if (buffer == NULL || aare == NULL || length < 2 || buffer[0] != 0x61) {
        return -1;
    }
    size_t content_length = buffer[1];
    if ((content_length & 0x80) != 0 || content_length + 2 != length) {
        return -1;
    }

    memset(aare, 0, sizeof(*aare));
    const uint8_t *cursor = buffer + 2;
    size_t remaining = content_length;
    int have_context = 0;
    int have_result = 0;
    int have_diagnostic = 0;
    int have_auth_value = 0;

    while (remaining > 0) {
        uint8_t tag;
        const uint8_t *value;
        size_t value_length;
        if (read_tlv(&cursor, &remaining, &tag, &value, &value_length) != 0) {
            return -1;
        }
        switch (tag) {
        case 0xA1:
            if (value_length != 9 || value[0] != 0x06 || value[1] != 7
                || memcmp(&value[2], app_context_ln, sizeof(app_context_ln))
                       != 0) {
                return -1;
            }
            have_context = 1;
            break;
        case 0xA2:
            if (value_length != 3 || value[0] != 0x02 || value[1] != 1) {
                return -1;
            }
            aare->result = value[2];
            have_result = 1;
            break;
        case 0xA3:
            if (value_length != 5 || value[0] != 0xA1 || value[1] != 3
                || value[2] != 0x02 || value[3] != 1) {
                return -1;
            }
            aare->diagnostic = value[4];
            have_diagnostic = 1;
            break;
        case 0x88:
            break;
        case 0x89:
            if (value_length != 7 || value[0] != 0x60 || value[1] != 0x85
                || value[2] != 0x74 || value[3] != 0x05
                || value[4] != 0x08 || value[5] != 0x02) {
                return -1;
            }
            if (value[6] == 5) {
                aare->auth = ND_DLMS_AUTH_HLS_GMAC;
            } else if (value[6] == 6) {
                aare->auth = ND_DLMS_AUTH_HLS_SHA256;
            } else if (value[6] == 7) {
                aare->auth = ND_DLMS_AUTH_HLS_ECDSA;
            } else {
                return -1;
            }
            break;
        case 0xAA:
            if (value_length != 10 || value[0] != 0x80 || value[1] != 8) {
                return -1;
            }
            memcpy(aare->auth_value, &value[2], 8);
            aare->auth_value_len = 8;
            have_auth_value = 1;
            break;
        case 0xBE:
            break;
        default:
            break;
        }
    }

    if (!have_context || !have_result || !have_diagnostic) {
        return -1;
    }
    if (aare->auth != ND_DLMS_AUTH_NONE && !have_auth_value) {
        return -1;
    }
    return 0;
}
