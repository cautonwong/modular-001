#ifndef ND_INFRA_STORAGE_CRC32_H
#define ND_INFRA_STORAGE_CRC32_H

#include <stddef.h>
#include <stdint.h>

#define ND_CRC32_INIT 0xFFFFFFFFu

/* Bitwise CRC-32 (reflected 0xEDB88320). Records are small and infrequent,
 * so the table and its ROM are not earned. */
static inline uint32_t nd_crc32_update(uint32_t crc, const uint8_t *data,
                                       size_t length)
{
    uint32_t value = crc;

    for (size_t index = 0; index < length; index++) {
        value ^= data[index];
        for (unsigned bit = 0; bit < 8u; bit++) {
            value = (value >> 1) ^ (0xEDB88320u & (0u - (value & 1u)));
        }
    }
    return value;
}

static inline uint32_t nd_crc32_finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}

#endif
