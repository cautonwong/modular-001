#ifndef ND_APPLICATION_ADAPTERS_DLMS_MESSAGE_H
#define ND_APPLICATION_ADAPTERS_DLMS_MESSAGE_H

#include <stdint.h>

/*
 * DLMS only needs a length and control byte in its internal handlers. The
 * kernel message type is not part of this adapter.
 */
typedef struct nd_dlms_msg {
    uint16_t len;
    uint8_t ctrl;
} nd_dlms_msg_t;

#endif
