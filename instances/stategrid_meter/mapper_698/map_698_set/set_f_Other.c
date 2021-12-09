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
    
    if((oad != 0xF300) && (oad != 0xF301))  return -1;
    
    switch (attribute)
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
                if(oad == 0xF300)
                    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AutoDisplayList);
                else
                    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList);
                nd_set_index(instance, index, &displayid, 2);
                break;
          case 0x04://显示参数
                switch(index)
                {
                    case 0x00:
                        p = p + 3;
                        if(oad == 0xF300)
                            mbase_cb->lcd_autodis_list_num = *p;
                        else
                            mbase_cb->lcd_keydis_list_num = *p;
                        p = p + 2;
                        if(oad == 0xF300)
                            mbase_cb->lcd_autodis_list_maxnum = *p;
                        else
                            mbase_cb->lcd_keydis_list_maxnum = *p;
                        break;
                    case 0x01://当前总对象数
                        p = p + 1;
                        if(oad == 0xF300)
                            mbase_cb->lcd_autodis_list_num = *p;
                        else
                            mbase_cb->lcd_keydis_list_num = *p;
                        break;
                    case 0x02://允许最大对象数
                        p = p + 1;
                        if(oad == 0xF300)
                            mbase_cb->lcd_autodis_list_maxnum = *p;
                        else
                            mbase_cb->lcd_keydis_list_maxnum = *p;
                        break;
                    default:
                        return -1;
                        break;
                }
                index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DisplayPara);
                nd_set_index(instance, index, (unsigned char*)&mbase_cb->backlight_key_time, 22);
                break;
          case 0x03://每个对象显示时间
                if((index == 0x00) || (index == 0x01))
                {
                    p = p + 1;
                    if(oad == 0xF300)
                        mem_swap_cpy((unsigned char*)&mbase_cb->lcd_autodis_keep_sec, p, 2);
                    else
                        mem_swap_cpy((unsigned char*)&mbase_cb->lcd_keydis_keep_sec, p, 2);
                    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DisplayPara);
                    nd_set_index(instance, index, (unsigned char*)&mbase_cb->backlight_key_time, 22);
                }
                else
                {
                    return -1;
                }
                break;
          default:
                return -1;
                break;
    }
    return 0x01;
}
