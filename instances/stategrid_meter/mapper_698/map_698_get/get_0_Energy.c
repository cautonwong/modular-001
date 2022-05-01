#include "nidavellir.h"
#include "fr_698.h"


int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t i;
    uint16_t OI;
    uint8_t attr_index, attribute,Num;
    int16_t len=-1;
    uint8_t *ptr, *p=buf;
    int32_t tmp=0;
    uint32_t s_index=0;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    OI = (uint16_t)(id>>16);
    attr_index = (uint8_t)(id&0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    if(attribute == 0x03)
    {
      switch(mode)
      {
      case 0x00:
        *p++ = TYPE_Scaler_Unit;
        *p++ = -2;
        *p++ = unit_kWh;
        return (p-buf);
      case 0x01:  // T L 的信息
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0x02;
        return (p-buf);
      }
    }
    else if(attribute == 0x05)
    {
      switch(mode)
      {
      case 0x00:
        *p++ = TYPE_Scaler_Unit;
        *p++ = -4;
        *p++ = unit_kWh;
        return (p-buf);   
      case 0x01:
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0x02;
        return (p-buf);
      }
    }
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3);    //读取费率个数
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
    if((Num==0) || (Num>12))
    {
        Num = 12;
    }
    Num += 1;
    switch(OI)
    {
        case 0x0000:
        {
            if(attr_index==0)
            {
                buf[0] = 0x01;//数组
                buf[1] = Num;//数据块
                len = 2;
                ptr = &buf[2];
                if(attribute == 2)
                {
                    switch(mode)
                    {
                    case 0x00:
                      for(i=0;i<Num;i++)
                      {
                          *ptr++ = TYPE_double_long;
                          tmp = Get_Cal_Active_Energy(mbase_cb , i , 0);
                          memcpy(ptr, (uint8_t*)&tmp, 4);
                          ptr += 4;
                          len += 5;
                      }                      
                      break;
                    case 0x01:
                      for(i=0;i<Num;i++)
                      {
                          *ptr++ = TYPE_double_long;
                          *ptr++ = 0x04;
                          len += 2;
                      }                         
                      break;
                    }
                }
                else if(attribute == 4)
                {
                    switch(mode)
                    {
                    case 0x00:
                      for(i=0;i<Num;i++)
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
                      break;
                    case 0x01:
                      for(i=0;i<Num;i++)
                      {
                          *ptr++ = TYPE_long64;
                          *ptr++ = 0x08;
                          len += 2;
                      }                      
                      break;                   
                    }
                }
            }
            else if(attr_index<Num)
            {
                if(attribute == 2)
                {
                    switch(mode)
                    {
                    case 0x00:
                      buf[0] = TYPE_double_long;
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      len = 5;                        
                      break;
                    case 0x01:
                      buf[0] = TYPE_double_long;
                      buf[1] = 0x04;
                      len = 0x02;
                      break;
                    }
                }
                else if(attribute == 4)
                {
                    switch(mode)
                    {
                    case 0x00:
                      buf[0] = TYPE_long64;
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      tmp = Get_Cal_Active_Energy(mbase_cb , attr_index , 1);
                      memcpy(&buf[5], (uint8_t*)&tmp, 4);
                      len = 9;                        
                      break;
                    case 0x01:
                      buf[0] = TYPE_long64;
                      buf[1] = 0x08;
                      len = 2;                        
                      break;
                    }
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
                if(attribute == 2)
                {
                    buf[0] = 0x01;//数组
                    buf[1] = Num;//数据块
                    len = 2;
                    ptr = &buf[2];
                    switch(mode)
                    {
                        case 0x00:
                          for(i=0;i<Num;i++)
                          {
                              *ptr++ = TYPE_double_long_unsigned;
                              memcpy(ptr, (uint8_t*)&mbase_cb->active_p[i], 4);
                              ptr += 4;
                              len += 5;
                          }                  
                          break;
                        case 0x01:
                          for(i=0;i<Num;i++)
                          {
                              *ptr++ = TYPE_double_long_unsigned;
                              *ptr++= 4;
                              len += 2;
                          }                  
                          break;
                    }
                }
                else if(attribute == 4)
                {
                    switch(mode)
                    {
                        case 0x00:
                          buf[0] = TYPE_long64;
                          tmp = Get_Active_Energy(mbase_cb , attr_index , 0 , 0);
                          memcpy(&buf[1], (uint8_t*)&tmp, 4);
                          tmp = Get_Active_Energy(mbase_cb , attr_index , 1 , 0);
                          memcpy(&buf[5], (uint8_t*)&tmp, 4);
                          len = 9;                        
                          break;
                        case 0x01:
                          buf[0] = TYPE_long64;
                          buf[1] = 0x08;
                          len = 2;
                          break;
                    }
                }
            }
            else if(attr_index<Num)
            {
                switch(mode)
                {
                case 0x00:
                  if(attribute==2)
                  {
                      buf[0] = TYPE_double_long_unsigned;
                      memcpy(&buf[1], (uint8_t*)&mbase_cb->active_p[attr_index], 4);
                      len = 5;
                  }
                  else if(attribute==4)
                  {
                      buf[0] = TYPE_long64;
                      tmp = Get_Active_Energy(mbase_cb , attr_index , 0 , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      tmp = Get_Active_Energy(mbase_cb , attr_index , 1 , 0);
                      memcpy(&buf[5], (uint8_t*)&tmp, 4);
                      len = 9;
                  }
                  break;
                case 0x01:
                  buf[0] = TYPE_double_long_unsigned;
                  buf[1] = 0x04;
                  len = 2;                            
                  break;
                }
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
                if(attribute == 2)
                {
                    buf[0] = 0x01;//数组
                    buf[1] = Num;//数据块
                    len = 2;
                    ptr = &buf[2];
                    switch(mode)
                    {
                    case 0x00:
                      for(i=0;i<Num;i++)
                      {
                          *ptr++ = TYPE_double_long_unsigned;
                          memcpy(ptr, (uint8_t*)&mbase_cb->active_n[i], 4);
                          ptr += 4;
                          len += 5;
                      }
                      break;
                    case 0x01:
                      for(i=0;i<Num;i++)
                      {
                          *ptr++ = TYPE_double_long_unsigned;
                          *ptr++ = 4;
                          len += 2;
                      }
                      break;
                    }
                }
                else if(attribute == 4)
                {
                    switch(mode)
                    {
                        case 0x00:
                          buf[0] = TYPE_long64;
                          tmp = Get_Active_Energy(mbase_cb , attr_index , 0 , 1);
                          memcpy(&buf[1], (uint8_t*)&tmp, 4);
                          tmp = Get_Active_Energy(mbase_cb , attr_index , 1 , 1);
                          memcpy(&buf[5], (uint8_t*)&tmp, 4);
                          len = 9;
                          break;
                        case 0x01:
                          buf[0] = TYPE_long64;
                          buf[1] = 0x08;
                          len = 2;
                          break;
                    }
                }
            }
            else if(attr_index<Num)
            {
                switch(mode)
                {
                  case 0x00:
                  if(attribute==2)
                  {
                      buf[0] = TYPE_double_long_unsigned;
                      memcpy(&buf[1], (uint8_t*)&mbase_cb->active_n[attr_index], 4);
                      len = 5;
                  }
                  else if(attribute==4)
                  {
                      buf[0] = TYPE_long64;
                      tmp = Get_Active_Energy(mbase_cb , attr_index , 0 , 0);
                      memcpy(&buf[1], (uint8_t*)&tmp, 4);
                      tmp = Get_Active_Energy(mbase_cb , attr_index , 1 , 0);
                      memcpy(&buf[5], (uint8_t*)&tmp, 4);
                      len = 9;
                  }
                  break;
                case 0x01:
                  buf[0] = TYPE_double_long_unsigned;
                  buf[1] = 0x04;
                  len = 2;                  
                  break;
                }
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
