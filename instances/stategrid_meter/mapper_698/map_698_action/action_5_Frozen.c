#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"



int ACTION_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf)
{  
#if 0   
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint16_t i;
    uint8_t tempbuf[200];
    uint8_t* des = tempbuf;
    uint8_t  dar = 0;
    int ret;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    uint16_t oi;
    uint8_t methond, index;

    oi = (uint16_t)((id & 0xFFFF0000) >> 16);
    methond = (uint8_t)((id & 0x00001F00) >> 8);
    index = (uint8_t)(id & 0x000000FF);

    switch (methond)
    {
    case 1: // 复位
        if (p[0] != TYPE_integer || p[1] != 0)
        {
            dar = DAR_OTHERS_ERROR;
            return -1;
        }
        p += 2;
        // 待调整
        // 如何判断是否带时间标签 及厂内
        //if ((req->time_tag_len < 2) && ((ctrl_block->factor_mode == false)))
        //{
        //    return DAR_TIME_TAG_INVALID;
        //}

        event_clear_omd_add(instance, id, 0 /*req->channel */); //待调整    如何在map  case中  取到通道号

        ret = single_event_clear_judge(oi);
    
        if (ret < 0)
        {
            dar = DAR_OTHERS_ERROR;
            return -1;
        }

        if (oi == 0x3023)
        {
            //无功组合特征字特殊处理
        }
        //清零
        ret = clr_record(instance, id);
        if (ret < 0)
        {
            dar = DAR_OTHERS_ERROR;
            return ret;
        }
        //待调整 上报确认
        //dl698_api_event_report_object_all_channel_confirmed(req);
        return (p - buf);

    case 4: //添加一个事件关联对象属性 
        return event_add_one_capture(instance, id, p,  &dar);

    case 5: //删除一个事件关联对象属性 
        return event_delete_one_capture(instance, id, p, &dar);

    default:
        break;
    }


#endif
    return -1;
}
