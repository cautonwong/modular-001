#include "nidavellir.h"
#include "fr_698.h"


int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t i;
    uint16_t OI;
    uint8_t attr_index, attribute;
    int16_t len=-1;
    uint8_t *ptr, *p=buf;
    int32_t tmp=0;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    OI = (uint16_t)(id>>16);
    attr_index = (uint8_t)(id&0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    if(attribute == 0x03)
    {
        *p++ = TYPE_Scaler_Unit;
        *p++ = -2;
        *p++ = unit_kWh;
        return (p-buf);
    }
    else if(attribute == 0x05)
    {
        *p++ = TYPE_Scaler_Unit;
        *p++ = -4;
        *p++ = unit_kWh;
        return (p-buf);
    }
    
    switch(OI)
    {
        case 0x0000:
        {
            if(attr_index==0)
            {
                buf[0] = 0x01;//数组
                buf[1] = 13;//数据块
                len = 2;
                ptr = &buf[2];
                if(attribute == 2)
                {
                    for(i=0;i<13;i++)
                    {
                        *ptr++ = TYPE_double_long;
                        tmp = Get_Cal_Active_Energy(mbase_cb , i , 0);
                        memcpy(ptr, (uint8_t*)&tmp, 4);
                        ptr += 4;
                        len += 5;
                    }
                }
                else if(attribute == 4)
                {
                    for(i=0;i<13;i++)
                    {
                        *ptr++ = TYPE_long64;
                        tmp = Get_Cal_Active_Energy(mbase_cb , i , 0);
                        memcpy(ptr, (uint8_t*)&tmp, 4);
                        ptr += 4;
                        tmp = Get_Cal_Active_Energy(mbase_cb , i , 1);
                        memcpy(ptr, (uint8_t*)&tmp, 4);
                        ptr += 4;
                        len += 9;
                    }
                }
            }
            else if(attr_index<13)
            {
                if(attribute == 2)
                {
                      buf[0] = TYPE_double_long;
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      len = 5;
                }
                else if(attribute == 4)
                {
                      buf[0] = TYPE_long64;
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 1);
                      memcpy(&buf[5], (uint8_t*)&tmp, 4);
                      len = 9;
                }
            }
            else
            {
                len = -1;
            }
        }
        break;
          
        case 0x0010:
        {
            if(attr_index==0)
            {
                buf[0] = 0x01;//数组
                buf[1] = 13;//数据块
                len = 2;
                ptr = &buf[2];
                for(i=0;i<13;i++)
                {
                    *ptr++ = TYPE_double_long_unsigned;
                    memcpy(ptr, (uint8_t*)&mbase_cb->active_p[i], 4);
                    ptr += 4;
                    len += 5;
                }
            }
            else if(attr_index<13)
            {
                buf[0] = TYPE_double_long_unsigned;
                memcpy(&buf[1], (uint8_t*)&mbase_cb->active_p[attr_index], 4);
                len = 5;
            }
            else
            {
                len = -1;
            }
        }
        break;
        
        case 0x0020:
        {
            if(attr_index==0)
            {
                buf[0] = 0x01;//数组
                buf[1] = 13;//数据块
                len = 2;
                ptr = &buf[2];
                for(i=0;i<13;i++)
                {
                    *ptr++ = TYPE_double_long_unsigned;
                    memcpy(ptr, (uint8_t*)&mbase_cb->active_n[i], 4);
                    ptr += 4;
                    len += 5;
                }
            }
            else if(attr_index<13)
            {
                buf[0] = TYPE_double_long_unsigned;
                memcpy(&buf[1], (uint8_t*)&mbase_cb->active_n[attr_index], 4);
                len = 5;
            }
            else
            {
                len = -1;
            }
        }
        break;
        
       default:
        len = -1;
        break;
    }
    return len;
}
