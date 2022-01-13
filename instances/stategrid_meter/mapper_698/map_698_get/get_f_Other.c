#include "nidavellir.h"
#include "fr_698.h"

extern uint8_t read_esam_info(uint8_t *esam_info, uint8_t index);

const uint8_t name_rs485[5] = {'R', 'S', '4', '8', '5'};
const uint8_t name_ir[5] = {'I', 'N', 'F', 'R', 'A'};
const uint8_t name_plc[3] = {'P', 'L', 'C'};
const uint8_t name_relay[5] = {'R', 'E', 'L', 'A', 'Y'};

int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,displayid;
    uint8_t index,attribute,i,len,dispalyseque,maxnum;
    uint8_t* p = buf;
    uint32_t displayoad1,displayoad2,addr=0,s_index=0,atrtime=0;
    uint8_t esam_inf[45]; 
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);    //属性
   
    if((oad == 0xF201) || (oad == 0xF202) || (oad == 0xF209) || (oad == 0xF205) || (oad == 0xF207))
    {
        if(attribute == 0x03)//设备对象数量
        {
            *p++ = TYPE_unsigned;
            *p++ = 1;
            return (p-buf);
        }
    }
    
    switch (oad)
    {
        case 0xF201://rs485
        case 0xF202://红外 
        case 0xF209://载波模块
            if(attribute == 0x02)
            {
                if(index >1 )   return -1;
                if(index == 0x00)
                {
                    *p++ = TYPE_array;
                    *p++ = 1;
                }
                *p++ = TYPE_structure;
                *p++ = 3;
                *p++ = TYPE_visible_string;
                if(oad == 0xF201)
                {
                    *p++ = 5;
                    memcpy(p, name_rs485, 5);
                    p = p + 5;
                }
                else if(oad == 0xF202)
                {
                    *p++ = 5;
                    memcpy(p, name_ir, 5);
                    p = p + 5;
                }
                else
                {
                    *p++ = 3;
                    memcpy(p, name_plc, 3);
                    p = p + 3;
                }
                *p++ = TYPE_COMDCB;
                if(oad == 0xF201)   *p++ = mbase_cb->Baut_rs485;    //485波特率
                else if(oad == 0xF202)   *p++ = 2;    //红外波特率
                else if(oad == 0xF209)   *p++ = mbase_cb->Baut_plc;    //载波波特率
                *p++ = 0x02;    //偶效验
                *p++ = 0x08;    //8个数据位
                *p++ = 0x01;    //1个停止位
                *p++ = 0x00;    //无流控
                if(oad == 0xF209)
                {
                    *p++ = 0;
                }
                else
                {
                    *p++ = TYPE_enum;
                    *p++ = 0;
                }
            }
            else
                return -1;
            return (p-buf);
        case 0xF205://继电器输出
            if(attribute == 0x02)
            {
                *p++ = TYPE_array;
                *p++ = 1;
                *p++ = TYPE_structure;
                *p++ = 4;
                *p++ = TYPE_visible_string;
                *p++ = 5;
                memcpy(p, name_relay, 5);
                p = p + 5;
                *p++ = TYPE_enum;
                *p++ = mbase_cb->relay_true_sta;
                *p++ = TYPE_enum;
                *p++ = mbase_cb->relay_ctl_mode;
                *p++ = TYPE_enum;
                *p++ = 0;
            }
            else
                return -1;
            return (p-buf);
            break;
        case 0xF207://多功能端子
            if(attribute == 0x02)
            {
                *p++ = TYPE_array;
                *p++ = 1;
                *p++ = TYPE_enum;
                *p++ = mbase_cb->MulPortMode;
            }
            else
                return -1;
            return (p-buf);
        case 0xF300://循环显示
        case 0xF301://按键显示
            switch (attribute)
            {
                case 0x02://显示对象列表
                    if(oad == 0xF300)
                    {
                        maxnum = mbase_cb->lcd_autodis_list_num;
                        addr = AutoDisplayList;
                    }
                    else if(oad == 0xF301)
                    {
                        maxnum = mbase_cb->lcd_keydis_list_num;
                        addr = KeyDisplayList;
                    }
                    if(index > maxnum)  return -DAR_OBJECT_NOT_EXIST;
                    
                    if(index != 0)
                    {
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                        nd_get_index(instance, s_index, &displayid, 2);
                        len = display_id_to_oad( displayid, esam_inf );
                        if(len == 5)
                        {
                            *p++ = TYPE_structure;
                            *p++ = 2;
                            *p++ = TYPE_CSD;
                            *p++ = 0;
                            mem_swap_cpy(p, esam_inf, 4);
                            p = p + 4;
                            *p++ = TYPE_unsigned;
                            *p++ = esam_inf[4];
                        }
                        else if(len == 9)
                        {
                            *p++ = TYPE_structure;
                            *p++ = 2;
                            *p++ = TYPE_CSD;
                            *p++ = 1;
                            mem_swap_cpy(p, esam_inf, 4);
                            p = p + 4;
                            *p++ = 1;
                            mem_swap_cpy(p, &esam_inf[4], 4);
                            p = p + 4;
                            *p++ = TYPE_unsigned;
                            *p++ = esam_inf[8];
                        }
                        else
                            return -DAR_OTHERS_ERROR;
                    }
                    else
                    {
                        *p++ = TYPE_array;
                        *p++ = maxnum;
                        for(i=0;i<maxnum;i++)
                        {
                            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
                            nd_get_index(instance, s_index, &displayid, 2);
                            len = display_id_to_oad( displayid, esam_inf );
                            if(len == 5)
                            {
                                *p++ = TYPE_structure;
                                *p++ = 2;
                                *p++ = TYPE_CSD;
                                *p++ = 0;
                                mem_swap_cpy(p, esam_inf, 4);
                                p = p + 4;
                                *p++ = TYPE_unsigned;
                                *p++ = esam_inf[4];
                            }
                            else if(len == 9)
                            {
                                *p++ = TYPE_structure;
                                *p++ = 2;
                                *p++ = TYPE_CSD;
                                *p++ = 1;
                                mem_swap_cpy(p, esam_inf, 4);
                                p = p + 4;
                                *p++ = 1;
                                mem_swap_cpy(p, &esam_inf[4], 4);
                                p = p + 4;
                                *p++ = TYPE_unsigned;
                                *p++ = esam_inf[8];
                            }
                            else
                                return -DAR_OTHERS_ERROR;
                        }
                    }
                    return (p-buf);
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
                    read_esam_info(esam_inf,0X05);
                    memcpy(p, (uint8_t*)&esam_inf[0], 4);
                    p = p + 4;
                    return (p - buf);
                    break;
                case 0x06://会话时效剩余时间
                    *p++ = TYPE_double_long_unsigned;
                    atrtime = mbase_cb->authen_tmr/60;
                    mem_swap_cpy(p, (uint8_t*)&atrtime, 4);
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
                case 0x0E://红外认证时效
                    *p++ = TYPE_double_long_unsigned;
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + IrAuthenTime);
                    nd_get_index(instance, addr, p, 4); 
                    p = p + 4;
                    return (p - buf);
                    break;
                case 0x0F://剩余红外认证时效
                    *p++ = TYPE_double_long_unsigned;
                    atrtime = mbase_cb->irauthen_tmr/60;
                    mem_swap_cpy(p, (uint8_t*)&atrtime, 4); 
                    p = p + 4;
                    return (p - buf);
                    break;
            }
            break;
    }
    return 0x01;
}
