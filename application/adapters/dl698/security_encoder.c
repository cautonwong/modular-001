#include "application/adapters/dl698/security_encoder.h"

int nd_security_encode_tail(uint8_t *buffer, size_t capacity, size_t offset,
                            const uint8_t *mac, uint8_t mac_len,
                            size_t *new_offset)
{
    if ((buffer == NULL) || (new_offset == NULL)) {
        return -1;
    }

    if (mac_len == 4u) {
        if ((mac == NULL) || (capacity - offset < 7u)) {
            return -1;
        }
        buffer[offset++] = 0x01;
        buffer[offset++] = 0x00;
        buffer[offset++] = 0x04;
        buffer[offset++] = mac[0];
        buffer[offset++] = mac[1];
        buffer[offset++] = mac[2];
        buffer[offset++] = mac[3];
        *new_offset = offset;
        return 0;
    }

    if (mac_len == 0u) {
        if (capacity - offset < 1u) {
            return -1;
        }
        buffer[offset++] = 0x00;
        *new_offset = offset;
        return 0;
    }
    return -1;
}
