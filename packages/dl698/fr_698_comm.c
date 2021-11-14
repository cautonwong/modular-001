#include "nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"

extern int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);


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
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_SET, &obj);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t obj;
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_ACTION, &obj);
}
