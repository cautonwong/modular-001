#include "nidavellir.h"
#include "fr_698.h"



/**主动上报 类型 active_report_CMDtype */
#define  ACTIVE_REPORT_3320            (1 << R_3320)   //事件列表
#define  ACTIVE_REPORT_2015            (1 << R_2015)   // 状态字
#define  ACTIVE_REPORT_3011_END        (1 << R_3011_END)//掉电恢复  时间
#define  ACTIVE_REPORT_3011_START      (1 << R_3011_START)//掉电发生  时间



int fr_follow_report_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    
    return 0;
    //读取3320 判断是否跟随上报
    //{
    //    p[cache->wr++] = 0x01;//follow_report
    //}
}

//主动上报 使能 检查
uint8_t active_report_enable_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int ret;
    uint32_t addr;
#if 1
    uint8_t flag = 0;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ActiveReport_en);
    ret = nd_get_index(instance, addr, &flag, 1);


    if ((ret < 0) || (flag == 0))
    {
        return false;
    }

#endif
    return true;
}
// 上报通道使能检查
uint8_t report_channel_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint16_t channel)
{
    int ret;
    uint32_t addr, oad;
    uint8_t i, * p, buf[13] = {0};



    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report_channel); // 待调整
    ret = nd_get_index(instance, addr, buf, 13);

    if (ret < 0)
    {
        return false;
    }

    if (buf[0] == 0 || buf[0] > 4)
    {
        return false;
    }

    p = buf + 1;

    for (i = 0; i < buf[0]; i++)
    {
        oad = buf_to_uint32_lsb(p);

        if (channel == chl_ir_698 && oad == 0xF2020201)
        {
            return true;
        }

        if (channel == chl_RS485_1_698 && oad == 0xF2010201)
        {
            return true;
        }

        if (channel == chl_PLC_698 && oad == 0xF2090201)
        {
            return true;
        }

        if (channel == chl_RS485_2_698 && oad == 0xF2010202)
        {
            return true;
        }

        p += 4;
    }

    return false;
}


//3011事件的主动上报方式和标识  检查    
//eventtype: 0掉电发生，1掉电恢复
//返回true 上报， false 不上报
uint8_t active_report_check_3011(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t *buf, uint8_t eventtype)
{
    uint8_t count, i, * p;
    uint32_t oad = 0;
    int ret;
    uint32_t addr;

    uint8_t rptflag = 0, rptmode = 0;

    if (eventtype == 1) //恢复
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report3320_OADList3); //待调整  读取模块通道的 上报列表
        ret = nd_get_index(instance, addr, buf, 241);

        if (ret < 0)
        {
            return false;
        }

        p = buf;
        count = *p++;
        if (count > 60)
            count = 60;

        for (i = 0; i < count; i++)
        {
            oad = buf_to_uint32_lsb(p);
            if (oad == 0x30110200)//
            {
            
                rptflag = read_event_report_flag_mode(instance, 0x30110200, 0);
                rptmode = read_event_report_flag_mode(instance, 0x30110200, 1);
                if (((rptflag == END_RPT) || (rptflag == ALL_RPT)))//掉电上报标识为事件恢复上报
                {
                    if (rptmode == ACTIVE_RPT)
                    {
                        return true;
                    }
                }
            }
            p += 4;
        }
    }
    else // 发生
    {
        rptflag = read_event_report_flag_mode(instance, 0x30110200, 0);
        rptmode = read_event_report_flag_mode(instance, 0x30110200, 1);
        if (((rptflag == START_RPT) || (rptflag == ALL_RPT)))//掉电上报标识为事件发生上报
        {
            if (rptmode == ACTIVE_RPT)
            {
                return true;
            }
        }

    }
    return true;
}

// 掉电事件 发生和恢复 主动上报组帧(apdu)
int active_report_3011_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t eventtype)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t *ptr_cache, *ptr = instance->apdu_cache;    // 500字节
    uint8_t* tmp_buff = instance->apdu_cache + 500; //保证300字节空间
    uint32_t rcsd_list[(REQ_RCSD_MAXNUM + 1)];
    uint32_t oad;
    uint8_t i;
    uint16_t  data_len = 0;

    struct rsd_info_t rsd_info;
    int ret;

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }
    
    if (false == report_channel_check(instance, pipeline, mesg, chl_PLC_698)) 
    {
        return 0;
    }

    active_report_check_3011(instance, pipeline, mesg, tmp_buff, eventtype);

    if (eventtype)
    {
        ptr_cache = ptr + 3;
    }
    else
    {
        ptr_cache = ptr;
    }


    *ptr_cache++ = 0x88;
    *ptr_cache++ = 0x02;
    *ptr_cache++ = mbase_cb->report_piid & 0x3F;
    *ptr_cache++ = 0x01;

    oad = 0x30110201;
    ptr_cache += uint32_to_buf_msb(oad, ptr_cache); //oad

    oad = 0x30110200;
    get_capture_list(instance, oad, rcsd_list, 3);

    *ptr_cache++ = rcsd_list[0]; // rcsd个数
    if (rcsd_list[0] > REQ_RCSD_MAXNUM)
    {
        return -1;
    }

    for (i = 0; i < rcsd_list[0]; i++)
    {
        *ptr_cache++ = 0; // rcsd:oad
        ptr_cache += uint32_to_buf_msb(rcsd_list[1 + i], ptr_cache);
    }

    *ptr_cache++ = 1; // 数据


    memset((uint8_t *)&rsd_info, 0, sizeof(rsd_info));
    rsd_info.select = 9;
    rsd_info.last_no = 1;
    rsd_info.start_mark = 0;
    rsd_info.end_mark = 0xffffffffU;
    rsd_info.mode = 0;

    ret = record_data_package(instance, oad, tmp_buff, &rsd_info, rcsd_list);
    if (ret < 0)
    {
        return 0;
    }
    data_len = ret;
    ndv_memcopy(ptr_cache, tmp_buff, data_len);
    ptr_cache += data_len;

    *ptr_cache++ = 0; //跟随上报
    *ptr_cache++ = 0; //timeTag
    
    if (eventtype)
    {
        data_len = ptr_cache - ptr - 3;
        ptr[0] = 0x10;
        ptr[1] = 0x00;
        ptr[2] = data_len; //待调整  

        *ptr_cache++ = 0x02;//RN+MAC
        *ptr_cache++ = 0x0c;//RN长度

        if (0)//待调整 ESAM故障标志  mbase_cb-> ？
        {
            data_len = 12 + 5;
            memset(ptr_cache, 0xFF, data_len);
            *(ptr_cache + 12) = 0x04;
        }
        else
        {
            //data_len = esam_report_rn_mac(res.cache_idx, data_len, res.cache_ptr);  //待调整 esam交互 获取主动上报的随机数和mac
        }
        ndv_memcopy(mbase_cb->report_random_data, ptr_cache, 12); //拷贝随机数
        ptr_cache += data_len;
    }

    return (ptr_cache - ptr);

}

//获取模块通道 3320上报列表中主动上报 oad
//函数返回值为主动上报oad个数，    传入buf空间需保证300字节 
uint8_t active_report_3320list_get(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* buf)
{
    uint8_t count, i, * p, num, tmp_buf[241];
    uint16_t len, len2;
    uint32_t oad = 0;


    int ret;
    uint32_t addr;
    uint8_t rptmode = 0;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report3320_OADList3); //待调整  读取模块通道的 上报列表
    ret = nd_get_index(instance, addr, tmp_buf, 241);

    if (ret < 0)
    {
        return false;
    }

    p = tmp_buf;
    count = *p++;
    if (count > 60)
        count = 60;
    num = count;
    len = 1 + count * 4ul;
    for (i = 0; i < num; i++)
    {
        oad = buf_to_uint32_lsb(p);
        rptmode = read_event_report_flag_mode(instance, oad, 1);
        if ((rptmode == FOLLOW_RPT) || (oad == 0x30110200))//非主动上报 不放在列表中,3011也不放主动上报OAD中
        {
            len2 = len - ((p - tmp_buf) + 4);
            ndv_memcopy(p, p + 4, len2);
            //len -= 4;
            count--;
        }
        else
            p += 4;
    }
    p = buf;
    for (i = 0; i < count; i++)
    {
        *p++ = TYPE_OAD;
        mem_swap_cpy(p, &tmp_buf[1 + i * 4], 4);
        p += 4;
    }
    return count;
}
//2015跟随上报状态字  检查是否需要上报，及获取状态字
//mode: 0 主动上报， 1跟随上报
uint32_t report_2015_get(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint16_t chn, enum report_mode_t mode)
{
    uint32_t value;

    uint8_t rptmode = 0;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);

    if (rptmode != mode)//上报方式为跟随上报，才能跟随上报
    {
        return 0;
    }

    value = mbase_cb->report_2015state[chn%4];  //chl_PLC_698

    return value;
}
// 常规的3320和2015 主动上报组帧(apdu)
int active_report_normal_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint8_t* p, * apdu, num, flag, count, need_report_piid = false;
    uint32_t value;
    uint16_t encry_len = 0, len_apdu = 0;


    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t *ptr = instance->apdu_cache;    // 500字节
    uint8_t* tmp_buff = instance->apdu_cache + 500; //保证300字节空间

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }

    if (false == report_channel_check(instance, pipeline, mesg, chl_PLC_698))
    {
        return 0;
    }


    p = ptr;
    num = 0;
    flag = 0;

    if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3320)
    {
        if ((mbase_cb->active_report_retry_dly[R_3320] == 0) && (mbase_cb->active_report_retry_cnt[R_3320] < 3))//已发送，主站未确认，等待超时
        {
            count = active_report_3320list_get(instance, pipeline, mesg, &tmp_buff[0]);
            if (count)
            {
                flag |= 0x01;
                num++;
                mbase_cb->active_report_retry_cnt[R_3320]++;
                if (3 == mbase_cb->active_report_retry_cnt[R_3320])
                    need_report_piid = true;
            }
            else
            {
                mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3320;
            }
        }
    }

    if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_2015)
    {
        if ((mbase_cb->active_report_retry_dly[R_3320] == 0) && (mbase_cb->active_report_retry_cnt[R_2015] < 3))//已发送，主站未确认，等待超时
        {
            value = report_2015_get(instance, pipeline, mesg, chl_PLC_698, ACTIVE_RPT);// 获取模块通道 需上报 2015
            if (value != 0)
            {
                num++;
                flag |= 0x02;
                mbase_cb->active_report_retry_cnt[R_2015]++;
                if (3 == mbase_cb->active_report_retry_cnt[R_2015])
                    need_report_piid = true;
            }
            else
            {
                mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_2015;
            }
        }
    }
    if (num == 0)
    {
        return 0;
    }
    *p++ = 0x10;
    *p++ = 0x00;
    *p++ = len_apdu;
    apdu = p;
    *p++ = 0x88;
    *p++ = 0x01;
    *p++ = mbase_cb->report_piid & 0x3F;
    *p++ = num;

    if (flag & 0x01)
    {
        *p++ = 0x33;
        *p++ = 0x20;
        *p++ = 0x02;
        *p++ = 0x00;
        *p++ = 0x01;
        *p++ = TYPE_array;
        *p++ = count;
        ndv_memcopy(p, &tmp_buff[0], 5ul * count);
        p += 5ul * count;
    }

    if (flag & 0x02)
    {
        *p++ = 0x20;
        *p++ = 0x15;
        *p++ = 0x02;
        *p++ = 0x00;
        *p++ = 1;
        *p++ = TYPE_bit_string;
        *p++ = 0x20;
        p += uint32_to_buf_lsb(value, p);//是lsb还是msb，待验证
    }
    *p++ = 0;
    *p++ = 0;
    len_apdu = p - apdu;
    *(apdu - 1) = len_apdu;
    *p++ = 0x02;//RN+MAC
    *p++ = 0x0c;//RN长度

    if (1)//待调整 ESAM故障标志  mbase_cb-> ？
    {
        encry_len = 12 + 5;
        memset(p, 0xFF, encry_len);
        *(p + 12) = 0x04;
    }
    else
    {
        //encry_len = esam_report_rn_mac(apdu, len_apdu, p);  //待调整 esam交互 获取主动上报的随机数和mac
    }
    ndv_memcopy(mbase_cb->report_random_data, p, 12); //拷贝随机数
    p += encry_len;

    if (need_report_piid)
        mbase_cb->report_piid++;

    return (len_apdu + encry_len + 5);
}

// 主动上报处理  每秒调用   
// 主动上报apdu数据存instance->apdu_cache， 返回值len为上报apdu数据长度
int fr_active_report_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t*  mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int len;


    if (mbase_cb->poweron_active_report_dly < 5)
    {
        return -1;
    }
    if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_END) 
    {
        if ((mbase_cb->active_report_retry_dly[R_3011_END] == 0) && (mbase_cb->active_report_retry_cnt[R_3011_END] < 3))//已发送，主站未确认，等待超时
        {
            len = active_report_3011_pro(instance, pipeline, mesg, 1);
            if (len > 0)
            {
                mbase_cb->active_report_retry_dly[R_3011_END] = 10;//  若无响应 10秒后重发       
                mbase_cb->active_report_retry_cnt[R_3011_END]++;

                if (3 == mbase_cb->active_report_retry_cnt[R_3011_END])
                    mbase_cb->report_piid++;

                return len;
            }
            else
                mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_END;

        }
    }
    //不完全掉电时 掉电发生上报
    else if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_START)
    {
        len = active_report_3011_pro(instance, pipeline, mesg, 0);
        if (len > 0)
        {
            mbase_cb->active_report_retry_cnt[R_3011_START]++;
            if (3 == mbase_cb->active_report_retry_cnt[R_3011_START])
            {
                mbase_cb->report_piid++;
                mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_START;
            }
            return len;
        }
        else
            mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_START;

    }
    if ((mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_END) && (mbase_cb->active_report_retry_cnt[R_3011_END] < 3))
    {
        return 0;
    }
    if (mbase_cb->active_report_retry_dly[R_3011_END])
        return 0;
    if (mbase_cb->active_report_CMDtype & (ACTIVE_REPORT_3320 | ACTIVE_REPORT_2015))//有上报请求
    {
        len = active_report_normal_pro(instance, pipeline, mesg);
        if (len > 0)
        {
            mbase_cb->active_report_retry_dly[R_3320] = 10;//  10秒无响应重发     
            mbase_cb->active_report_retry_dly[R_2015] = 10;//  10秒无响应重发     
            return len;
        }
    }
    return 0;
}



int active_report_handle(struct instance_t* instance, struct pipeline_t* p_pipeline, struct message_t* mesg)
{
    struct pipeline_t pipeline = {0};
    uint8_t buf[4];
    int ret = 1;//fr_active_report_pro(instance, p_pipeline, mesg);
    if(ret > 0){
        //pipeline.id
        mesg->len = 4;
        mesg->to = pid_tx_698;
        pipeline.id = pid_rx_698;
        mesg->payload = buf;
        send_mesg(instance, &pipeline, mesg);
    }
}


int get_report_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{

}


