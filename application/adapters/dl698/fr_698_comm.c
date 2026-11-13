#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_axdr.h"
#include "fr_698.h"
#include "application/adapters/dl698/fr_698.h"
#include "dl698_time.h"


#ifdef WIN32_U_TEST

int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_get(instance, id, buf, buf_size, ctx);
}

int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_set(instance, id, buf, buf_size, ctx);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_action(instance, id, buf, buf_size, ctx);
}

int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}

int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}

#endif



int check_oad(uint32_t oad)
{
    uint16_t val = 0xF000&(oad>>16);
    if((val >= 0x8000) && (0xF000 != val)){
        return -DAR_OTHERS_ERROR;
    }
    return 0;
}
