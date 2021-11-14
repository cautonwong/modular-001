#include "nidavellir.h"
#include "fr_698.h"


int resource_map_698_OIA1_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb = instance->mbase_cb;
    
    switch (id)
    {
    case 0x20000200://电压
        {
            //obj->block_name = 0x0A;//只是举例
            //obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_long_unsigned;
            //memcpy(buf+3, (uint8_t*)mbase_cb->voltage_ac, 0x02);
            return 0x05;
        }
    break;
    case 0x20010200://电流  current_l current_n
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_double_long;
            memcpy(buf+3, (uint8_t*)mbase_cb->current_l, 0x04);
            return 0x07;
        }
    break;
    case 0x20040200://active_power 
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = 0x01;//数组
            buf[1] = 0x02;//总 分
            buf[2] = TYPE_double_long;
            memcpy(buf+3, (uint8_t*)mbase_cb->active_power, 0x04);
            buf[7] = TYPE_double_long;
            memcpy(buf+8, (uint8_t*)mbase_cb->active_power, 0x04);
            return 12;
        }
    break;
    case 0x200F0200://frequency
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = TYPE_long_unsigned;//数组
            memcpy(buf+1, (uint8_t*)mbase_cb->frequency, 0x02);
            return 0x03;
        }
    break;
    case 0x200A0200://power_factor
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = 0x01;//数组
            buf[1] = 0x02;//总分
            buf[2] = TYPE_long;
            memcpy(buf+3, (uint8_t*)mbase_cb->power_factor, 0x02);
            buf[5] = TYPE_long;
            memcpy(buf+6, (uint8_t*)mbase_cb->power_factor, 0x02);
            return 8;
        }
    break;
    case 0x20100200://temperature
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = TYPE_long;
            memcpy(buf+1, (uint8_t*)mbase_cb->temperature, 0x02);
            return 3;
        }
    break;
    case 0x20110200://voltage_battery
        {
            obj->block_name = 0x0A;//只是举例
            obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf[0] = TYPE_long;
            memcpy(buf+1, (uint8_t*)mbase_cb->voltage_battery, 0x02);
            return 3;
        }
    break;



    default:
        break;
    }
    return 0x01;
}
