#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "fr_axdr.h"
#include "fr_698.h"
#include "application/adapters/dl698/fr_698.h"
#include "dl698_time.h"


#ifdef WIN32_U_TEST

int __nd_get(void * instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_get(instance, id, buf, buf_size, ctx);
}

int __nd_set(void * instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_set(instance, id, buf, buf_size, ctx);
}

int __nd_action(void * instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return nd_action(instance, id, buf, buf_size, ctx);
}

int16_t nd_get_index(void *ctx, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    (void)ctx;
    (void)index;
    (void)buf;
    (void)buf_size;
    return 0;
}

int16_t nd_set_index(void *ctx, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    (void)ctx;
    (void)index;
    (void)buf;
    (void)buf_size;
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
