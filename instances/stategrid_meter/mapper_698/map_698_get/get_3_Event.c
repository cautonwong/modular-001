#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"



// 获取事件关联对象
int get_event_obj(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{

    uint8_t i, num, maxnum, scheme;
    uint8_t tempbuf[200];
    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t CaptureLen;
    uint32_t CaptureId;
    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -1;
    }

    CaptureId = record_id_info.capture_index;
    CaptureLen = record_id_info.capture_len;// 
    maxnum = (CaptureLen - 1) / 4;

    nd_get_index(instance, CaptureId, &tempbuf[0], sizeof(tempbuf));
    oadlist = tempbuf;
    num = *oadlist++;

    *p++ = TYPE_array; //TYPE_array

    if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    *p++ = num;
    for (i = 0; i < num; i++)
    {
        *p++ = TYPE_OAD;
        mem_swap_cpy(p, oadlist, 4);
        oadlist += 4;
        p += 4;
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}

// 获取事件当前记录数
int get_current_record_number(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{

    struct circular_info_t circular_info = { 0 };
    uint8_t temp[200]; 
    uint8_t* p = buf;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    circular_info.start_mark = 0;
    circular_info.end_mark = 0xFFFFFFFF;
    circular_info.mode = 0;

    ret = nd_log_output(instance, record_id_info.save_index, temp, 1, &circular_info);

    *p++ = TYPE_long_unsigned;
    p += uint16_to_buf_msb(circular_info.valid_num, p);

    *dar = DAR_SUCCESS;
    return (p - buf);
}


int get_current_record_number_ic_24(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t  i, num;
    uint16_t tims;

    struct circular_info_t circular_info = { 0 };
    uint8_t tempbuf[200];
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;

    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);

    if (index != 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }


    //需区分个数
    num = 1; //单相固定为1
    *p++ = TYPE_structure;
    *p++ = 4;
    if ((num == 3) || (num == 1))
    {
        //p += 3;
        *p++ = 0;
    }


    tims = 0;
    for (i = 0; i < num; i++)
    {
        circular_info.start_mark = 0;
        circular_info.end_mark = 0xFFFFFFFF;
        circular_info.mode = 0;

        ret = nd_log_output(instance, record_id_info.save_index, tempbuf, 1, &circular_info);
        if (ret <= 0)
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }
        
        *p++ = TYPE_long_unsigned;
        tims += circular_info.valid_num;
        p += uint16_to_buf_msb(circular_info.valid_num, p);
        //get record_id_info.save_index
    }

    //单相
    if (num == 1)
    {
        *p++ = 0;
        *p++ = 0;
    }
   
    *dar = DAR_SUCCESS;
    return (p - buf);
}

// 获取事件最大记录数
int get_max_record_number(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t* p = buf;
    uint16_t OI = get_OAD_OI(oad);
    uint16_t max_num = 10;

    if ((OI == 0x3011) || (OI == 0x303C))
    {
        max_num = 100;
    }

    *p++ = TYPE_long_unsigned;
    p += uint16_to_buf_msb(max_num, p);

    *dar = DAR_SUCCESS;
    return (p - buf);
}
int get_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    struct event_para_pack_t para_pack;
    uint8_t* p;
    uint16_t ret;
    uint8_t i, count = 0, index = (uint8_t)oad;
    p = buf;

    memset((uint8_t*)&para_pack, 0, sizeof(struct event_para_pack_t));

    ret = get_event_para_pack(instance, oad, &para_pack);
    if (ret < 0)
    {
        *p++ = TYPE_structure;
        *p++ = 0;
        return (p - buf);
    }

    count = para_pack.para_num;

    if (index > count)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (index == 0)
    {
        *p++ = TYPE_structure;
        *p++ = count;
        for (i = 0; i < count; i++)
        {
            *p++ = para_pack.event_para[i].tag;
            memcpy(p, (uint8_t*)para_pack.event_para[i].ctrlblock_para, para_pack.event_para[i].datalen);
            p += para_pack.event_para[i].datalen;
        }
    }
    else
    {
        i = index - 1;
        *p++ = para_pack.event_para[i].tag;
        memcpy(p, (uint8_t*)para_pack.event_para[i].ctrlblock_para, para_pack.event_para[i].datalen);
        p += para_pack.event_para[i].datalen;
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}

uint8_t get_current_value(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar )
{
    uint32_t sequence;
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);
   

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (OI == 0x3023) //
    {
        return -1;
    }
    else
    {
        if ((index > 1))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        if (index == 0)
        {
            *p++ = TYPE_array;
            *p++ = 1;
        }

        *p++ = TYPE_structure;
        *p++ = 2;

        //事件发生源  除特殊事件外
        *p++ = 0;
     
        *p++ = TYPE_structure;
        *p++ = 2;

        //事件发生次数
        *p = TYPE_double_long_unsigned;

        nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));

        *p++ = TYPE_double_long_unsigned;
        p += uint32_to_buf_msb(sequence, p);

        //事件累计时间/
        *p++ = TYPE_double_long_unsigned;

        p += uint32_to_buf_msb(*record_id_info.acc_time, p);
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}
int get_current_value_one_element_ic_24(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar, void* current_value)
{
    uint32_t sequence;
    struct circular_info_t circular_info = { 0 };
    uint8_t temp[200];
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);

    struct current_value_t
    {
        uint32_t cnts;    //发生总次数 
        uint32_t times;   //发生总累计时间 
    };

    struct current_value_t* value = (struct current_value_t*)current_value;


    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    *p++ = TYPE_structure;
    *p++ = 2;

    //事件发生次数
    nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));

    *p++ = TYPE_double_long_unsigned;
    p += uint32_to_buf_msb(sequence, p);

    //事件累计时间
    *p++ = TYPE_double_long_unsigned;

    p += uint32_to_buf_msb(*record_id_info.acc_time, p);
    value->cnts = sequence;
    value->times = *record_id_info.acc_time;

    *dar = DAR_SUCCESS;
    return (p - buf);
}
int get_current_value_ic_24(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint32_t sequence, time;
    uint8_t tempbuf[200];
    uint8_t* p = buf;
    int ret;
    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);

    uint8_t i, num;
    uint32_t oad_temp = oad;
    struct current_value_t
    {
        uint32_t cnts;    // 发生总次数 
        uint32_t times;   // 发生总累计时间 
    };
    struct current_value_t record, datt;


    //单相固定  
    num = 1;

    if (index > 4)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (index == 0)
    {
        *p++ = TYPE_array;
        *p++ = 4;

        if ((num == 3) || (num == 1))
        {
            *p++ = 0;       //第一个元素补空
        }

        memset((uint8_t*)&record, 0,  sizeof(struct current_value_t));

        for (i = 0; i < num; i++)
        {
            memset((uint8_t*)&datt, 0,  sizeof(struct current_value_t));

            ret = get_current_value_one_element_ic_24(instance, oad_temp, p, dar, &datt);

            record.cnts += datt.cnts;
            record.times += datt.times;

            if (ret < 0)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }

            p += ret;
            oad_temp += (0x00000001 << 13);
        }

        if (num == 1)
        {
            *p++ = 0;
            *p++ = 0;
        }

    }
    else
    {
        if (num == 4)
        {
            oad_temp += index * (0x00000001 << 13);
            ret = get_current_value_one_element_ic_24(instance, oad_temp, p, dar, &datt);

            if (ret < 0)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }

            p += ret;
        }
        else
        {

            if (((num == 3) && (index == 1)) || ((num == 1) && (index != 2)))
            {
                *p++ = 0;
            }
            else
            {
                oad_temp += index * (0x00000001 << 13);
                
                ret = get_current_value_one_element_ic_24(instance, oad_temp, p, dar, &datt);

                if (ret < 0)
                {
                    *dar = DAR_OTHERS_ERROR;
                    return -1;
                }

                p += ret;
            }
        }
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}


uint8_t get_time_status_record(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint32_t sequence, time;
    struct circular_info_t circular_info = { 0 };
    uint8_t tempbuf[200];
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);
    uint32_t start_t = 0xFFFFFFFF, end_t = 0xFFFFFFFF;
    uint8_t time_buf[2][8], i;
 
    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (OI == 0x3023)
    {
 
    }
    else
    {
        //事件发生源
        if ((index > 1))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        if (index == 0)
        {
            *p++ = TYPE_array;
            *p++ = 1;
        }

        *p++ = TYPE_structure;
        *p++ = 2;

        //事件发生源  除特殊事件外
        *p++ = 0;

        *p++ = TYPE_structure;
        *p++ = 2;

        //最近一次发生时间、结束时间
        circular_info.start_mark = 0;
        circular_info.end_mark = 0xFFFFFFFF;
        circular_info.mode = 0;

        ret = nd_log_output(instance, record_id_info.save_index, tempbuf, 1, &circular_info);
        if (ret <= 0)
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        *p = TYPE_date_time_s;
         memset(time_buf, 0xff, sizeof(time_buf));


        if (circular_info.valid_num > 0)
        {
            circular_info.curr_num = circular_info.valid_num - 1;

            ret = nd_log_output(instance, record_id_info.save_index, tempbuf, sizeof(tempbuf), &circular_info);
            if (ret < 0)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }

            start_t = buf_to_uint32_msb(&tempbuf[5+1]);
            end_t = buf_to_uint32_msb(&tempbuf[10+1]);
            second_to_dt_buf(start_t, (p + 1));

            memcpy(&time_buf[0][0], p, 8); //start

            if (end_t == 0xFFFFFFFF)//上 1 次事件未结束，则取上 2 次事件的结 束时间；
            {
                if (circular_info.valid_num > 1)
                {
                    circular_info.curr_num = circular_info.valid_num - 2;
                    ret = nd_log_output(instance, record_id_info.save_index, tempbuf, sizeof(tempbuf), &circular_info);
                    if (ret >= 0)
                    {
                        if (end_t != 0xFFFFFFFF)
                        {
                            second_to_dt_buf(end_t, (p + 1));
                            memcpy(&time_buf[1][0], p, 8);
                        }
                    }
                }
            }
            else
            {
                second_to_dt_buf(end_t, (p + 1));
                memcpy(&time_buf[1][0], p, 8);
            }
        }

        for (i = 0; i < 2; i++)
        {
            if (time_buf[i][0] == TYPE_date_time_s)
            {
                memcpy(p, &time_buf[i][0], 8);
                p += 8;
            }
            else
            {
                *p++ = 0;
            }
        }
    }

    *dar = DAR_SUCCESS;
    return (p - buf);

}

int get_time_status_record_one_element_ic_24(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint32_t sequence, time;
    struct circular_info_t circular_info = { 0 };
    uint8_t tempbuf[200];
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);
    uint32_t start_t = 0xFFFFFFFF, end_t = 0xFFFFFFFF;
    uint8_t time_buf[2][8], i;


    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }


    *p++ = TYPE_structure;
    *p++ = 2;

    //最近一次发生时间、结束时间
    circular_info.start_mark = 0;
    circular_info.end_mark = 0xFFFFFFFF;
    circular_info.mode = 0;

    ret = nd_log_output(instance, record_id_info.save_index, tempbuf, 1, &circular_info);
    if (ret <= 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    *p = TYPE_date_time_s;
    memset(time_buf, 0xff, sizeof(time_buf));


    if (circular_info.valid_num > 0)
    {
        circular_info.curr_num = circular_info.valid_num - 1;

        ret = nd_log_output(instance, record_id_info.save_index, tempbuf, sizeof(tempbuf), &circular_info);
        if (ret < 0)
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }
        start_t = buf_to_uint32_msb(&tempbuf[5 + 1]);
        end_t = buf_to_uint32_msb(&tempbuf[10 + 1]);
        second_to_dt_buf(start_t, (p + 1));

        memcpy(&time_buf[0][0], p, 8); //start

        if (end_t == 0xFFFFFFFF)//上 1 次事件未结束，则取上 2 次事件的结 束时间；
        {
            if (circular_info.valid_num > 1)
            {
                circular_info.curr_num = circular_info.valid_num - 2;
                ret = nd_log_output(instance, record_id_info.save_index, tempbuf, sizeof(tempbuf), &circular_info);
                if (ret >= 0)
                {
                    if (end_t != 0xFFFFFFFF)
                    {
                        second_to_dt_buf(end_t, (p + 1));
                        memcpy(&time_buf[1][0], p, 8);
                    }
                }
            }
        }
        else
        {
            second_to_dt_buf(end_t, (p + 1));
            memcpy(&time_buf[1][0], p, 8);
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (time_buf[i][0] == TYPE_date_time_s)
        {
            memcpy(p, &time_buf[i][0], 8);
            p += 8;
        }
        else
        {
            *p++ = 0;
        }
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}

int get_time_status_record_ic_24(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    //uint32_t sequence, time;
   // struct circular_info_t circular_info = { 0 };
    uint8_t* p = buf;
    //struct record_storage_id_info_t  record_id_info;
    int ret;
    //uint16_t OI = get_OAD_OI(oad);
    uint8_t index = (uint8_t)(oad);
    //uint32_t start_t = 0xFFFFFFFF, end_t = 0xFFFFFFFF;
    uint8_t  i;


    uint32_t oad_temp = oad;
    uint8_t num;


    num = 1; //单相固定1 ，三相根据不同事件 特殊处理

    if ((index > 4))
    {
        return DAR_OTHERS_ERROR;
    }

    if (index == 0)//支持块
    {
        *p++ = TYPE_array;
        *p++ = 4;


        if ((num == 3) || (num == 1))
        {
            *p++ = 0;       //第一个元素补空
        }


        for (i = 0; i < num; i++)
        {
            ret = get_time_status_record_one_element_ic_24(instance, oad_temp, p, dar);

            if (ret < 0)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }

            p += ret;
            oad_temp += (0x00000001 << 13);
        }

        if ((num == 1))
        {
            *p++ = 0;
            *p++ = 0;
        }

    }
    else
    {
        if (num == 4)
        {
            oad_temp += index * (0x00000001 << 13);
            ret = get_time_status_record_ic_24(instance, oad_temp, p, dar);

            if (ret < 0)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }

            p += ret;
        }
        else
        {

            if (((num == 3) && (index == 1)) || ((num == 1) && (index != 2)))
            {
                *p++ = 0;
            }
            else
            {
                oad_temp += index * (0x00000001 << 13);
                ret = get_time_status_record_ic_24(instance, oad_temp, p, dar);

                if (ret < 0)
                {
                    *dar = DAR_OTHERS_ERROR;
                    return -1;
                }

                p += ret;
            }

        }
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}


int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t tempbuf[200];
    uint8_t* src = tempbuf;
    uint16_t i, len = 0;
    uint16_t OI;
    uint8_t attr, class = 7;
    uint8_t dar = DAR_SUCCESS;
    uint32_t index;
    int ret;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }

    OI = get_OAD_OI(id);
    attr = get_OAD_Attribute(id);

    if (((OI >= 0x3000) && (OI <= 0x3008)) || (OI == 0x300B))
    {
        class = 24;
    }

    switch(id)
    {
        case 0x33000200: 
        {
            *p++ = TYPE_null;
            return (p-buf);
        }
        break;
        case 0x33020206: //事件编程   
        {
            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MeterWr_List);
            len = 52;
            nd_get_index(instance, index, p, len);
            p += len;
            return (p-buf);
        }
        break;
        case 0x330C0206: //事件清零 
        {

            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + EventClr_List);
            len = 7;
            nd_get_index(instance, index, p, len);
            p += len;
            return (p-buf);
        }

        case 0x33140206: //编程时段表对象  事件记录用
        {

            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Set_DayTab_Oad);
            len = 5;
            nd_get_index(instance, index, p, len);
            p += len;
            return (p - buf);
        }
        break;

        case 0x33140207: //编程前时段表 事件记录用
        {
            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_DayTab);
            ret = nd_get_index(instance, index, p, 114);
            if ((ret < 0) || (p[1] > 14) || (p[0] != TYPE_array))// 错误
            {
                *p++ = 0;
            }
            else
            {
                p += (p[1] * 8 + 2);
            }
            return (p - buf);
        }
        break;
        case 0x33150206: //编程节假日对象  事件记录用
        {
            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Set_Holiday_Oad);
            len = 5;
            nd_get_index(instance, index, p, len);
            p += len;
            return (p - buf);
        }
        break;
        case 0x33150207: //编程前节假日内容 事件记录用  
            index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_Holiday);
            ret = nd_get_index(instance, index, p, 10);
            if ((ret < 0) || (p[0] != TYPE_structure))// 错误
            {
                *p++ = 0;
            }
            else
            {
                p += 10;
            }
            return (p - buf);

        default:
            break;
    }
    
    switch (class)
    {
        case 7://class7
        {
            switch (attr) //属性
            {
                case 3:// 关联对象
                {
                    return get_event_obj(instance, id, p, &dar);
                }
                break;

                case 4: // 当前记录数
                {
                    return get_current_record_number(instance, id, p, &dar);
                }
                break;
                case 5: // 最大记录数
                {    
                    return get_max_record_number(instance, id, p, &dar);
                }
                break;

                case 6: // 配置参数 
                {
                    return get_struct_event_para_t(instance, id, p, &dar);
                }
                break;
                case 7: // 当前值记录表 
                {
                    return get_current_value(instance, id, p, &dar);
                }
                break;
                case 8:  // 上报标识  //待添加
                {
                    //return current_value_get(instance, id, p, &dar);
                }
                break;
                case 11: // 上报方式 //待添加
                {
                    //return current_value_get(instance, id, p, &dar);
                }
                break;
                case 29: // 非法插卡总次数 
                {   //return dl698_normal_data_get(element, req, res);


                }
                break;
                case 9: // 有效标识 固定为1
                {
                    *p++ = TYPE_bool;
                    *p++ = 1;
                    return (p - buf);
                }
                break;
                case 10:
                {
                    return get_time_status_record(instance, id, p, &dar);
                }
                break;


                default:
                    break;
            }
        }
        break;
        case 24://类24
        {
            switch (attr) //属性
            {
                case 2: // 关联对象属性表 
                {
                    return get_event_obj(instance, id, p, &dar);
                }
                break;
                case 3: // 当前记录数 
                {
                    return get_current_record_number_ic_24(instance, id, p, &dar);
                }
                break;
                case 4: // 最大记录数 
                {
                    return get_max_record_number(instance, id, p, &dar);
                }
                break;
                case 5: // 配置参数 
                case 13:// 待调整
                {
                    return get_struct_event_para_t(instance, id, p, &dar);
                }
                break;

                case 10: // 当前值记录表 
                {
                    return get_current_value_ic_24(instance, id, p, &dar);
                }
                break;
                case 11: // 上报标识 
                //case 12: // 有效标识 
                case 15:// 上报方式 
                    //return dl698_normal_data_get(element, req, res);

                case 12: // 有效标识 固定1
                {
                    *p++ = TYPE_bool;
                    *p++ = 1;
                    return (p - buf);
                }
                break;

                case 14: //
                {
                    return get_time_status_record_ic_24(instance, id, p, &dar);
                }
                break;
            }
        }
        break;
        default:
            break;
    }


#if 0
    case 0x30050200://关联对象属性表
    {
        uint8_t num,maxnum;
        uint32_t CaptureId, index;
        
        CaptureId = OverCurrentAS_oad;
        maxnum = ((13*init_30050200[1]+9)/10);

        index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + CaptureId);
        nd_get_index(instance, index, &tempbuf[0], sizeof(tempbuf));
        src = tempbuf;
        num = *src++;

        *p++ = TYPE_array; //TYPE_array
        
        if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
        {
            return DAR_OTHERS_ERROR;
        }
        
        *p++ = num;
        for (i = 0; i < num; i++)
        {
            *p++ = TYPE_OAD;
            mem_swap_cpy(p, src, 4);
            src += 4;
            p += 4;            
        }

        return (p - buf);
    }
    case 0x30110300://当前记录数
    case 0x30050300://当前记录数
    {
        return 0;
    }
    case 0x30050400://最大记录数
    {
        return 0;
    }
    case 0x30050500://配置参数 过流阈值
    {
        *p++ = TYPE_structure;
        *p++ = 2;
        *p++ = TYPE_double_long;
        memcpy(p, (uint8_t*)&mbase_cb->i_over_event_threshold, 0x04);
        p += 4;
        *p++ = TYPE_unsigned;;
        memcpy(p, (uint8_t*)&mbase_cb->i_over_event_duration_para, 0x01);
        p++;
        return (p - buf);
    }

    case 0x30050B00://上报标识
    {
        *p++ = TYPE_enum;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_type, p, 0x01);
        p++;
        return (p - buf);
    }
    case 0x30050C00://有效标识   固定1
    {
        *p++ = TYPE_bool;
        *p++ = 1;
        return (p - buf);
    }
    case 0x30050E00://时间状态记录表
    {
        return 0;
    }
    case 0x30050F00://上报方式
    {
        *p++ = TYPE_enum;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }


    default:
        break;
    }
#endif

    return -1;
}
