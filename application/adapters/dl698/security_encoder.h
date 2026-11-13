#ifndef ND_APPLICATION_DL698_SECURITY_ENCODER_H
#define ND_APPLICATION_DL698_SECURITY_ENCODER_H

#include <stddef.h>
#include <stdint.h>

int nd_security_encode_tail(uint8_t *buffer, size_t capacity, size_t offset,
                            const uint8_t *mac, uint8_t mac_len,
                            size_t *new_offset);

#endif
