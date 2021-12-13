#include "nidavellir.h"
#include "fr_698.h"



int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute,i;
    uint8_t* p = buf;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    switch (oad)
    {
        case 0x4007://LCD参数
          if(attribute == 0x02)
          {
                uint32_t idbuf[] = {
                (uint32_t)&mbase_cb->lcd_poweron_all_dis_time,            //上电全显时间
                (uint32_t)&mbase_cb->backlight_key_time,                  //背光点亮时间
                (uint32_t)&mbase_cb->lcd_checkdis_keep_sec,               //显示查看背光点亮时间
                (uint32_t)&mbase_cb->lcd_nopower_key_keep_sec,            //无电按键驻留时间
                (uint32_t)&mbase_cb->energy_point,                        //电能小数显示位
                (uint32_t)&mbase_cb->power_point,                         //功率小数显示位
                (uint32_t)&mbase_cb->lcd_12_mean,                         //液晶12字样意义
                };
                uint8_t lens[] = {1, 2, 2, 2, 1, 1, 1};
                if(index == 0)
                {
                    p = p + 2;
                    for(i=0; i<8; i++)
                    {
                        p++;
                        mem_swap_cpy((uint8_t*)idbuf[i], p, lens[i]);
                        p = p + lens[i];
                    }
                    return (p-buf);
                }
                else if(index < 8)
                {
                    p++;
                    mem_swap_cpy((uint8_t*)idbuf[index-1], p, lens[index-1]);
                    p = p + lens[i];
                    return (p-buf);
                }
                else
                {
                    return -1;
                }
          }
          else
          {
                return -1;
          }
          break;
        default:
          break;
    }
    return 0x01;
}
