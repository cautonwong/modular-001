#include "nidavellir.h"
#include "fr_698.h"


int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x00000200://电压
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_long_unsigned;
            //memcpy(buf+3, (uint8_t*)mbase_cb->voltage_ac, 0x02);
            return 0x05;
        }
    break;
    default:
        break;
    }
    return 0x01;
}
