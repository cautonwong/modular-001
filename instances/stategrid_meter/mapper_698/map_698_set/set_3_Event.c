#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"


#define U32toBuf(x)  ((uint8_t)(x)), ((uint8_t)(x>>8)), ((uint8_t)(x>>16)), ((uint8_t)(x>>24))
// 初始化默认数据
// 过流事件关联对象
const uint8_t init_30050200[] = {0x01,0x08, U32toBuf(0x00102201), U32toBuf(0x00202201), U32toBuf(0x20002200), U32toBuf(0x20012200), U32toBuf(0x20042200), U32toBuf(0x200A2200), U32toBuf(0x00108201), U32toBuf(0x00208201)};

const uint8_t init_50020300[] = { 0x01,0x07, U32toBuf(0x00100401), U32toBuf(0x00200401), U32toBuf(0x20000200), U32toBuf(0x20010200), U32toBuf(0x20010400), U32toBuf(0x20040200), U32toBuf(0x200A0200) };


//  关联对象检查  待添加
uint8_t oad_valid_check(uint32_t oad)
{
    return true;
}


// 设置事件关联对象
int set_event_obj(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{

    uint8_t i, num, maxnum, scheme;
    uint8_t tempbuf[200];
    uint8_t* tmp0, * tmp1;
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
        return -1;
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
        oad_tmp = buf_to_uint32_lsb(p);
        if (false == oad_valid_check(oad_tmp))  //无效关联对象
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }
        mem_swap_cpy(oadlist, p, 4);
        oadlist += 4;
        p += 4;
    }

    nd_set_index(instance, info.capture_index, &tempbuf[0], (1 + num * 4));

    clr_record(instance, oad);
    return (p - buf);
}

int set_struct_event_para_t(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
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
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    count = para_pack.para_num;

    if (index > count)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    if (index == 0)
    {
        if ((p[0] != TYPE_structure) || (p[1] != count))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        p += 2;

        for (i = 0; i < count; i++)
        {
            if ((*p) != para_pack.event_para[i].tag)
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }
            p++;

            memcpy((uint8_t*)para_pack.event_para[i].ctrlblock_para, p, para_pack.event_para[i].datalen);
            p += para_pack.event_para[i].datalen;
        }
    }
    else
    {
        i = index - 1;

        if ((*p) != para_pack.event_para[i].tag)
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }
        p++;

        memcpy((uint8_t*)para_pack.event_para[i].ctrlblock_para, p, para_pack.event_para[i].datalen);
        p += para_pack.event_para[i].datalen;
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}



int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf)
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
        return -1;
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
            //return get_struct_event_para_t(instance, id, p, &dar);
        }
        break;

        case 11: // 上报标识 
        case 15:// 上报方式 
            //return dl698_normal_data_get(element, req, res);
        }
    }
    break;
    default:
        break;
    }










    switch (id)
    {
    case 0x30050200://关联对象属性表
    {
        uint8_t num,maxnum;
        uint32_t oad_tmp;

        struct record_storage_id_info_t info;
        int ret = get_record_id_info(instance,0x30050700, &info);
        if (ret < 0)
        {
            return -1;
        }
        
        maxnum = ((13*init_30050200[1]+9)/10);
        // tag 判断  待添加
        
        p++; //TYPE_array
        num = *p++;
        if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
        {
            return DAR_OTHERS_ERROR;
        }
        
        *des++ = num;
        for (i = 0; i < num; i++)
        {
            p++; // DTC_OAD
            oad_tmp = buf_to_uint32_lsb(p);
            if (false == oad_valid_check(oad_tmp))  //无效关联对象
            {
                return DAR_OTHERS_ERROR;
            }
            mem_swap_cpy(des, p, 4);
            des += 4;
            p += 4;            
        }

        nd_set_index(instance, info.capture_index, &tempbuf[0], (1 + num*4));
        
        clr_record(instance, 0x30050700);
        return (p - buf);
    }
    case 0x30050500://过流阈值
        {
            //obj->block_name = 0x0A;//只是举例
            //obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            p++;
            p++;
            p++;
            memcpy((uint8_t*)&mbase_cb->OverCurrent_threshold, p, 0x04);
            p += 4;
            p++;
            memcpy((uint8_t*)&mbase_cb->OverCurrent_duration_para, p, 0x01);
            p++;
            return (p - buf);
        }
    case 0x30050B00://上报标识
    {
        p++;
        //memcpy((uint8_t*)&mbase_cb->OverCurrent_report_type, p, 0x01);
        p++;
        return (p - buf);
    }

    case 0x30050F00://上报方式
    {
        p++;
        //memcpy((uint8_t*)&mbase_cb->OverCurrent_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }

    break;
    
    default:
        break;
    }
    return 0x01;
}
