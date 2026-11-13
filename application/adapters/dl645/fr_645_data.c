#include "application/adapters/dl645/fr_645_data.h"

static uint32_t read_id_lsb(const uint8_t* data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8)
           | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

void fr_645_parse_91_data(const uint8_t* data, uint16_t length,
                          nd_dl645_msg_t* mesg, uint8_t to)
{
    mesg->to = to;
    mesg->ctrl = 0x91;
    mesg->id = read_id_lsb(data);
    mesg->len = (uint16_t)(length + 4);
    mesg->payload = (uint8_t*)(data + 4);
}
