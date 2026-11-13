#ifndef ND_DLMS_APDU_H
#define ND_DLMS_APDU_H

#include <stddef.h>
#include <stdint.h>

#include "components/protocol/dlms/include/nd_dlms_assoc.h"

typedef struct {
    uint8_t result;
    uint8_t diagnostic;
    uint8_t auth;
    uint8_t auth_value[8];
    uint8_t auth_value_len;
} nd_dlms_aare_t;

int nd_dlms_apdu_encode_aarq(const nd_dlms_aarq_t *aarq, uint8_t *buffer,
                             size_t capacity, size_t *length);
int nd_dlms_apdu_decode_aarq(const uint8_t *buffer, size_t length,
                             nd_dlms_aarq_t *aarq);
int nd_dlms_apdu_encode_aare(const nd_dlms_aare_t *aare, uint8_t *buffer,
                             size_t capacity, size_t *length);
int nd_dlms_apdu_decode_aare(const uint8_t *buffer, size_t length,
                             nd_dlms_aare_t *aare);

#endif
