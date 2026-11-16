#include "fr_698.h"
#include "port.h"
#include "application/adapters/dl698/meter_base_view.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "infra/kernel/resource_blocks.h"


int GET_0_Energy(void *instance, uint32_t id, uint8_t* buf, uint8_t mode, void *ctx)
{    
    uint8_t i;
    uint16_t OI;
    uint8_t attr_index, attribute,Num;
    int16_t len=-1;
    uint8_t *ptr, *p=buf;
    int32_t tmp=0;
    uint32_t s_index=0;
    unsigned long long engery=0;
    uint8_t phase = 0,pn = 0;
    if(!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    OI = (uint16_t)(id>>16);
    attr_index = (uint8_t)(id&0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    phase = 0;//(uint8_t)(OI&0x000F);
    
    if((OI != 0x0000) && (OI != 0x0010) && (OI != 0x0020))
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    if(attribute == 1)
    {
        p += fr_698_get_logic_name(p, OI);
        return (p-buf);
    }

    if(attribute == 0x03)
    {
      if(attr_index != 0x00)   return -DAR_OBJECT_NOT_EXIST;
      switch(mode)
      {
      case 0x00:
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0xFE;//-2;
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
      if(attr_index != 0x00)   return -DAR_OBJECT_NOT_EXIST;  
      switch(mode)
      {
      case 0x00:
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0xFC;//-4;
        *p++ = unit_kWh;
        return (p-buf);   
      case 0x01:
        *p++ = TYPE_Scaler_Unit;
        *p++ = 0x02;
        return (p-buf);
      }
    }
    
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3);    //读取费率个数
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
    if((Num==0) || (Num>12))
    {
        Num = 0;
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
                          tmp = nd_mb_cal_active_energy(instance,  i, 0);
                          mem_swap_cpy(ptr, (uint8_t*)&tmp, 4);
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
                            engery = nd_mb_cal_active_energy(instance, i, 1);
                            mem_swap_cpy(ptr, (uint8_t*)&engery, 8);
                            ptr += 8;
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
                else
                {
                    len = -DAR_OBJECT_NOT_EXIST;
                }
            }
            else if(attr_index<=Num)
            {
                attr_index -= 1;
                if(attribute == 2)
                {
                    switch(mode)
                    {
                    case 0x00:
                      buf[0] = TYPE_double_long;
                      tmp = nd_mb_cal_active_energy(instance, attr_index, 0);
                      mem_swap_cpy(&buf[1], (uint8_t*)&tmp, 4);
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
                      engery = nd_mb_cal_active_energy(instance, attr_index, 1);
                      mem_swap_cpy(&buf[1], (uint8_t*)&engery, 8);
                      len = 9;      
                      break;
                    case 0x01:
                      buf[0] = TYPE_long64;
                      buf[1] = 0x08;
                      len = 2;                        
                      break;
                    }
                }
                else
                {
                    len = -DAR_OBJECT_NOT_EXIST;
                }
            }
            else
            {
                len = -DAR_OBJECT_UNDEF;
            }
        }
        break;
          
        case 0x0010:
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
                              *ptr++ = TYPE_double_long_unsigned;
                              tmp = nd_mb_active_energy_tabc(instance, phase, i, 0, 0);
                              mem_swap_cpy(ptr, (uint8_t*)&tmp, 4);
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
                          for(i=0;i<Num;i++)
                          {
                            *ptr++ = TYPE_long64_unsigned;
                            engery = nd_mb_active_energy_tabc(instance, phase, i, 1, 0);
                            mem_swap_cpy(ptr, (uint8_t*)&engery, 8);
                            ptr += 8;
                            len += 9;
                          }
                          break;
                        case 0x01:
                          buf[0] = TYPE_long64_unsigned;
                          buf[1] = 0x08;
                          len = 2;
                          break;
                    }
                }
                else
                {
                    len = -DAR_OBJECT_NOT_EXIST;
                }
            }
            else if(attr_index<=Num)
            {
                attr_index -= 1;
                switch(mode)
                {
                case 0x00:
                  if(attribute==2)
                  {
                      buf[0] = TYPE_double_long_unsigned;
                      tmp = nd_mb_active_energy_tabc(instance, phase, attr_index, 0, 0);
                      mem_swap_cpy(&buf[1], (uint8_t*)&tmp, 4);
                      len = 5;
                  }
                  else if(attribute==4)
                  {
                      buf[0] = TYPE_long64_unsigned;
                      engery = nd_mb_active_energy_tabc(instance, phase, attr_index, 1, 0);
                      mem_swap_cpy(&buf[1], (uint8_t*)&engery, 8);
                      len = 9;
                  }
                  else
                    {
                        len = -DAR_OBJECT_NOT_EXIST;
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
                len = -DAR_OBJECT_UNDEF;

            }
        }
        break;
        
        case 0x0020:
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
                          *ptr++ = TYPE_double_long_unsigned;
                          tmp = nd_mb_active_energy_tabc(instance, phase, i, 0, 1);
                          mem_swap_cpy(ptr, (uint8_t*)&tmp, 4);
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
                          for(i=0;i<Num;i++)
                          {
                            *ptr++ = TYPE_long64_unsigned;
                            engery = nd_mb_active_energy_tabc(instance, phase, i, 1, 1);
                            mem_swap_cpy(ptr, (uint8_t*)&engery, 8);
                            ptr += 8;
                            len += 9;
                          }
                          break;
                        case 0x01:
                          buf[0] = TYPE_long64_unsigned;
                          buf[1] = 0x08;
                          len = 2;
                          break;
                    }
                }
                else
                {
                    len = -DAR_OBJECT_NOT_EXIST;
                }
            }
            else if(attr_index<=Num)
            {
                attr_index -= 1;
                switch(mode)
                {
                  case 0x00:
                  if(attribute==2)
                  {
                      buf[0] = TYPE_double_long_unsigned;
                      tmp = nd_mb_active_energy_tabc(instance, phase, attr_index, 0, 1);
                      mem_swap_cpy(&buf[1], (uint8_t*)&tmp, 4);
                      len = 5;
                  }
                  else if(attribute==4)
                  {
                      buf[0] = TYPE_long64_unsigned;
                      engery = nd_mb_active_energy_tabc(instance, phase, attr_index, 1, 1);
                      mem_swap_cpy(&buf[1], (uint8_t*)&engery, 8);
                      len = 9;
                  }
                else
                {
                    len = -DAR_OBJECT_NOT_EXIST;
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
                len = -DAR_OBJECT_UNDEF;
            }
        }
        break;
        
       default:
        len = -DAR_OBJECT_NOT_EXIST;
        break;
    }
    return len;
}
