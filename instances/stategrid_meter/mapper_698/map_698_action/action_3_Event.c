#include "nidavellir.h"
#include "fr_698.h"

// 单个事件是否支持复位判断
int single_event_clear_judge(uint16_t oi)
{

    if ((0x400B == oi) || (0x4009 == oi) || (0x400a == oi))
    {
        return -1;
    }

    return 1;
}


//记录事件清零列表
int event_clear_omd_add(struct instance_t* instance, uint32_t omd, uint16_t chn)
{
    uint8_t tempbuf[7];
//    uint16_t id;
    uint32_t index;
    int ret;

    memset(tempbuf, 0, sizeof(tempbuf));

#if 0 //待调整 清当前通道2015 的合闸成功或跳闸成功    //可考虑挪统一清零接口中
    id = event_report_fsid_get(chn, 1);

    if (omd == 0x301F0100 || omd == 0x30200100)
    {
        read(id, buf, 5, NULL);
        buf[0] = 32;

        if (omd == 0x301F0100)
        {
            buf[2] &= ~0x40;
        }
        else
        {
            buf[2] &= ~0x80;
        }

        write(id, buf, 5, NULL);
    }
#endif
    tempbuf[0] = 0x01;
    tempbuf[1] = 0x01;
    tempbuf[2] = 0x53;
    uint32_to_buf_msb(omd, tempbuf + 3);


    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + EventClr_List);;
    ret = nd_set_index(instance, index, &tempbuf[0], 7);
    return ret;
}


int event_add_one_capture(struct instance_t* instance, uint32_t oad , uint8_t *buf, uint8_t *dar)
{

    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint16_t i;
    uint8_t tempbuf[200] = {0};
    uint16_t oi;
    uint8_t attr, index, Schem, maxnum, num;
    struct record_storage_id_info_t info;
    uint8_t* oadlist = tempbuf;
    uint16_t CaptureLen;
    uint32_t CaptureId, capture_oad, temp_oad;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    int ret = get_record_id_info(instance, oad, &info);
    if (ret < 0)
    {
        return -1;
    }
    
    CaptureId = info.capture_index;
    CaptureLen = info.capture_len;// 
    maxnum = (CaptureLen - 1) / 4;

    oi = (uint16_t)((oad & 0xFFFF0000) >> 16);
    attr = (uint8_t)((oad & 0x00001F00) >> 8);
    index = (uint8_t)(oad & 0x000000FF);
    

    if ((*p++ != TYPE_OAD) || ( oad & 0xF0000000 != 0x30000000))
    {
        *dar =  DAR_OTHERS_ERROR;
        return -1;
    }
    
    capture_oad = buf_to_uint32_msb(p);
    Schem = get_OAD_Schem(capture_oad);
    
    //待调整  秘钥状态如何获取？
#if 0
    if ((oi == 0x3013) && (0 == esam_key_status_get()))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
#endif


    //待调整 判断表型无关的oad
#if 0
    if (true == oad_valid_check(capture_oad))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
#endif

#if 0 // vs 要求
    if ((Schem < 1) || (Schem > 4))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
#endif
    
    ret = nd_get_index(instance, CaptureId, &tempbuf[0], sizeof(tempbuf));
    if (ret < 0)
    {
        tempbuf[0]=0;//*dar = DAR_OTHERS_ERROR;
        //return -1;
    }
    oadlist = tempbuf;
    num = *oadlist++;

    if (num >= maxnum)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
    
    for (i = 0; i < num; i++)
    {
        temp_oad = buf_to_uint32_msb(oadlist);
        oadlist += 4;

        if (capture_oad == temp_oad)
        {
            //return DAR_SUCCESS;
            *dar = DAR_RW_REJECT;
            return -1;
            //重复关联对象 返回拒绝
        }
    }
    memcpy(oadlist, p, 4);
    p += 4;

    tempbuf[0] = num + 1;

    ret = nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

#if 0 //待调整  //记录编程列表
    if ((session->client & 0xF0) == 0x00)//终端
    {
        cilent_index = 1;
    }
    else//主站及其他
    {
        cilent_index = 0;
    }

    if (current_connect_status_get(cilent_index))
    {
        program_object_list_add(req->oad.data, DT_OMD);
    }
#endif

    //清零
    ret = clr_record(instance, oad);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
    //待调整 上报确认
    //api_event_report_object_all_channel_confirmed(req);

    return (int)(p - buf);
}

int event_delete_one_capture(struct instance_t* instance, uint32_t oad, uint8_t *buf, uint8_t* dar)
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint16_t i;
    uint8_t tempbuf[200] = { 0 };
    uint16_t oi;
    uint8_t attr, index, Schem, maxnum, num;
    struct record_storage_id_info_t info;
    uint8_t* oadlist = tempbuf;
    uint16_t CaptureLen;
    uint32_t CaptureId, capture_oad, temp_oad;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    int ret = get_record_id_info(instance, oad, &info);
    if (ret < 0)
    {
        return -1;
    }

    CaptureId = info.capture_index;
    CaptureLen = info.capture_len;// 
    maxnum = (CaptureLen - 1) / 4;

    oi = (uint16_t)((oad & 0xFFFF0000) >> 16);
    attr = (uint8_t)((oad & 0x00001F00) >> 8);
    index = (uint8_t)(oad & 0x000000FF);


    if ((*p++ != TYPE_OAD) || ((oad & 0xF0000000) != 0x30000000))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
    capture_oad = buf_to_uint32_msb(p);
    p += 4;
    Schem = get_OAD_Schem(capture_oad);
    //待调整  秘钥状态如何获取？
#if 0
    if ((oi == 0x3013) && (0 == esam_key_status_get()))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
#endif


#if 0 // vs 要求 其他
    if ((Schem < 1) || (Schem > 4))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
#endif

    ret = nd_get_index(instance, CaptureId, &tempbuf[0], sizeof(tempbuf));
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    oadlist = tempbuf;
    num = *oadlist++;

    if ((num > maxnum) || (num == 0))
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    for (i = 0; i < num; i++)
    {
        temp_oad = buf_to_uint32_msb(oadlist);
        if (capture_oad == temp_oad)
        {
            break;
        }
        oadlist += 4;
    }

    if (i >= num)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    memcpy(oadlist, oadlist + 4, CaptureLen + tempbuf - oadlist);
    tempbuf[0] -= 1;

    ret = nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }


#if 0 //待调整  //记录编程列表
    if ((session->client & 0xF0) == 0x00)//终端
    {
        cilent_index = 1;
    }
    else//主站及其他
    {
        cilent_index = 0;
    }

    if (current_connect_status_get(cilent_index))
    {
        program_object_list_add(req->oad.data, DT_OMD);
    }
#endif

    //清零
    ret = clr_record(instance, oad);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }
    //待调整 上报确认
    //api_event_report_object_all_channel_confirmed(req);

    return (int)(p - buf);
}



int ACTION_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
//    uint16_t i;
    uint8_t tempbuf[200];
//    uint8_t* des = tempbuf;
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
    methond = (uint8_t)((id & 0x0000FF00) >> 8);
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
        event_clear_omd_add(instance, id, 0 /*req->channel */); //待调整    如何在map  case中  取到通道号
        //清零
        ret = clr_record(instance, id);
        if (ret < 0)
        {
            dar = DAR_OTHERS_ERROR;
            return ret;
        }

        send_event_mesg_pro(instance, NULL, EventClrS, NULL, 1);


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



    return -1;
}
