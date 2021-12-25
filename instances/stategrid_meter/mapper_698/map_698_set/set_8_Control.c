#include "nidavellir.h"
#include "fr_698.h"
#include "port.h"


int SET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute,i,len=0,num,buffer[42]={0},num_hourtab=0,j;
    uint8_t* p = buf;
    uint32_t s_index = 0x00,addr=0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    if(general_set_4_para(instance, id, buf) != -1)
    {
        return 1;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    switch (oad)
    {
        case 0x8000://遥控
            if(attribute == 0x02)
            {
                switch(index)
                {
                    case 0x00:
                        p = p + 3;
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                        nd_set_index(instance, addr, p, 4); 
                        p = p + 4;
                        p++;
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                        nd_set_index(instance, addr, p, 2); 
                        p = p + 2;
                        return (p-buf);
                    case 0x01:
                        p++;
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                        nd_set_index(instance, addr, p, 4); 
                        p = p + 4;
                        return (p-buf);
                    case 0x02:  
                        p++;
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                        nd_set_index(instance, addr, p, 2); 
                        p = p + 2;
                        return (p-buf);
                    default:
                      break;
                }
            }
            break;
        default:
            break;
    }

    return -1;
}
