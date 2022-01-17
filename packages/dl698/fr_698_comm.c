#include "nidavellir.h"
#include "applications.h"
#include "fr_axdr.h"
#include "fr_698.h"


#ifdef WIN32_U_TEST

int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size,ctx, RESOURCE_MAP_GET);
}

int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size, ctx, RESOURCE_MAP_SET);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size, ctx, RESOURCE_MAP_ACTION);
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