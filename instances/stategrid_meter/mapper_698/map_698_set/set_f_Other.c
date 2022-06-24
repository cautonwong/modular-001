#include "nidavellir.h"
#include "fr_698.h"


int SET_F10103XX(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t count,maxnum = 0, offset = 0, index=0,i, tempbuf[SecurityMode_EXData_LEN+1] = {0};
    uint8_t *p = buf, *Ptmp = tempbuf;
    uint32_t s_index=0;
    
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

    index = (unsigned char)(id & 0x000000FF);
    maxnum = (SecurityMode_EXData_LEN - 1) / 4;

    if (index > maxnum)
    {
        return -DAR_BUFFER_OVERFLOW;
    }   

    if (index == 0)
    {
        count = p[1];

        if (p[0] != TYPE_array || count > maxnum)
        {
            return -DAR_OTHERS_ERROR;
        }

        p += 2;
        *Ptmp++ = count;

        for (i = 0; i < count; i++)
        {
            if (p[0] != TYPE_structure || p[1] != 2 || p[2] != TYPE_OI || p[5] != TYPE_long_unsigned)
            {
                return -DAR_OTHERS_ERROR;
            }

            *Ptmp++ = p[4];
            *Ptmp++ = p[3];
            *Ptmp++ = p[7];
            *Ptmp++ = p[6];
            p += 8;
        }

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
        nd_set_index(instance, s_index, tempbuf, SecurityMode_EXData_LEN);
    }
    else
    {
        if (p[0] != TYPE_structure || p[1] != 2 || p[2] != TYPE_OI || p[5] != TYPE_long_unsigned)
        {
            return DAR_OTHERS_ERROR;
        }

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
        nd_get_index(instance, s_index, tempbuf, SecurityMode_EXData_LEN);
        
        offset = 1 + (index - 1) * 4;
        

        tempbuf[offset++] = p[4];
        tempbuf[offset++] = p[3];
        tempbuf[offset++] = p[7];
        tempbuf[offset++] = p[6];

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
        nd_set_index(instance, s_index, tempbuf, SecurityMode_EXData_LEN);

    }

    return (p-buf);
}





int SET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,displayid;
    uint8_t index=0,attribute,i,num=0, tempbuf[10] = {0};
    uint8_t* p = buf,Sequ=0;
    uint32_t displayoad1,displayoad2,s_index=0;
    uint16_t len = 0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);    //属性
    
    switch(oad)
    {
        case 0xF100://ESAM
            if(attribute == 0x0E)//红外认证时效
            {
                p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + IrAuthenTime);
                nd_set_index(instance, s_index, p, 4);
                p = p + 4;
                return (p-buf);
            }
            break;
        case 0xF101://
            if(attribute == 0x02)   //安全模式参数
            {
                if(mbase_cb->factor_mode == false)
                {
                    return -DAR_RW_REJECT;
                }
                p++;
                tempbuf[0] = *p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_En);
                nd_set_index(instance, s_index, &tempbuf[0], 1);
#if 0
                mbase_cb->SafeEn = *p++;
                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                offsetof(struct mbase_control_block_t, SafeEn) + 1);
                nd_set_offset(instance, s_index, p, 1);
#endif                
                return (p-buf);
            }
            else if(attribute == 0x03) //安全模式参数
            {
                return SET_F10103XX(instance,  id, buf, ctx);
            }
            break;
        case 0xF201://rs485
        case 0xF202://红外
        case 0xF205://继电器输出
        case 0xF207://多功能端子
        case 0xF209://载波模块
            return -DAR_RW_REJECT;
            break;
        case 0xF300://显示
        case 0xF301:
            switch (attribute)
            {
                  case 0x02://显示对象列表
                        if(index == 0)
                        {
                            num = *(p+1);
                            p = p + 2;
                        }
                        else
                        {
                            num = 1;
                        }
                        for(i=0;i<num;i++)
                        {
                            p = p + 3;
                            if(*p++ == 0)
                              len = 4;
                            else
                              len = 8;
                            mem_swap_cpy((uint8_t*)&displayoad1, p, 4);
                            p = p + 4;
                            if(len == 8)
                            {
                                p++;
                                mem_swap_cpy((uint8_t*)&displayoad2, p, 4);
                                p = p + 4;
                            }
                            p++;
                            Sequ = *p++;
                            displayid = display_oad_to_id((uint8_t*)&displayoad1, (uint8_t*)&displayoad2, Sequ, len);
                            if(displayid == 0)  return -DAR_BUFFER_OVERFLOW;
                            if(oad == 0xF300)
                            {
                                if(index > 0)
                                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AutoDisplayList + (index - 1));
                                else
                                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AutoDisplayList + i);
                            }
                            else
                            {
                                if(index > 0)
                                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + (index - 1));
                                else
                                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + i);
                            }
                            if(index > 0)
                                nd_set_index(instance, s_index, (uint8_t*)&displayid, 2); 
                            else
                                nd_set_index(instance, s_index, (uint8_t*)&displayid, 2); 
                        }
                        return (p-buf);
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
                                    mbase_cb->lcd_autodis_list_maxnum = 99;
                                else
                                    mbase_cb->lcd_keydis_list_maxnum = 99;
                                //len = 2;
                                //s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_cb << 16) + 
                                //offsetof(struct mbase_control_block_t, lcd_autodis_list_num));
                                
                                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                                offsetof(struct mbase_control_block_t, backlight_key_time));
                                len = offsetof(struct mbase_control_block_t, Baut_plc) - offsetof(struct mbase_control_block_t, backlight_key_time);
                                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, len);
                                return (p-buf);
                                break;
                            case 0x01://当前总对象数
                                p = p + 1;
                                if(oad == 0xF300)
                                    mbase_cb->lcd_autodis_list_num = *p++;
                                else
                                    mbase_cb->lcd_keydis_list_num = *p++;

                                //s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                                //offsetof(struct mbase_control_block_t, lcd_autodis_list_num));
                                //len = offsetof(struct mbase_control_block_t, lcd_autodis_list_maxnum) - offsetof(struct mbase_control_block_t, lcd_autodis_list_num);
                                //nd_set_offset(instance, s_index, p, len);
                                
                                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                                offsetof(struct mbase_control_block_t, backlight_key_time));
                                len = offsetof(struct mbase_control_block_t, Baut_plc) - offsetof(struct mbase_control_block_t, backlight_key_time);
                                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, len);
                                return (p-buf);
                                break;
                            case 0x02://允许最大对象数
                                return -DAR_RW_REJECT;
                                /*p = p + 1;
                                if(oad == 0xF300)
                                    mbase_cb->lcd_autodis_list_maxnum = *p++;
                                else
                                    mbase_cb->lcd_keydis_list_maxnum = *p++;
                                //len = 1;
                                //s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_cb << 16) + 
                                //offsetof(struct mbase_control_block_t, lcd_autodis_list_maxnum));
                                
                                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                                offsetof(struct mbase_control_block_t, backlight_key_time));
                                len = offsetof(struct mbase_control_block_t, Baut_plc) - offsetof(struct mbase_control_block_t, backlight_key_time);
                                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, len);
                                return (p-buf);*/
                                break;
                            default:
                                return -DAR_OTHERS_ERROR;
                                break;
                        }
                        
                        break;
                  case 0x03://每个对象显示时间
                        if((index == 0x00) || (index == 0x01))
                        {
                            p = p + 1;
                            if(oad == 0xF300)
                            {
                                mem_swap_cpy((unsigned char*)&displayid, p, 2);
                                if((displayid < 5) || (displayid > 20))   return -DAR_BUFFER_OVERFLOW;
                                mem_swap_cpy((unsigned char*)&mbase_cb->lcd_autodis_keep_sec, p, 2);
                            }
                            else
                                mem_swap_cpy((unsigned char*)&mbase_cb->lcd_keydis_keep_sec, p, 2);
                            //index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DisplayPara);
                            //nd_set_index(instance, index, (unsigned char*)&mbase_cb->backlight_key_time, 22);
                            p = p + 2;
                            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                            offsetof(struct mbase_control_block_t, backlight_key_time));
                            len = offsetof(struct mbase_control_block_t, Baut_plc) - offsetof(struct mbase_control_block_t, backlight_key_time);
                            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, len);
                            return (p-buf);
                        }
                        else
                        {
                            return -DAR_OTHERS_ERROR;
                        }
                        break;
                  default:
                        return -DAR_OTHERS_ERROR;
                        break;
            }
            break;
        case 0xFFFF: // 内扩命令
        {
            switch (attribute)
            {
                case 0x02:// 进厂内
                {
                    tempbuf[0] = *p++;
                    
                    if( ((tempbuf[0] == 0xAA) && (mbase_cb->meter_cover_sta == EVENT_SET)) || (tempbuf[0] == 0xFB) )
                    {
                        mbase_cb->factor_mode = true;
                    }
                    else if(tempbuf[0] == 0x00) // 退厂内
                    {
                        mbase_cb->factor_mode = false;
                    }
                    else 
                    {
                        return -DAR_OTHERS_ERROR;
                    }
                    // 写 e2p 待调整
                }
                return (p-buf);
                break;
            }  
        }
            break;
        default:
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
