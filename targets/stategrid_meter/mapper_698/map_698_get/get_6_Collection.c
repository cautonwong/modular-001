#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"



int GET_6_Collection(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    
    if(!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    switch (id)
    {
    case 0x60000200://电压
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_long_unsigned;
            //memcpy(buf+3, (uint8_t*)nd_mbase_voltage_ac(mbase_cb), 0x02);
            return 0x05;
        }
    break;

    default:
        break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
