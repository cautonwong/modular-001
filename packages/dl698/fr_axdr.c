#include "nidavellir.h"
#include "fr_698.h"


uint8_t axdr_decode(uint8_t type, const uint8_t *buf, uint16_t *len)
{
    if (buf[0] < 128)
    {
        if (len != NULL)
        {
            *len = buf[0];
        }
        return 1;
    }
    else if (buf[0] == 0x81)
    {
        if (len != NULL)
        {
            *len = buf[1];
        }
        return 2;
    }
    else if (buf[0] == 0x82)
    {
        if (len != NULL)
        {
            *len = ((uint16_t)buf[1] << 8) + buf[2];
        }
        return 3;
    }
    else
    {
        if (len != NULL)
        {
            *len = 0;
        }
        return 0;
    }
}

uint8_t axdr_len_encode(uint8_t* buf, uint16_t len)
{
    if (len < 128)
    {
        if (buf != NULL)
        {
            buf[0] = (uint8_t)len;
        }

        return 1;
    }
    else if (len < 256)
    {
        if (buf != NULL)
        {
            buf[0] = 0x81;
            buf[1] = (uint8_t)len;
        }

        return 2;
    }
    else
    {
        if (buf != NULL)
        {
            buf[0] = 0x82;
            buf[1] = (uint8_t)(len >> 8);
            buf[2] = (uint8_t)len;
        }

        return 3;
    }
}

uint16_t axdr_get_len(const uint8_t *data)
{
    uint16_t type_len = 0;
    uint16_t data_len = 0,total_len=0;
    uint16_t len = 0;
    uint8_t num = 0;
    uint8_t type = data[0];

    switch (type)
    {
    case TYPE_null:
        return 1;
        break;
    case TYPE_array: 
    case TYPE_structure:
        data_len++;
        num = data[data_len];
        data_len++;
        data = data + data_len;
        data_len = 0;
        total_len = 0;
        while (num--)
        {
            data_len = axdr_get_len(data);
            total_len = total_len + data_len;
            data = data + data_len;
        }
        return (total_len + 2);
        break;
    case TYPE_bool:
    case TYPE_enum:
    case TYPE_integer:
    case TYPE_unsigned:
        return 1+1;
        break;
    case TYPE_long:
    case TYPE_long_unsigned:
    case TYPE_Scaler_Unit:
    case TYPE_OI:
        return 2+1;
        break;
    case TYPE_double_long:
    case TYPE_double_long_unsigned:
    case TYPE_OAD:
    case TYPE_OMD:
        return 4+1;
        break;
    case TYPE_long64:
    case TYPE_long64_unsigned:
        return 8+1;
        break;
    case TYPE_bit_string:
    case TYPE_octet_string:
    case TYPE_visible_string:
    case TYPE_UTF8_string:
    case TYPE_TSA:
    case TYPE_MAC:
    case TYPE_RN:        
        type_len = 1;
        type_len += axdr_decode(type, data+type_len, &data_len);
        if(TYPE_bit_string == type)
            type_len += (data_len + 7) / 8;
        else
            type_len += data_len;
        return (type_len);
        break;

    case TYPE_date_time:
        return 10;
        break;
    case TYPE_date:
    case TYPE_COMDCB:
        return 6;
        break;
    case TYPE_time:
    case TYPE_TI:
        return 3;
        break;
    case TYPE_date_time_s:
        return 7;
        break;

    case TYPE_Region:
        data++;
        len = 1;
        data_len = axdr_get_len(data);
        data += data_len;
        len += data_len;
        data_len = axdr_get_len(data);
        return (len + data_len);
        break;

    case TYPE_RSD:
        type = data[0];
        len = 1;
        if (0 == type)
        {
           len = 1;
        }
        else if (1 == type)
        {
            len += 4;
            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len]  ))
                return 0;

            type_len = axdr_decode(data[len], data+len, &data_len);
            len += type_len + data_len;
        }
        else if (2 == type)
        {
            len += 4;
            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len]))
                return 0;

            type_len = axdr_decode(data[len], data+len, &data_len);
            len += type_len + data_len;

            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len] ))
                return 0;

            type_len = axdr_decode(data[len], data+len, &data_len);
            len += type_len + data_len;
            if ((TYPE_TI != data[len]) && (TYPE_double_long_unsigned != data[len]) && (TYPE_null != data[len]))
                return 0;

            len += type_len + data_len;
        }
        else if ( 9 == type)
        {
            len += 1;
        }
        else
        {
            return 0;
        }
        return len;
        break;

    case TYPE_CSD:
        len = 1+4;
        if (1 == data[0]){
            num  = data[len];
            len += num * 4;
        }
        return len;
        break;

    case TYPE_SID:
        len = 4;
        if(TYPE_octet_string == data[len])
            type_len = axdr_decode(data[len], data+len, &data_len);
        return (len + type_len + data_len);
        break;

    case TYPE_SID_MAC:
        len = 4;
        type_len = axdr_decode(data[len], data+len, &data_len);
        len += type_len + data_len;
        type_len = axdr_decode(data[len], data+len, &data_len);
        return (len + type_len + data_len);
        break;

    case TYPE_ROAD:
        len = 4;
        num  = data[len];
        len += num * 4;
        return (len);
        break;

    case TYPE_RCSD:
        len = 1;
        num = data[0];
        while (num--)
        {
            type = data[len];
            len +=1+4;
            if (1 == type){
                num  = data[len];
                len += num * 4;
            }
        }
        return len;
        break;

#if 0
    case DTC_HIGH_ENERGY_INTEGER:
    case DTC_HIGH_ENERGY_UNSIGNED:
        p += 5;
        break;
#endif
    default:
        break;
    }

    return 0;
}


uint16_t axdr_record_len_get(uint8_t* data, uint16_t* rsdLen, uint16_t* rcsdLen)
{
    uint8_t  tmpData, * p = data;
    uint16_t len;

    /* rsd */
    tmpData = *(p - 1);
    *(p - 1) = TYPE_RSD;
    len = axdr_get_len(p - 1);
    if (len == 0)
    {
        return 0;
    }
    len -= 1;
    *(p - 1) = tmpData;
    p += len;

    if (rsdLen != NULL)
    {
        *rsdLen = len;
    }

    /* rcsd */
    tmpData = *(p - 1);
    *(p - 1) = TYPE_RCSD;
    len = axdr_get_len(p - 1);
    if (len == 0)
    {
        return 0;
    }
    len -= 1;
    *(p - 1) = tmpData;
    p += len;

    if (rcsdLen != NULL)
    {
        *rcsdLen = len;
    }

    return p - data;
}

uint16_t get_octet_string(struct context_698_t* ctx, uint16_t *offset)
{
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len = 0;
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    *offset = ctx->apdu_rx_offset;
    ctx->apdu_rx_offset += len;
    return len;
}






/////////////////////////////////////////////////
/////////////////////////////////////////////////
//GS   暂存

uint32_t datetimes_to_second(struct data_time_s_t* dt)
{
    struct tm time;

    time.tm_sec = dt->second;
    time.tm_min = dt->minute;
    time.tm_hour = dt->hour;
    time.tm_mday = dt->day;
    time.tm_mon = dt->month - 1;
    time.tm_year = (uint8_t)(dt->year - 1900);

    return rtc_mktime(&time);
}

//year, month, day, hour, minute, secon
uint32_t dt_buf_to_second(uint8_t* dt)
{
    uint16_t year;
    struct tm time;


    year = buf_to_uint16_msb(dt);
    dt += 2;

    time.tm_year = (uint8_t)(year  - 1900);
    time.tm_mon = (*dt++)-1;
    time.tm_mday = *dt++;
    time.tm_hour = *dt++;
    time.tm_min = *dt++;
    time.tm_sec = *dt++;

    return rtc_mktime(&time);
}

//year, month, day, hour, minute, secon
int second_to_dt_buf(uint32_t timesec, uint8_t* dt)
{
    uint8_t* p = dt;
    uint16_t year_temp;
    struct tm time;

    rtc_localtime(timesec, &time);

    year_temp = (time.tm_year + 1900);

    p += uint16_to_buf_msb(year_temp, p);
    *p++ = (uint8_t)time.tm_mon + 1;
    *p++ = (uint8_t)time.tm_mday;
    *p++ = (uint8_t)time.tm_hour;
    *p++ = (uint8_t)time.tm_min;
    *p++ = (uint8_t)time.tm_sec;

    return (p - dt);
}

uint32_t ti_to_second(uint8_t unit, uint16_t interval)
{
    uint32_t tmp = 0;

    switch (unit)
    {
    case 0: // second
        tmp = interval;
        break;
    case 1: // minute
        tmp = interval * 60l;
        break;
    case 2: // hour 
        tmp = interval * 3600l;
        break;
    case 3: // day 
        tmp = interval * 3600l * 24;
        break;
    case 4: // month 
        tmp = interval * 3600l * 30 * 12;
        break;
    case 5: // year 
        tmp = interval * 3600l * 30 * 12 * 365;
        break;
    default:
        break;
    }

    return tmp;
}
//year, month, day, hour, minute, secon
uint32_t dt_buf_to_month(uint8_t* dt)
{
    uint16_t time[2];
    uint16_t year;
    uint32_t month;

    year = buf_to_uint16_msb(dt);
    dt += 2;

    time[1] = (year - 2000);
    time[0] = *dt++;
    month = time[1] * 12;
    month += time[0];

    return month;
}




// 方法1 事件rsd解析
int decode_rsd_event_selector_1(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存
    uint32_t rsd_oad;
    
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    memcpy(p, apdu_rx + ctx->apdu_rx_offset, 4);
    ctx->apdu_rx_offset += 4;
    rsd_oad = buf_to_uint32_msb(p);

    switch (rsd_oad)
    {
    case 0x201e0200: // 事件发生时间
    case 0x20200200: // 事件结束时间 
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }
        rsd_info->start_mark = dt_buf_to_second(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;
        rsd_info->end_mark = rsd_info->start_mark;

        if (rsd_oad == 0x201e0200)
        {
            rsd_info->mode = 2;
        }
        else
        {
            rsd_info->mode = 3; 
        }
        break;

    case 0x20220200: //事件记录序号
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }

        rsd_info->start_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;
        rsd_info->end_mark = rsd_info->start_mark;
        rsd_info->mode = 1;
        break;

    default:
        return -1;
    }

    return (1);
}
// 非周期性冻结判断
uint8_t frz_non_period_type_judge(uint16_t oi)
{

    if ((oi <= 0x5007) && (oi >= 0x5001) && (oi != 0x5005))
    {
        return 0;   //周期性冻结
    }

    return 1;        //非周期性冻结
}
struct frz_period_t
{
    uint16_t oi;
    uint16_t period_id;
    uint32_t one_period;// 秒
};
const struct frz_period_t frz_period_tab[] =
{
    {0x5002, MinuteFreeze_period,   1 * 60,             },
    {0x5003, HourFreeze_period,     1 * 60 * 60,        }, 
    {0x5004, DayFreeze_period,      1 * 60 * 60 * 24,   },
};


//周期性冻结(除去月冻结) 用方法1 时间点筛选时 给予时间点开始再加一个周期的时间范围搜索
int frz_selector_1_add_period(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存

    uint32_t rcsd, range_sec = 0,index;
    uint16_t frz_oi = (uint16_t)((oad&0xFFFF0000) >> 16);
    uint16_t PeriodId = 0xFFFF;
    uint8_t period_buf[24], scheme = 0, i, period = 0, rcsd_num = 0;
    int ret;

    //判断是否为周期冻结
    if ((1 == frz_non_period_type_judge(frz_oi)) || (frz_oi == 0x5006))
    {
        return -1;
    }

    //根据冻结 获取冻结周期
    for (i = 0; i < sizeof(frz_period_tab) / sizeof(frz_period_tab[0]); i++)
    {
        if (frz_oi == frz_period_tab[i].oi)
        {
            PeriodId = frz_period_tab[i].period_id;
            range_sec = frz_period_tab[i].one_period;
            break;
        }
    }

    if (PeriodId == 0xFFFF)
    {
        return -1;
    }

    //获取捕获周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodId);
    nd_get_index(instance, index, &period_buf[0], sizeof(period_buf));

    memcpy(p, apdu_rx + ctx->apdu_rx_offset, 15);
      
    rcsd_num = *(p+0); //待调整
    if (rcsd_num)
    {
        rcsd = buf_to_uint32_msb(p + 2);//暂取第一个rcsd
        scheme = get_OAD_Schem(rcsd);
    }

    if (scheme > 7)
    {
        scheme = 0;
    }

    if (period_buf[scheme * 3] != 0)
    {
        period = period_buf[scheme * 3 + 1];
    }
    range_sec = range_sec * period;
    // 结束时间  增加 周期s的时间
    rsd_info->end_mark += (range_sec);

    return true;
}

int decode_rsd_freeze_selector_1(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad,  struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存
    uint32_t rsd_oad;

    memcpy(p, apdu_rx + ctx->apdu_rx_offset, 4);
    ctx->apdu_rx_offset += 4;
    rsd_oad = buf_to_uint32_msb(p);

    switch (rsd_oad)
    {
    case 0x20210200: // 冻结时间    
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }
        rsd_info->start_mark = dt_buf_to_second(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;
        rsd_info->end_mark = rsd_info->start_mark;
        rsd_info->mode = 2;
        frz_selector_1_add_period(instance,pipeline,oad, rsd_info);
        break;

    case 0x20230200: // 冻结记录序号
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }
        rsd_info->start_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;
        rsd_info->end_mark = rsd_info->start_mark;
        rsd_info->mode = 1;
        break;

    default:
        return -1;
    }

    return (1);
}
int selector_2_seq(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t* rsd_info)
{
    return 1;//待调整
}
int decode_rsd_freeze_selector_2(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache;//当临时缓存
    uint16_t value;
    uint32_t rsd_oad;
    uint16_t oi = get_OAD_OI(oad);


    rsd_oad = buf_to_uint32_msb(&apdu_rx[ctx->apdu_rx_offset]);
    ctx->apdu_rx_offset += 4;

    switch (rsd_oad)
    {
    case 0x20210200: // 冻结时间   
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }
        rsd_info->start_mark = dt_buf_to_second(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;

        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }
        
        rsd_info->end_mark = dt_buf_to_second(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;
        
        // 协议搜索范围前闭后开 ，表内部读取驱动为前闭后闭 
        if ((rsd_info->start_mark == 0) && (rsd_info->end_mark == 0))
        {
            rsd_info->start_mark = 1; 
        }
        if (rsd_info->end_mark > 0)
        {
            rsd_info->end_mark--;
        }

        //fifoBuf_getDataPeek(fifo, p, 4);
        memcpy(p, apdu_rx + ctx->apdu_rx_offset, 4);
        
        if (*p != TYPE_null)
        {
            //周期性冻结  间隔必须与冻结周期单位一致,否则返回 DAR=3
            if ((oi <= 0x5007) && (oi >= 0x5001) && (oi != 0x5005))
            {
                //只支持秒，分，时，日, 月 间隔
                if ((p[0] != TYPE_TI) || (p[1] > 5))
                {
                    return -1;
                }

                //只有冻结支持冻结间隔搜索
                if ((oi < 0x5000) || (oi > 0x5011))
                {
                    return -1;
                }
                
                p++;
                value = buf_to_uint16_msb(p + 1);

                if (!(((p[0] == 1) && (oi == 0x5002)) || ((p[0] == 2) && (oi == 0x5003))
                    || ((p[0] == 3) && (oi == 0x5004)) || ((p[0] == 4) && (oi == 0x5006))
                    || ((p[0] == 0) && (oi == 0x5001)) || ((p[0] == 5) && (oi == 0x5007))))
                {
                    return -1;
                }

                rsd_info->interval = ti_to_second(*p, value);
                rsd_info->interval_special = 0;
                if ((p[0] == 4) && (oi == 0x5006) && (value != 0))    //月冻结月间隔特殊处理
                {
                    rsd_info->interval_special = value;
                }
            }
            else
            {
                rsd_info->interval = 0;
                rsd_info->interval_special = 0;
            }

            //fifoBuf_getData(fifo, p, 3); // 剪切3个字节
            //memcpy(p, apdu_rx + ctx->apdu_rx_offset, 3);
            ctx->apdu_rx_offset += 3;
        }

        //fifoBuf_getData(fifo, p, 1);// 剪切1个字节
        //*p = apdu_rx[ctx->apdu_rx_offset++];
        ctx->apdu_rx_offset++;//

        rsd_info->mode = 2;
        break;

    case 0x20230200: // 冻结记录序号     selector_2_seq()
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }
        
        rsd_info->start_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;
        
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }

        rsd_info->end_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;
        
        if ((rsd_info->start_mark == 0) && (rsd_info->end_mark == 0))
        {
            rsd_info->start_mark = 1;
        }
        if (rsd_info->end_mark > 0)
        {
            rsd_info->end_mark--;
        }

        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_null)
        {
            if (*p != TYPE_double_long_unsigned)
            {
                return -1;
            }
            rsd_info->interval = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
            ctx->apdu_rx_offset += 4;
        }

        rsd_info->mode = 1;

        break;

    default:
        return -1;
    }

    return (1);
}

int decode_rsd_event_selector_2(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存

    uint16_t value;
    uint32_t rsd_oad;
    uint16_t oi = get_OAD_OI(oad);

    memcpy(p, apdu_rx + ctx->apdu_rx_offset, 4);
    ctx->apdu_rx_offset += 4;    
    rsd_oad = buf_to_uint32_msb(p);

    switch (rsd_oad)
    {
    case 0x201e0200: // 事件发生时间
    case 0x20200200: // 事件结束时间 
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }
        rsd_info->start_mark = dt_buf_to_second(apdu_rx+ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;  
    
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_date_time_s)
        {
            return -1;
        }

        rsd_info->end_mark = dt_buf_to_second(apdu_rx+ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 7;  

        // 协议搜索范围前闭后开 ，表内部读取驱动为前闭后闭 
        if ((rsd_info->start_mark == 0) && (rsd_info->end_mark == 0))
        {
            rsd_info->start_mark = 1;
        }
        if (rsd_info->end_mark > 0)
        {
            rsd_info->end_mark--;
        }
        
        memcpy(p, apdu_rx + ctx->apdu_rx_offset, 4);
        
        if (*p != TYPE_null)
        {
            //只支持秒，分，时，日间隔
            if ((p[0] != TYPE_TI) || (p[1] > 5))
            {
                //return -1;
            }

            p++;
            value = buf_to_uint16_msb(p + 1);

            if ((p[0] == 4 || p[0] == 5) && (value != 0))
            {
                //return -1;
            }

            rsd_info->interval = ti_to_second(*p, value);
            memcpy(p, apdu_rx + ctx->apdu_rx_offset, 3);
            ctx->apdu_rx_offset += 4;
        }
        else
        {
            ctx->apdu_rx_offset += 1;
        }

        rsd_info->mode = 2;

        if (rsd_oad == 0x20200200)
        {
            rsd_info->mode = 3;
        }

        break;

    case 0x20220200: // 事件记录序号 
        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }

        rsd_info->start_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;

        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_double_long_unsigned)
        {
            return -1;
        }

        rsd_info->end_mark = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
        ctx->apdu_rx_offset += 4;

        if ((rsd_info->start_mark == 0) && (rsd_info->end_mark == 0))
        {
            rsd_info->start_mark = 1;
        }
        if (rsd_info->end_mark > 0)
        {
            rsd_info->end_mark--;
        }

        *p = apdu_rx[ctx->apdu_rx_offset++];
        if (*p != TYPE_null)
        {
            if (*p != TYPE_double_long_unsigned)
            {
                return -1;
            }

            rsd_info->interval = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
            ctx->apdu_rx_offset += 4;
        }

        rsd_info->mode = 1;

        break;


    default:
        return -1;
    }

    return (1);
}

int decode_rsd_selector_9(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存

    *p = apdu_rx[ctx->apdu_rx_offset++];
    rsd_info->last_no = *p;

    rsd_info->start_mark = 0;
    rsd_info->end_mark = 0xffffffffU;
    rsd_info->mode = 0;

    return (1);
}

uint8_t check_capture_exist(uint32_t rcsd_req, uint32_t rcsd_src)
{
    uint8_t type, attr_req, attr_src, index_req,index_src;
    uint16_t oi_req, oi_src;


    //常规
    if (rcsd_req == rcsd_src)
    {
        return true;
    }

    //特殊
    type = get_OAD_Class(rcsd_req);
    attr_req = get_OAD_Attribute(rcsd_req);
    index_req = get_OAD_Index(rcsd_req);
    oi_req = get_OAD_OI(rcsd_req);

    attr_src = get_OAD_Attribute(rcsd_src);
    index_src = get_OAD_Index(rcsd_src);
    oi_src = get_OAD_OI(rcsd_src);

    if ((type) == 0) //电能
    {
        if(oi_req == oi_src)
        {
            if (((attr_req == 2) || (attr_req == 4)) && ((attr_src == 2) || (attr_src == 4)))
            {
                if( index_req > 13 ) // 不存在的费率  // 或者改为 fee num 待调整
                {
                    return false;
                }
                
                if(index_req != index_src)
                {
                    if(index_src != 0)
                    {
                        return false;
                    }
                }

                return true;
            }
        }
    }

    return false;
}


//协议列选打包    rcsd_req:列选oad，   rcsd_src：原关联对象oad，   
//mode：0，不拷贝数据，只计算长度   1，  解析数据，并返回长度 
int _record_data_package_01(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src, uint8_t mode)
{
    uint32_t i, k, n;
    uint8_t *p_des = des, *p_src = src;
    int single_len, store_len, orig_len;
    uint8_t tmpbuf[200];//单个oad最大长度
    
    for(i=1; i < (rcsd_req[0] + 1); i++)
    {
        p_src = src;
        for(k=1; k < (rcsd_src[0] + 1); k++)
        {
            if (true == check_capture_exist(rcsd_req[i], rcsd_src[k]))
            //if (rcsd_req[i] == rcsd_src[k])  // 
            {
                //特殊rcsd长度处理
                single_len = record_change_format(instance, tmpbuf, p_src, rcsd_req[i], 0, RFC_GET_STORE_LEN);
                if (single_len < 0)
                {
                    //正常处理
                    single_len = nd_size(instance, rcsd_req[i]);
                }

                if (single_len > 0)
                {
                    // 个别数据仍需特殊处理
                    for (n = 0; n < single_len; n++) // 数据全f时 转空
                    {
                        if (p_src[n] != 0xff)
                        {
                            break;
                        }
                    }
                    if (n >= single_len)
                    {
                        if (mode)
                        {
                            *p_des = TYPE_null;
                        }
                        p_des++;
                        p_src += single_len;
                    }
                    else
                    {
                        orig_len = single_len;
                        if (mode)
                        {
                            orig_len = record_change_format(instance, tmpbuf, p_src, rcsd_req[i], rcsd_src[k], RFC_READ);
                            if (orig_len >= 0)
                            {
                                memcpy(p_des, tmpbuf, orig_len);
                            }
                            else
                            {
                                orig_len = single_len;
                                memcpy(p_des, p_src, single_len);
                            }

                        }
                        p_des += orig_len;
                        p_src += single_len;
                    }
                    
                }
                else
                {
                    if (mode)
                    {
                        *p_des = TYPE_null;
                    }
                    p_des++;
                    p_src++;
                }
                break;
            }
            else
            {
                single_len = record_change_format(instance, tmpbuf, p_src, rcsd_src[k], 0,  RFC_GET_STORE_LEN);
                if (single_len < 0)
                {
                    single_len = nd_size(instance, rcsd_src[k]);
                }

                if (single_len > 0)
                {
                    p_src += single_len;
                }
                else
                {
                    p_src++;
                }
            }
        }
        
        if (k >= (rcsd_src[0]+1))
        {
            if (mode)
            {
                *p_des = TYPE_null;
            }
            p_des++;
        }
    }  
    
    return (p_des - des);
}

//获取协议列选打包长度   rcsd_req:列选oad，   rcsd_src：原关联对象oad，   mode：0，不拷贝数据，只计算长度。  1，  解析数据，并返回长度 
int get_package_01_len(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src)
{
    return _record_data_package_01(instance,  des, src, rcsd_req, rcsd_src, 0);
}

//协议列选打包   rcsd_req:列选oad，   rcsd_src：原关联对象oad，   mode：0，不拷贝数据，只计算长度。  1，  解析数据，并返回长度 
int record_package_01(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src)
{
    return _record_data_package_01(instance,  des, src, rcsd_req, rcsd_src, 1);
}


//rsd 解析
int rsd_decode(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = instance->apdu_cache; //当临时缓存
    int ret;
    uint8_t dar;
    uint8_t oad_type = (uint8_t)(oad>>28);

    memset((uint8_t *)rsd_info, 0, sizeof(struct rsd_info_t));
    *p = apdu_rx[ctx->apdu_rx_offset++];
    rsd_info->select = *p;

    switch (rsd_info->select)
    {

    case 0: // NULL

        break;

    case 1: 
        // 周期性冻结方法1 需至少多搜索一个周期  // 待添加
        if (oad_type == 5)// 冻结
        {
            ret = decode_rsd_freeze_selector_1(instance, pipeline, oad, rsd_info);
        }
        else // 事件
        {
            ret = decode_rsd_event_selector_1(instance, pipeline, oad, rsd_info);
        }
        break;

    case 2: 
        if (oad_type == 5)
        {
            ret = decode_rsd_freeze_selector_2(instance, pipeline, oad, rsd_info);
        }
        else
        {
            ret = decode_rsd_event_selector_2(instance, pipeline, oad, rsd_info);
        }
        break;

    case 9: 
        ret = decode_rsd_selector_9(instance, pipeline, oad, rsd_info);
        break;

    default:
        break;
    }


    return ret;
}

//rcsd 解析   到rcsd_list[]. 第一个元素为 rcsd个数
int rcsd_decode(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad,  uint32_t *rcsd_list)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint8_t i;
    //int ret;

    //RCSD 个数
    rcsd_list[0] = apdu_rx[ctx->apdu_rx_offset++];

    if (rcsd_list[0] > REQ_RCSD_MAXNUM) // 请求的rcsd个数超出，返回错误 
    {
        // 待添加
        return -DAR_RW_REJECT;
    }
    else // 赋值rcsd  
    {
        if (rcsd_list[0] == 0) //全选
        {
            if(0x50020200 == oad)
            {
                return -DAR_RW_REJECT;
            }
            get_capture_list(instance, oad, rcsd_list, 3);

            p[cache->wr++] = rcsd_list[0];
            for (i = 0; i < rcsd_list[0]; i++)
            {
                p[cache->wr++] = 0; // rcsd:oad
                uint32_to_buf_msb(rcsd_list[1 + i], &p[cache->wr]);
                cache->wr += 4;
            }
        }
        else
        {
            p[cache->wr++] = rcsd_list[0];
            for (i = 0; i < rcsd_list[0]; i++)
            {
                p[cache->wr++] = apdu_rx[ctx->apdu_rx_offset++]; // csd
                memcpy(p + cache->wr, apdu_rx + ctx->apdu_rx_offset, 4);
                ctx->apdu_rx_offset += 4;    
                rcsd_list[1 + i] = buf_to_uint32_msb(&p[cache->wr]);
                cache->wr += 4;
            }
        }
    }
    
    return 1;
}

int record_response_null(uint8_t *resbuf)
{
    resbuf[0] = 0;
    return 1;
}

//rcsd请求的方案号处理，返回方案号 
uint8_t rcsd_req_schem_pro(uint32_t oad, uint32_t *rcsd_req)
{
    uint8_t i, schemNo = 0;
    //方案号 
    if((oad&0xFFFF0000) == 0x50020000) 
    {
        for(i = 1; i <= *(rcsd_req); i++ )
        {
            if((0x2021 != get_OAD_OI(*(rcsd_req + i)))  && (0x2023 != get_OAD_OI(*(rcsd_req + i))))
            {
                break;
            }
        }
        
        if (i > *(rcsd_req))
        {
            i = 1;      //找不到取第一个
        }
        
        schemNo = get_OAD_Schem((*(rcsd_req + i)));
        
        if (schemNo > 7)
        {
            schemNo = 0;
        }
        
        //修改 请求的方案号
        for(i = 1; i <= *(rcsd_req); i++ )
        {
            if((0x2021 == get_OAD_OI(*(rcsd_req + i))) || (0x2023 == get_OAD_OI(*(rcsd_req + i))))
            {
                *(rcsd_req + i) |=  (schemNo<<13);  // 分钟冻结 记录头 先加方案号
            }
        }
    }
    else // 其他 
    {
        schemNo = 0;
    }
    return schemNo;
}


//协议筛选该条记录是否符合检索时间区间或时间点，主要用于非周期性的时间检索
//入参src:记录内容            non_period
//0: 不满足检索范围的
//1: 方法1按时间检索条件满足
//2: 方法2按时间检索条件满足,或无需协议层筛选的都返回2; 
uint8_t select_record_time_check(uint8_t *src, uint8_t non_period, struct rsd_info_t *rsd_info)
{
    uint8_t *p, v, buf[7];
    uint16_t selec_mode;
    uint32_t row_start, row_end, cur_data;

    if (!non_period)
    {
        return 2; 
    }

    if ((rsd_info->mode != 2) && (rsd_info->mode != 3))
    {
         return 2; 
    }

    p = src;
    selec_mode = rsd_info->mode;
    row_start = rsd_info->start_mark;
    row_end = rsd_info->end_mark;

    v = 0;

    (selec_mode == 2)?(p += 6):(p += 11);  //发生时间或结束时间

    cur_data = buf_to_uint32_msb(p);

    if ((rsd_info->select == 1) && (cur_data == row_start) && (cur_data == row_end))
    {
        v = 1;
    }     
    if ((rsd_info->select == 2) && (cur_data >= row_start) && (cur_data <= row_end))
    {
        v = 2;
    }


    return v;

}

//获取间隔搜索的基值
uint32_t get_interval_basic_value(uint8_t *src, struct rsd_info_t *rsd_info)
{
    uint8_t *p, buf[8];
    uint32_t value = 0xffffffffU;

    p = src;

    switch (rsd_info->mode)
    {
    case 0x02: 
    case 0x03: 
        if (rsd_info->mode == 2)
        {
            p = src + 5;
        }
        else
        {
            p = src + 10;

            if (p[1] == 0xff)
            {
                p = src + 6;
            }
        }

        if (*p != TYPE_date_time_s)
        {
            break;
        }

       
        if ((rsd_info->interval_special) > 0)
        {
            //value = dl698_api_dt_buf_to_month(buf + 1);  //转月间隔 待调整
            //value = buf_to_uint32_msb(p + 1); 
        }
        else
        {
            value = buf_to_uint32_msb(p + 1);
        }

        break;

    case 0x01: // 按序号搜索 
        if (*p != TYPE_double_long_unsigned)
        {
            break;
        }

        value = buf_to_uint32_msb(p + 1);
        break;

    default: 
        break;
    }

    return value;
}


//读记录型数据打包
int record_data_package(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info)
{
#ifndef WIN32
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)rcsd_info; // 待定
    uint8_t *ptr_tmp = instance->apdu_cache; //需要替换协议共用缓存 2k  待调整   apdu_cache是否支持2k
    uint32_t rcsd_src[REQ_RCSD_MAXNUM];//待调整   设置的关联对象，用于与请求的关联对象作比对筛选
    uint16_t i, read_num, number = 0;
    uint16_t judge_size, one_len = 0,len = 0;
    uint8_t count = 0, rcsd_schem = 0, non_period = 0, check_result = 0;
    uint32_t oad_schem = 0, value = 0;
    uint8_t *p = (buf + 3); // 预留3个字节条数
    int ret = 0;
    
    //方案号 
    rcsd_schem = rcsd_req_schem_pro(oad, rcsd_req);
    oad_schem = oad | (rcsd_schem<<13);
    ret = get_record_id_info(instance, oad_schem, &record_id_info);
    if(ret < 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    non_period = frz_non_period_type_judge((uint16_t)(oad>>16));

    //---获取条数
    
    circular_info.start_mark = rsd_info->start_mark;
    circular_info.end_mark = rsd_info->end_mark;
    circular_info.mode =   rsd_info->mode;

    switch(rsd_info->select)
    {
        case 0:
            //ret = GET_record_sel_0(instance, pipeline, mesg);
            //circular_info.curr_num--;
            break;
            
        case 1:
        case 2:
            if((1 == non_period) && ((rsd_info->mode == 2) || (rsd_info->mode == 3))) //方法1/2 非周期性类 按时间搜索时 由协议筛选
            {
                circular_info.mode = 0;
            }
            ret = nd_log_output(instance, record_id_info.save_index,  ptr_tmp, 1, &circular_info);
            circular_info.mode =   rsd_info->mode; 
            break;
            
        case 9: //上n次
            ret = nd_log_output(instance, record_id_info.save_index,  ptr_tmp, 1, &circular_info);
            if ((rsd_info->last_no == 0) || (rsd_info->last_no > circular_info.valid_num))
            {
                return record_response_null(buf);
            }
            circular_info.curr_num = circular_info.valid_num - rsd_info->last_no;
            circular_info.valid_num = circular_info.curr_num + 1;
            break;
        default:
            break;
    }

    if (circular_info.valid_num == 0)
    {
        return record_response_null(buf);
    }
    //------------
    
    //获取关联对象
    get_capture_list(instance, oad_schem, rcsd_src, 3);

    check_result = 0;
    read_num = circular_info.valid_num;
    for (i = circular_info.curr_num; i < read_num; i++)
    {
        //获取1条记录内容
        ret = nd_log_output(instance, record_id_info.save_index,  ptr_tmp, circular_info.unit_size, &circular_info);
        if(ret < 0)
        {
            return -DAR_OTHERS_ERROR;
        }
        

        //-----长度判断
        if (one_len == 0)
        {   
            //预算打包长度
            one_len = get_package_01_len(instance, p+len, ptr_tmp, rcsd_req, rcsd_src);
        }
        
        if (check_result == 1) //非周期性 方法1 按时间检索 若有多条，返回记录序号最大的一条  
        {
            judge_size = p - buf + one_len;
        }
        else
        {
            judge_size = (p - buf + len + one_len);
        }

        if((judge_size) > 2000) //待调整
        {
            return -DAR_BUFFER_OVERFLOW;
        }
        //-----------

        //----协议筛选
        if (rsd_info->interval_special)
        {
            rsd_info->interval = rsd_info->interval_special;
        }
        if (rsd_info->interval > 0)
        {
            value = get_interval_basic_value(ptr_tmp, rsd_info);
            
            if (number == 0)
            {
                circular_info.start_mark = value;
            }
            
            if (value == circular_info.start_mark)
            {
                one_len = record_package_01(instance, p+len, ptr_tmp, rcsd_req, rcsd_src);
                len += one_len;
                number++;
                circular_info.start_mark += rsd_info->interval;
            }
            else if (value > circular_info.start_mark)
            {
                if (((value - circular_info.start_mark) % rsd_info->interval) == 0)
                {
                    one_len = record_package_01(instance, p+len, ptr_tmp, rcsd_req, rcsd_src);
                    len += one_len;
                    number++;
                    circular_info.start_mark = value + rsd_info->interval;
                }
                else
                {
                    // 方法2读取 周期性冻结 返回应为 以搜索范围内第一条时间点作为起始时间 + 整数倍间隔时间
                    circular_info.start_mark += (((value - circular_info.start_mark) / rsd_info->interval) + 1) * rsd_info->interval;
            
                    // 方法2读取 周期性冻结 返回应为 读取起始时间 + 整数倍间隔时间
                    //circular_info.start_mark = circular_info.start_mark + rsd_info->interval;  
                }
            }
        }
        else
        {
            //协议判断是否符合要求
            check_result = select_record_time_check(ptr_tmp, non_period, rsd_info);
            if ( check_result > 0 )
            {
                if ((check_result == 1) && (number >= 1)) // 至少已搜索到一条
                {
                    len -= one_len; // 
                }
            
                //开始打包
                one_len = record_package_01(instance, p+len, ptr_tmp, rcsd_req, rcsd_src);
                len += one_len;
                number++;
            }
        }

    }      
        
    count = axdr_len_encode(buf, number);

    if (count < 3)
    {
        ndv_memcopy(buf + count, p, len);
    }
    len += count;
    
    //读取事件时 确认上报
    if ((oad & 0xF0000000) == 0x30000000) //待调整  首次读取？ 
    {
        report_3320list_confirmed(instance, chl_PLC_698, oad);
    }
    return (int)len;
#endif
}
//GS   暂存
/////////////////////////////////////////////////
/////////////////////////////////////////////////

