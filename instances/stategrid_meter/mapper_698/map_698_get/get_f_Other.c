#include "nidavellir.h"
#include "fr_698.h"



int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,displayid;
    uint8_t index,attribute,i,len,dispalyseque,maxnum;
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
            if(oad == 0xF300)
                maxnum = mbase_cb->lcd_autodis_list_num;
            else if(oad == 0xF301)
                maxnum = mbase_cb->lcd_keydis_list_num;
            if(index > maxnum)  return -1;
            
            if(index != 0)
            {
            
            }
            else
            {
                
            }
            break;
        case 0x03://显示时间
            if(index != 0)  return -1;
            *p++ = TYPE_long_unsigned;
            if(oad == 0xF300)
                mem_swap_cpy(p, (uint8_t*)&mbase_cb->lcd_autodis_keep_sec, 2);
            else
                mem_swap_cpy(p, (uint8_t*)&mbase_cb->lcd_keydis_keep_sec, 2);
            p = p + 2;
            return (p - buf);
            break;
        case 0x04://显示参数
            switch(index)
            {
                case 0x00:
                    *p++ = TYPE_structure;
                    *p++ = 2;
                    *p++ = TYPE_unsigned;
                    if(oad == 0xF300)
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_autodis_list_num, 1);
                    else
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_keydis_list_num, 1);
                    p++;
                    *p++ = TYPE_unsigned;
                    if(oad == 0xF300)
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_autodis_list_maxnum, 1);
                    else
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_keydis_list_maxnum, 1);
                    p = p + 1;
                    return (p - buf);
                    break;
                case 0x01://当前对象数
                    *p++ = TYPE_unsigned;
                    if(oad == 0xF300)
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_autodis_list_num, 1);
                    else
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_keydis_list_num, 1);
                    p = p + 1;
                    return (p - buf);
                    break;
                case 0x02://最大对象数
                    *p++ = TYPE_unsigned;
                    if(oad == 0xF300)
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_autodis_list_maxnum, 1);
                    else
                        memcpy(p, (uint8_t*)&mbase_cb->lcd_keydis_list_maxnum, 1);
                    p = p + 1;
                    return (p - buf);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return 0x01;
}
