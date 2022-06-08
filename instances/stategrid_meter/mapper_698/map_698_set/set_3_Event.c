#include "nidavellir.h"
#include "fr_698.h"


// 698设置事件上报标识或上报方式   mode  0: 上报标识      1:上报方式
int set_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t mode)
{
    uint8_t tempbuf[200] = { 0 };
    uint8_t* p = buf;
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint32_t AddrID = 0;
    uint8_t index = (uint8_t)oad;

    if ((index != 0) || (*p++ != TYPE_enum))
    {
        return -DAR_OTHERS_ERROR;
    }

    tempbuf[0] = *p++;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    if (mode == 0)
    {
        AddrID = record_id_info.rpt_flag;
        if (tempbuf[0] > 3)
        {
            return -DAR_OTHERS_ERROR;
        }
    }
    else //if (mode == 1)
    {
        AddrID = record_id_info.rpt_mode;
        if (tempbuf[0] > 1)
        {
            return -DAR_OTHERS_ERROR;
        }

    }

    nd_set_index(instance, AddrID, &tempbuf[0], 1);

    
    EventOut_698_check(instance);  
    return (p - buf);
}


//  关联对象检查  待添加
uint8_t oad_valid_check(uint32_t oad)
{
    return true;
}


// 设置事件关联对象
int set_event_obj(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{

    uint8_t i, num, maxnum;//, scheme;
    uint8_t tempbuf[200];
//    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t CaptureLen;
    uint32_t CaptureId;
    uint32_t oad_tmp;

    struct record_storage_id_info_t  info;
    int ret;

    ret = get_record_id_info(instance, oad, &info);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OBJECT_NOT_EXIST;
    }

    CaptureId = info.capture_index;
    CaptureLen = info.capture_len;// 
    maxnum = (CaptureLen - 1) / 4;

    // tag 判断  待添加

    p++; //TYPE_array
    num = *p++;
    if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
    {
        return DAR_OTHERS_ERROR;
    }

    *oadlist++ = num;
    for (i = 0; i < num; i++)
    {
        p++; // DTC_OAD
        oad_tmp = buf_to_uint32_msb(p);
        if (false == oad_valid_check(oad_tmp))  //无效关联对象
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
        memcpy(oadlist, p, 4);
        oadlist += 4;
        p += 4;
    }

    nd_set_index(instance, info.capture_index, &tempbuf[0], (1 + num * 4));

    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    
    report_3320list_confirmed_All_channel(instance, oad);
    
    return (p - buf);
}

int set_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    struct mbase_control_block_t* mbase_cb= (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct event_para_pack_t para_pack;
    uint8_t* p;
    int16_t ret;
    uint8_t i, count = 0, index = (uint8_t)oad;
    uint16_t s_len;
    uint32_t s_index=0;
    
    p = buf;

    memset((uint8_t*)&para_pack, 0, sizeof(struct event_para_pack_t));

    ret = get_event_para_pack(instance, oad, &para_pack);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    count = para_pack.para_num;

    if (index > count)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    if (index == 0)
    {
        if ((p[0] != TYPE_structure) || (p[1] != count))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }

        p += 2;

        for (i = 0; i < count; i++)
        {
            if ((*p) != para_pack.event_para[i].tag)
            {
                *dar = DAR_OTHERS_ERROR;
                return -DAR_OTHERS_ERROR;
            }
            p++;

            mem_swap_cpy((uint8_t*)para_pack.event_para[i].ctrlblock_para, p, para_pack.event_para[i].datalen);
            p += para_pack.event_para[i].datalen;
        }
    }
    else
    {
        i = index - 1;

        if ((*p) != para_pack.event_para[i].tag)
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
        p++;

        mem_swap_cpy((uint8_t*)para_pack.event_para[i].ctrlblock_para, p, para_pack.event_para[i].datalen);
        p += para_pack.event_para[i].datalen;
    }

    
    // 事件参数 全部更新// 待调整
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, OverCurrent_threshold));
    s_len = offsetof(struct mbase_control_block_t, ZeroLineErrS_duration_para) - offsetof(struct mbase_control_block_t, OverCurrent_threshold);
    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->OverCurrent_threshold, s_len);  
    
    
    *dar = DAR_SUCCESS;
    return (p - buf);
}



int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint16_t i;
    uint8_t tempbuf[200];
    uint8_t *des = tempbuf;
    uint16_t OI;
    uint8_t attr, index, class;
    uint8_t dar = 0;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }


    OI = get_OAD_OI(id);
    attr = get_OAD_Attribute(id);
    index = (uint8_t)(id & 0x000000FF);

    class = 7;
    if (((OI >= 0x3000) && (OI <= 0x3008)) || (OI == 0x300B))
    {
        class = 24;
    }

    switch (class)
    {
    case 7://class7
    {
        switch (attr) //属性
        {
            case 3:// 关联对象
            {
                return set_event_obj(instance, id, p, &dar);
            }
            break;

            case 6: // 配置参数 
            {
                return set_struct_event_para_t(instance, id, p, &dar);
            }
            break;

            case 8:  // 上报标识  
            {
                return set_event_report_flag_mode(instance, id, p, 0);
            }
            break;
            case 11: // 上报方式 
            {
                return set_event_report_flag_mode(instance, id, p, 1);
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
                return set_event_obj(instance, id, p, &dar);
            }
            break;
            case 5: // 配置参数 
            {
                return set_struct_event_para_t(instance, id, p, &dar);
            }
            break;

            case 11: // 上报标识 
            {
                return set_event_report_flag_mode(instance, id, p, 0);
            }
            break;
            case 15:// 上报方式 
            {
                return set_event_report_flag_mode(instance, id, p, 1);
            }
            break;
            default:
                break;

        }
    }
    break;
    default:
        break;
    }


    return -DAR_OBJECT_UNDEF;
}
