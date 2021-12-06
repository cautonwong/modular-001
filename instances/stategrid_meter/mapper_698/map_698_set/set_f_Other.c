#include "nidavellir.h"
#include "fr_698.h"



int SET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,displayid;
    uint8_t index,attribute,i,len,dispalyseque;
    uint8_t* p = buf;
    uint32_t displayoad1,displayoad2;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);    //属性
    
    switch (oad)
    {
        case 0xF300:    //循显
          switch(attribute)
          {
              case 0x02://显示对象列表
                p = p + 5;
                if(*p++ == 0)
                  len = 4;
                else
                  len = 8;
                mem_swap_cpy((uint8_t*)&displayoad1, p, 4);
                if(len == 8)
                    mem_swap_cpy((uint8_t*)&displayoad2, p+5, 4);
                displayid = display_oad_to_id(displayoad1, len);
                break;
              case 0x03://显示参数
                break;
              case 0x04://每个对象显示时间
                break;
              default:
                break;
          }
          break;
        case 0xF301:    //键显
          break;
        default:
            break;
    }
    return 0x01;
}
