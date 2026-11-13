#ifndef ND_APPLICATION_ADAPTERS_DL645_DATA_H
#define ND_APPLICATION_ADAPTERS_DL645_DATA_H

#include <stdint.h>

#include "fr_645_message.h"

void fr_645_parse_91_data(const uint8_t* data, uint16_t length,
                          nd_dl645_msg_t* mesg, uint8_t to);

#endif
