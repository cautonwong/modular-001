#include "nidavellir.h"
#include "fr_698.h"

extern uint8_t read_esam_info(uint8_t *esam_info, uint8_t index);


int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,displayid;
    uint8_t index,attribute,i,len,dispalyseque,maxnum;
    uint8_t* p = buf;
    uint32_t displayoad1,displayoad2;
    uint8_t esam_inf[45]; 
    
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
        case 0xF300://循环显示
        case 0xF301://按键显示
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
            break;
        case 0xF100://ESAM信息
            switch (attribute)
            {
                case 0x02://ESAM序列号
                    *p++ = TYPE_octet_string;
                    *p++ = 8;
                    read_esam_info(esam_inf, 0x02);
                    memcpy(p, (uint8_t*)&esam_inf[0], 8);
                    p = p + 8;
                    return (p - buf);
                    break;
                case 0x03://版本号
                    *p++ = TYPE_octet_string;
                    *p++ = 5;
                    read_esam_info(esam_inf,0x01);
                    memcpy(p, (uint8_t*)&esam_inf[0], 5);
                    p = p + 5;
                    return (p - buf);
                    break;
                case 0x04://对称秘钥版本
                    *p++ = TYPE_octet_string;
                    *p++ = 16;
                    read_esam_info(esam_inf,0X04);
                    memcpy(p, (uint8_t*)&esam_inf[0], 16);
                    p = p + 16;
                    return (p - buf);
                    break;
                case 0x05://会话时效门限
                    *p++ = TYPE_double_long_unsigned;
                    //*p++ = 4;
                    read_esam_info(esam_inf,0X05);
                    memcpy(p, (uint8_t*)&esam_inf[0], 4);
                    p = p + 4;
                    return (p - buf);
                    break;
                case 0x06://会话时效剩余时间
                    *p++ = TYPE_double_long_unsigned;
                    //*p++ = 4;
                    read_esam_info(esam_inf,0X05);
                    memcpy(p, (uint8_t*)&esam_inf[4], 4);
                    p = p + 4;
                    return (p - buf);
                    break;
                case 0x07://当前计数器
                    *p++ = TYPE_structure;
                    *p++ = 3;
                    read_esam_info(esam_inf,0X06);
                    for(i=0;i<3;i++)
                    {
                        *p++ = TYPE_double_long_unsigned;
                        memcpy(p, (uint8_t*)&esam_inf[0], 4);
                        p = p + 4;
                    }
                    return (p - buf);
                    break;
            }
            break;
    }
    return 0x01;
}
