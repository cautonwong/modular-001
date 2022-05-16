#include "nidavellir.h"
#include "fr_698.h"
#include "port.h"

int SET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute;
    uint8_t* p = buf;


    
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
        case 0x2015:// 跟随上报模式字
            if(attribute == 0x04)
            {
                if ((p[0] != TYPE_bit_string) || (p[1] != 32))
                {
                    return -DAR_TYPE_NOT_MATCH;
                }   
                p += 2;
                mem_swap_cpy((uint8_t*)&mbase_cb->report_2015mode, p,  4); //待调整
                p += 4;
                EventOut_698_check(instance);
                return (p-buf);
            }
            else if(attribute == 0x05) //上报方式
            {
                return set_event_report_flag_mode(instance, 0x20150200, p, 1);
            }
            break;
       
        default:
            break;
    }

    return -1;
}


