#ifndef ND_APPLICATION_ADAPTERS_DL698_LEGACY_MESSAGE_H
#define ND_APPLICATION_ADAPTERS_DL698_LEGACY_MESSAGE_H

#include <stdint.h>

/* Wire-layout copy of the legacy kernel message, packed identically.
 * Target code uses this type so it does not include the kernel header;
 * callers that still take struct message_t * accept it by layout. */
#pragma pack(push, 1)
typedef struct nd_message_t
{
    uint32_t magic;
    uint8_t from;
    uint8_t to;
    uint8_t ctrl;
    uint8_t qos;
    uint32_t id;
    uint16_t len;
    uint8_t oad_n;
    uint8_t x;
#ifdef WIN32
    int32_t err_no;
#endif
    uint8_t *payload;
    uint16_t check;
} nd_message_t;
#pragma pack(pop)

#endif
