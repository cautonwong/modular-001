#include "nidavellir.h"
#include "applications.h"
#include "fr_axdr.h"
#include "fr_698.h"


#ifdef WIN32_U_TEST
int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t obj;
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_GET, &obj);
}

int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t obj;
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_SET, &obj);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t obj;
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_ACTION, &obj);
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