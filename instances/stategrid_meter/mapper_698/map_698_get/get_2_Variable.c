#include "nidavellir.h"
#include "fr_698.h"



int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;


    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x20000200://电压
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_long_unsigned;
            memcpy(buf+3, (uint8_t*)&mbase_cb->voltage_ac, 0x02);
            return 0x05;
        }
    break;
    case 0x20010200://电流  current_l current_n
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x01;//单相一个元素
            buf[2] = TYPE_double_long;
            memcpy(buf+3, (uint8_t*)&mbase_cb->current_l, 0x04);
            return 0x07;
        }
    break;
    case 0x20040200://active_power 
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x02;//总 分
            buf[2] = TYPE_double_long;
            memcpy(buf+3, (uint8_t*)&mbase_cb->active_power, 0x04);
            buf[7] = TYPE_double_long;
            memcpy(buf+8, (uint8_t*)&mbase_cb->active_power, 0x04);
            return 12;
        }
    break;
    case 0x200F0200://frequency
        {
            buf[0] = TYPE_long_unsigned;//数组
            memcpy(buf+1, (uint8_t*)mbase_cb->frequency, 0x02);
            return 0x03;
        }
    break;
    case 0x200A0200://power_factor
        {
            buf[0] = 0x01;//数组
            buf[1] = 0x02;//总分
            buf[2] = TYPE_long;
            memcpy(buf+3, (uint8_t*)&mbase_cb->power_factor, 0x02);
            buf[5] = TYPE_long;
            memcpy(buf+6, (uint8_t*)&mbase_cb->power_factor, 0x02);
            return 8;
        }
    break;
    case 0x20100200://temperature
        {
            buf[0] = TYPE_long;
            memcpy(buf+1, (uint8_t*)&mbase_cb->temperature, 0x02);
            return 3;
        }
    break;
    case 0x20110200://voltage_battery
        {
            buf[0] = TYPE_long;
            memcpy(buf+1, (uint8_t*)&mbase_cb->voltage_battery, 0x02);
            return 3;
        }
    break;
    case 0x20220200://事件记录序号
    {
        buf[0] = TYPE_double_long_unsigned;
        memset(&buf[1], 0xFF, 4);
        return 5;
    }
    break;

    case 0x201E0200://事件发生时间
    case 0x20200200://事件结束时间    
    {
        uint16_t year_temp = instance->time.tm_year;// +2000;
        *p++ = TYPE_date_time_s;
        p += uint16_to_buf_msb(year_temp, p);
        *p++ = (uint8_t)instance->time.tm_mon;
        *p++ = (uint8_t)instance->time.tm_mday;
        *p++ = (uint8_t)instance->time.tm_hour;
        *p++ = (uint8_t)instance->time.tm_min;
        *p++ = (uint8_t)instance->time.tm_sec;

        return (p-buf);
    }
    break;

    case 0x20240200:// 时间发生源  
    case 0x33000200:// 事件上报状态   
    {
        buf[0] = TYPE_null;
        return 1;
    }
    
    break;    
    case 0x20000000:  //测试代码
        break;
    default:
        break;
    }
    return 0x01;
}
