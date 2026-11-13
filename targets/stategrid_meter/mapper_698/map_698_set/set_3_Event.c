#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "fr_698.h"
#include "fr_645_report.h"
#include "cpu_surport.h"
#include "domain/events/legacy/gs_record.h"
#include "domain/freeze/legacy/freeze_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
#include "targets/stategrid_meter/mapper_698/resource_map.h"

// 设置事件上报标识或上报方式   mode  0: 上报标识      1:上报方式 (无tag)
int set_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t value, uint8_t mode)
{
    struct record_storage_id_info_t  record_id_info;
    int ret;
    uint32_t AddrID = 0;
    uint8_t index = (uint8_t)oad;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    if (mode == 0)
    {
        AddrID = record_id_info.rpt_flag;
        if (value > 3)
        {
            return -DAR_OTHERS_ERROR;
        }
    }
    else //if (mode == 1)
    {
        AddrID = record_id_info.rpt_mode;
        if (value > 1)
        {
            return -DAR_OTHERS_ERROR;
        }

    }

    ret = nd_set_index(instance, AddrID, &value, 1);
    if(ret <= 0)
    {
        return -DAR_OTHERS_ERROR;
    }

    return 1;
    
}



// 698协议 设置事件上报标识或上报方式   mode  0: 上报标识      1:上报方式
int fr_set_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t mode)
{
    uint8_t tempbuf[20] = { 0 };
    uint8_t* p = buf;
    int ret;
    uint32_t AddrID = 0;
    uint8_t index = (uint8_t)oad;

    if ((index != 0) || (*p++ != TYPE_enum))
    {
        return -DAR_OTHERS_ERROR;
    }

    tempbuf[0] = *p++;

    ret = set_event_report_flag_mode(instance,  oad,  tempbuf[0],  mode);

    if (ret <= 0)
    {
        return ret;
    }

    if(mode == 0) // 注意2015的上报方式调用 
    {   
        report_3320flag_set_from_698(instance, oad, tempbuf[0]);
    }

    EventOut_698_check(instance);  
    return (p - buf);
}



// 设置事件关联对象
int set_event_obj(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t i, num, maxnum, Schem;
    uint8_t tempbuf[200]; //oadlist
//    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t CaptureLen;
//    uint32_t CaptureId;
    uint32_t oad_tmp;
    uint8_t* apdu_cache = instance->apdu_cache; 

    struct record_storage_id_info_t  info;
    int ret;

    
    if (get_OAD_Index(oad) != 0)
    {
        return -DAR_RW_REJECT;
    }

    if ((get_OAD_OI(oad) == 0x3013) && ( nd_mbase_key_state(mbase_cb) == false ))
    {
        *dar = DAR_RW_REJECT;
        return -DAR_RW_REJECT;
    }

    ret = get_record_id_info(instance, oad, &info);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OBJECT_NOT_EXIST;
    }

//    CaptureId = info.capture_index;
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

        Schem = get_OAD_Schem(oad_tmp);
#if 1 // vs 要求
        if ((Schem < 1) || (Schem > 4))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
#endif

        ret = check_metertype_OI_valid(instance, get_OAD_OI(oad_tmp));
        if(ret < 0){
            return -DAR_RW_REJECT;
        }

        ret = nd_size(instance, apdu_cache, ignore_OAD_Schem(oad_tmp)); 
        if(ret < 0){
            return -DAR_RW_REJECT;
        }

        memcpy(oadlist, p, 4);
        oadlist += 4;
        p += 4;
    }

    ret = cal_record_size_sectors(instance,tempbuf, __NULL, oad);
    if (ret <= 0)
    {
        return -DAR_BUFFER_OVERFLOW;
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

    if ((oad & 0xFFFF0000)== 0x30110000) //掉电 配置参数 不支持设置
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

    
    // 事件参数 全部更新
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    nd_mbase_offset_OverCurrent_threshold());
    s_len = nd_mbase_offset_ZeroLineErrS_duration_para() - nd_mbase_offset_OverCurrent_threshold() + sizeof(nd_mbase_ZeroLineErrS_duration_para(mbase_cb));
    nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_OverCurrent_threshold(mbase_cb), s_len);  
    
    
    *dar = DAR_SUCCESS;
    return (p - buf);
}



int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
//    uint8_t tempbuf[200];
//    uint8_t *des = tempbuf;
    uint16_t OI;
    uint8_t attr, class;
    uint8_t dar = 0;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }


    OI = get_OAD_OI(id);
    attr = get_OAD_Attribute(id);
//    index = (uint8_t)(id & 0x000000FF);

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
                return fr_set_event_report_flag_mode(instance, id, p, 0);
            }
            break;
            case 11: // 上报方式 
            {
                return fr_set_event_report_flag_mode(instance, id, p, 1);
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
                return fr_set_event_report_flag_mode(instance, id, p, 0);
            }
            break;
            case 15:// 上报方式 
            {
                return fr_set_event_report_flag_mode(instance, id, p, 1);
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


    return -DAR_RW_REJECT;
}
