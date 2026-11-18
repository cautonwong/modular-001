#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#include "application/adapters/dl698/meter_base_view.h"
#include "port.h"
#include "fr_645_report.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "targets/stategrid_meter/mapper_698/resource_map.h"

int SET_2_Variable(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    uint16_t oad;
    uint8_t attribute;
    uint8_t* p = buf;
    uint32_t s_index = 0;

    
    if(!nd_mb_valid(instance))
    {
        return -1;
    }

    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
//    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    switch (oad)
    {
        case 0x2015:// 跟随上报模式字
            if(attribute == 0x04)
            {
                if ((p[0] != TYPE_bit_string) || (p[1] != 32))
                {
                    return -DAR_TYPE_NOT_MATCH;
                }   
                p += 2;
                mem_swap_cpy((uint8_t*)&nd_mb_report_2015_mode(instance), p,  4); 
                p += 4;

                s_index = ((uint32_t)(DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                nd_mb_offset_report_2015mode());
                nd_set_offset(instance, s_index, (uint8_t*)&nd_mb_report_2015_mode(instance), 4);

                report_2015mode_set_from_698(instance);

                EventOut_698_check(instance);
                return (p-buf);
            }
            else if(attribute == 0x05) //上报方式
            {
                return fr_set_event_report_flag_mode(instance, 0x20150200, p, 1);
            }
            break;
       
        default:
            break;
    }

    return -DAR_RW_REJECT;
}


