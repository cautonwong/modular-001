#ifndef ND_TARGET_STATEGRID_MESSAGE_H
#define ND_TARGET_STATEGRID_MESSAGE_H

#include <stdint.h>

/* Wire-layout copy of the retired kernel message. The product callbacks still
 * accept this pointer; its fields are copied into nd_dl645/nd_dl698 messages. */
#pragma pack(push, 1)
struct message_t
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
    uint8_t *payload;
    uint16_t check;
};
#pragma pack(pop)

#endif
