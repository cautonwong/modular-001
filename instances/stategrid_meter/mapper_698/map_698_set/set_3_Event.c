#include "nidavellir.h"
#include "fr_698.h"


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


int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint16_t i;
    uint8_t tempbuf[200];
    uint8_t *des = tempbuf;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }

    uint16_t oi;
    uint8_t attr, index;
    oi = (uint16_t)((id & 0xFFFF0000) >> 16);
    attr = (uint8_t)((id & 0x0000FF00) >> 8);
    index = (uint8_t)(id & 0x000000FF);



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
            memcpy((uint8_t*)&mbase_cb->i_over_event_threshold, p, 0x04);
            p += 4;
            p++;
            memcpy((uint8_t*)&mbase_cb->i_over_event_duration_para, p, 0x01);
            p++;
            return (p - buf);
        }
    case 0x30050B00://上报标识
    {
        p++;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_type, p, 0x01);
        p++;
        return (p - buf);
    }

    case 0x30050F00://上报方式
    {
        p++;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }

    break;
    
    default:
        break;
    }
    return 0x01;
}
