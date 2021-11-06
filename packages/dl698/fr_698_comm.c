#include "nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"


int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    const struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_GET, &obj);
}

int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    const struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_SET, &obj);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    const struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    return resource_map_698(instance, id, buf, RESOURCE_MAP_698_ACTION, &obj);
}
