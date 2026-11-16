#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#include "application/adapters/dl698/meter_base_view.h"
#include "port.h"
#include "infra/kernel/resource_blocks.h"
#include "targets/stategrid_meter/mapper_698/resource_map.h"


int SET_8_Control(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    uint16_t oad;
    uint8_t index,attribute;
    uint8_t* p = buf;
    uint32_t addr=0;
    
    if(!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    
/*    if(general_set_4_para(instance, id, buf) != -1)
    {
        return 1;
    }*/
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    switch (oad)
    {
        case 0x8000://遥控
            if(attribute == 0x02)
            {
                switch(index)
                {
                    case 0x00:
                        p = p + 3;
                        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                        nd_set_index(instance, addr, p, 4); 
                        mem_swap_cpy((uint8_t *)&nd_mb_i_prot(instance), p, 4);
                        p = p + 4;
                        p++;
                        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                        nd_set_index(instance, addr, p, 2); 
                        mem_swap_cpy((uint8_t *)&nd_mb_prot_time(instance), p, 2);
                        if(nd_mb_imaxmin(instance) > 0)   //过流中修改时间，刷新
                            nd_mb_imaxmin(instance) = nd_mb_prot_time(instance)*60;
                        p = p + 2;
                        return (p-buf);
                    case 0x01:
                        p++;
                        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
                        nd_set_index(instance, addr, p, 4); 
                        mem_swap_cpy((uint8_t *)&nd_mb_i_prot(instance), p, 4);
                        p = p + 4;
                        return (p-buf);
                    case 0x02:  
                        p++;
                        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
                        nd_set_index(instance, addr, p, 2); 
                        mem_swap_cpy((uint8_t *)&nd_mb_prot_time(instance), p, 2);
                        if(nd_mb_imaxmin(instance) > 0)   //过流中修改时间，刷新
                            nd_mb_imaxmin(instance) = nd_mb_prot_time(instance)*60;
                        p = p + 2;
                        return (p-buf);
                    default:
                      break;
                }
            }
            break;
        case 0x8001://保电
            if(attribute == 0x02)
            {
                return -DAR_RW_REJECT;
            }
            break;
        default:
            break;
    }

    return -DAR_RW_REJECT;
}
