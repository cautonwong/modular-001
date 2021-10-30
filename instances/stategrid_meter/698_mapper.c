#include "nidavellir.h"

int8_t resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj)
{
    struct mbase_control_block_t* mbase_cb = instance->mbase_cb;
    switch (id)
    {
    case 0x20000200:
    {
        obj->block_name = 0x0A;//只是举例
        obj->device_name = 0x0D;//只是举例
        //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
        switch (mode)
        {
        case 0x01://读数据
            //1、直接读缓存的值
         //   memcpy(buf, (uint8_t*)mbase_cb->UA, 0x04);
            return 0x01;
            //2、需要读EPROM
            return 0x02;
            break;
        case 0x02://写数据
            break;
        }
        break;
    }
    default:
        break;
    }
    return 0x01;
}

//职责链模式  过滤器
