#include "nidavellir.h"
#include "fr_698.h"

extern uint8_t read_esam_no(uint8_t *esam_info);

int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute,i,temp[10];
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
                uint8_t tag[] = {TYPE_unsigned, TYPE_long_unsigned, TYPE_long_unsigned, TYPE_long_unsigned, TYPE_unsigned, TYPE_unsigned, TYPE_unsigned};
                if(index == 0)
                {
                    *p++ = TYPE_structure;
                    *p++ = 7;
                    for(i=0; i<7; i++)
                    {
                        *p++ = tag[i];
                        mem_swap_cpy(p, (uint8_t*)idbuf[i], lens[i]);
                        p = p + lens[i];
                    }
                    return (p - buf);
                }
                else if(index < 8)
                {
                    *p++ = tag[index-1];
                    mem_swap_cpy(p, (uint8_t*)idbuf[index-1], lens[index-1]);
                    p = p + lens[index-1];
                    return (p - buf);
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
        case 0x4001://表地址
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                memset(p, 0x11, 6); //待修改
                //nd_get(instance, 0x40010200, p, 6);
                //mem_swap_cpy(p, temp, 6);
                p = p + 6;
                return (p - buf);
            }
            else
            {
                return -1;
            }
            break;
        case 0x4002://表号
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                read_esam_no(temp);
                mem_swap_cpy(p, temp, 6);
                p = p + 6;
                return (p - buf);
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
