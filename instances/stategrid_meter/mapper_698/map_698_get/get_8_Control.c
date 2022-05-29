#include "nidavellir.h"
#include "fr_698.h"

#define		F_RemoteRelayOn  	    0x01	//保电

int GET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute;
    uint8_t* p=buf;
    uint32_t addr = 0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    switch (oad)
    {
        case 0x8000://遥控
            switch(attribute)
            {
                case 0x02://配置参数
                    switch(index)
                    {
                        case 0x00:
                            *p++ = TYPE_structure;
                            *p++ = 2;
                            *p++ = TYPE_double_long_unsigned;
                            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                            nd_get_index(instance, addr, p, 4); 
                            p = p + 4;
                            *p++ = TYPE_long_unsigned;
                            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                            nd_get_index(instance, addr, p, 2); 
                            p = p + 2;
                            return (p-buf);
                            break;
                        case 0x01:
                            *p++ = TYPE_double_long_unsigned;
                            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                            nd_get_index(instance, addr, p, 4); 
                            p = p + 4;
                            return (p-buf);
                        case 0x02:
                            *p++ = TYPE_long_unsigned;
                            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                            nd_get_index(instance, addr, p, 2); 
                            p = p + 2;
                            return (p-buf);
                        default:
                            break;
                    }
                case 0x04://告警状态
                    *p++ = TYPE_bit_string;
                    *p++ = 8;
                    *p++ = 00;
                    return (p-buf);
                case 0x05://命令状态
                    *p++ = TYPE_bit_string;
                    *p++ = 8;
                    *p++ = mbase_cb->sta;
                    return (p-buf);
                default:
                    return -DAR_OBJECT_NOT_EXIST;
                    break;
            }
        case 0x8001://保电
            if((attribute != 0x02) || (index != 0x00))  return -1;
            *p++ = TYPE_enum;
            if(mbase_cb->sta & F_RemoteRelayOn)//保电
                *p++ = 1;
            else
                *p++ = 0;
            break;
        default:
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
