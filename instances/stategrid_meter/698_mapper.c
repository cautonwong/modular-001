#include "nidavellir.h"
#include "fr_698.h"

int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj)
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
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_long_unsigned;
            memcpy(buf+3, (uint8_t*)mbase_cb->Ua, 0x02);
            //buf[3] = 0x11;
            //buf[4] = 0x22;
            return 0x05;
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
