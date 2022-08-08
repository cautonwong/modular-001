#include "nidavellir.h"
#include "fr_698.h"
#include "port.h"
#include "gs_record.h"


typedef struct 
{
    uint32_t    s_index;    //地址
    uint16_t    oad;        
    uint8_t     atr;        //属性
    uint8_t     offset;     //偏移
    uint8_t     len;        //长度
}general_set4;

const general_set4  set4300_table[] = 
{
     Device_descriptor,   0x0200,   0x02,   0x02,   32,//设备描述符
    
     Factory_No,          0x0301,   0x03,   0x02,    4, //厂家代码
     Version_SoftWare,    0x0302,   0x03,   0x02,    4, //软件版本
     Date_SoftWare,       0x0303,   0x03,   0x02,    6, //软件版本日期
     Version_HardWare,    0x0304,   0x03,   0x02,    4, //硬件版本
     Date_HardWare,       0x0305,   0x03,   0x02,    6, //硬件版本日期
     Extend_Info,         0x0306,   0x03,   0x02,    8, //扩展信息
    
     Produced_Date,       0x0400,   0x04,   0x01,    7, //生产日期
//   Subset_List,         0x0500,   0x04,   0x05,    2, //子设备列表(num+2*oi)
//   Protocol_List,       0x0601,   0x06,   0x02,   20, //协议列表
     FollowReport_en,     0x0700,   0x07,   0x01,    1, //允许跟随上报
     ActiveReport_en,     0x0800,   0x08,   0x01,    1, //允许主动上报
     Comm_Master_en,      0x0900,   0x09,   0x01,    1, //允许与主站通讯
//   Report_channel,      0x0A00,   0x0a,   0x03,    4, //上报通道
};

const general_set4  general_set4_talble[] =
{
//   Comm_Number,               0x4001,     0x02,   0x02,   6,//表地址
//   Meter_Number,              0x4002,     0x02,   0x02,   6,//表号
//     User_Number,               0x4003,     0x02,   0x02,   6,//客户编号
//   Meter_Location,            0x4004,     0x02,   0x01,   27,//设备地理位置
     SeasonTable_Changetime,    0x4008,     0x02,   0x01,   7,//两套时区切换时间
     DayTable_Changetime,       0x4009,     0x02,   0x01,   7,//两套时段切换时间
     SecretKey_Num,             0x400F,     0x02,   0x01,   1,//秘钥总条数
     Meter_Num,                 0x4010,     0x02,   0x01,   1,//计量元件个数
     Weekend,                   0x4012,     0x02,   0x02,   1,//周休日特征字
     WeekendFee,                0x4013,     0x02,   0x01,   1,//周休日采用费率号
     Meter_CT,                  0x401C,     0x02,   0x01,   4,//CT
     Meter_PT,                  0x401D,     0x02,   0x01,   4,//PT
     Pulse_Width,               0x4102,     0x02,   0x01,   1,//脉冲宽度
     ASCII_AssetsMan,           0x4103,     0x02,   0x02,   32,//资产管理编码
     ASCII_Un,                  0x4104,     0x02,   0x01,   6,//额定电压
     ASCII_Ib,                  0x4105,     0x02,   0x01,   6,//额定电流
     ASCII_Imax,                0x4106,     0x02,   0x01,   6,//最大电流
     ASCII_Imin,                0x4106,     0x03,   0x01,   6,//最小电流
     ASCII_Itr,                 0x4106,     0x04,   0x01,   6,//转折电流
     ASCII_AccuracyClass1,      0x4107,     0x02,   0x01,   4,//有功准确度等级
     Meter_Const1,              0x4109,     0x02,   0x01,   4,//有功脉冲常数
     ASCII_MeterType,           0x410B,     0x02,   0x02,   32,//电能表型号
//   MeterRunWord,              0x4110,     0x02,   0x01,   1,//电能表运行特征字
     SoftWareNo,                0x4111,     0x02,   0x02,   16,//软件备案号
     Measure_Mode,              0x4112,     0x02,   0x02,   1,//有功组合特征字
     Meter_Password,            0x4401,     0x02,   0x02,   8,//应用连接密码
};
#define		set4num			sizeof(general_set4_talble)/sizeof(general_set4)


int general_set_4_para(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
//    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,i,year2;
    uint8_t attribute,timer[6];
    uint8_t* p = buf;
    uint32_t addr=0;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    for(i=0; i<set4num; i++)
    {
        if(oad == general_set4_talble[i].oad) 
        {
            if(attribute == 0x02)
            {
                switch(oad)
                {
                    case 0x4010://元器件个数
                        if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
                        if(*(p+1) != 1)   return -DAR_TYPE_NOT_MATCH;
                        break;
                    case 0x4012://周休日
                        if(*p != 4) return -DAR_TYPE_NOT_MATCH;
                        mbase_cb->Fee |= F_DAYTABChange;
                        break;
                    case 0x4013://周休日日时段表号
                        if(*p != 17) return -DAR_TYPE_NOT_MATCH;
                        //保存编程前
                        nd_get(instance, id, timer, 2, NULL);
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_WeekTab);
                        nd_set_index(instance, addr, timer, 2);
                        send_event_mesg_pro(instance, NULL, WeekTabWrS, NULL, 1);// 周休日编程事件
                        mbase_cb->Fee |= F_DAYTABChange;
                        break;
                    case 0x400F://秘钥总条数
                    case 0x401C://变比
                    case 0x401D:
                        return -DAR_RW_REJECT;
                        break;
                    case 0x4102:    //脉冲宽度
                    case 0x4104:    //额定电压
                    case 0x4105:    //额定电流
                    case 0x4106:    //最大电流
                    case 0x4107:    //有功准确度等级
                    case 0x4109:    //有功脉冲常数
                        return -DAR_RW_REJECT;
                    case 0x410B:    //电表型号
                        if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
                        if(*p != 0X0A) return -DAR_RW_REJECT;
                    case 0x4103:    //资产管理编码
                    case 0x4111:    //软件备案号
                        if((mbase_cb->factor_mode == false) && (0x4111 == oad))  return -DAR_RW_REJECT; //工厂模式外，禁止写入
                        if(*p != 0x0A)    return -DAR_TYPE_NOT_MATCH;
						break;
                    case 0x4008://切换时间
                    case 0x4009:
                        if((*(p+7) == 0xFF) && (*(p+6) == 0x00) && (*(p+5) == 0x00) && (*(p+4) == 0x00) && (*(p+3) == 0x00) && (*(p+2) == 0x00) && (*(p+1) == 0x00))
                        {
                        }
                        else
                        {
                            mem_swap_cpy((uint8_t*)&year2, p+1, 2);
                            if((year2 < 2000)&& (year2 > 0)) return -DAR_RW_REJECT;
                            year2 = year2 - 2000;
                            timer[0] = *(p+4); //day
                            timer[1] = *(p+3); //month
                            timer[2] = (uint8_t)year2;
                            timer[3] = *(p+5);  //hour
                            timer[4] = *(p+6);  //min
                            timer[5] = 0;  //sec
                            if((Time_Format_Check(&timer[3])) || (Date_Format_Check(timer)))    return -DAR_OTHERS_ERROR;
                        }
                        *(p+7) = 0xFF;
                        mbase_cb->Fee |= F_ChangeTmOn;
                        break;
                    case 0x4112://有功组合特征字
                        if(*p != 4) return -DAR_TYPE_NOT_MATCH;
                        nd_get(instance, id, timer, 3, NULL);
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_ActCom);
                        nd_set_index(instance, addr, timer, 3);
                        send_event_mesg_pro(instance, NULL, ActCombWrS, NULL, 1);// 
                        break;
						
                }
              
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + general_set4_talble[i].s_index);
                nd_set_index(instance, addr, (p + general_set4_talble[i].offset), general_set4_talble[i].len); 
                return (general_set4_talble[i].offset + general_set4_talble[i].len);
            }
            else
            {
                return -DAR_RW_REJECT;
            }
        }
    }
    return 0;
}


int set_support_protocol_list(struct instance_t* instance, uint32_t id, uint8_t* buf)
{

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i;
    uint8_t* p = buf;
    uint32_t addr=0;
    uint8_t index = 0;
    uint8_t *tmp = instance->apdu_cache;
    uint8_t *tmp_cache = instance->apdu_cache;
    uint8_t count, len,maxlen;

    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

    index = (uint8_t)(id & 0x000000FF);
    
    if (index > 3)
    {
        return -DAR_OTHERS_ERROR;
    }

    maxlen = (76 - 1) / 3;
    
    if (index == 0)
    {
        count = p[1];

        if (p[0] != TYPE_array || count > 3)
        {
            return -DAR_OTHERS_ERROR;
        }

        p += 2;
        *tmp++ = count;

        for (i = 0; i < count; i++)
        {
            len = p[1];

            if ((p[0] != TYPE_visible_string) || (len > maxlen - 1))
            {
                return -DAR_OTHERS_ERROR;
            }

            ndv_memcopy(tmp, &p[1], len + 1);
            p += len + 2;
            tmp += maxlen;
        }

        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Protocol_List);
        nd_set_index(instance, addr, tmp_cache, (uint16_t)(tmp - tmp_cache)); 
        p = p + (uint16_t)(tmp - tmp_cache);

    }
    else
    {
        len = p[1];

        if ((p[0] != TYPE_visible_string) || (len > maxlen - 1))
        {
            return -DAR_OTHERS_ERROR;
        }

        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Protocol_List);
        nd_get_index(instance, addr, tmp_cache, 76); 

        ndv_memcopy((tmp_cache + (index - 1) * maxlen + 1), &p[1], len + 1);

        nd_set_index(instance, addr, tmp_cache, 76); 

        p += (len + 2);
    }

    return (p-buf);
}


int set4300(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t j, i,timer[6], num, Tnum,tmpbuf[32]={0};
    uint8_t* p = buf;
    uint32_t addr=0, chn;
    uint16_t idl = 0,year2, tmpid1;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    
    idl = (uint16_t)(id & 0x0000FFFF);
    
    for(i=0; i<sizeof(set4300_table)/sizeof(general_set4); i++)
    {
        if(idl == set4300_table[i].oad)
        {
            switch(idl)
            {
                case 0x0200:   //设备描述符
                    if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
                    if(p[1] > 32)  return -DAR_TYPE_NOT_MATCH;
                    break;
                case 0x0400://生产日期
            		if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
            
                    mem_swap_cpy((uint8_t*)&year2, p+1, 2);
                    if(year2 < 2000) return -DAR_RW_REJECT;
                    year2 = year2 - 2000;
                    timer[0] = *(p+4); //day
                    timer[1] = *(p+3); //month
                    timer[2] = (uint8_t)year2;
                    timer[3] = *(p+5);  //hour
                    timer[4] = *(p+6);  //min
                    timer[5] = *(p+7);  //sec
                    
                    if(Date_Format_Check(timer))  return -DAR_RW_REJECT;
                    if(Time_Format_Check(&timer[3])) return -DAR_RW_REJECT;
                    break;
                case 0x0700://允许跟随上报
                case 0x0800://允许主动上报
                    if(*p != 3)   return -DAR_TYPE_NOT_MATCH;
                    break;

            }
            memset(tmpbuf, 0x00, set4300_table[i].len);
            p = p + set4300_table[i].offset;
            memcpy(tmpbuf, p, set4300_table[i].len);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + set4300_table[i].s_index);
            nd_set_index(instance, addr, tmpbuf, set4300_table[i].len); 
            p = p + set4300_table[i].len; 
            EventOut_698_check(instance);  
            return (p-buf);
        }
    }
    
    
    switch(idl)
    {
        case 0x0300: //版本信息
        {
            if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
                        tmpid1 = idl;

            num = p[1];
            if ((p[0] != TYPE_structure) || (num != 6))
            {
                return -DAR_TYPE_NOT_MATCH;
            }
            p += 2;
            Tnum = 6; // 6个成员
            for (j = 1; j < (Tnum + 1) ; j++)
            {
                for(i=0; i<sizeof(set4300_table)/sizeof(general_set4); i++)
                {
                    if((tmpid1 + j) == set4300_table[i].oad)
                    {
                        if ((p[0] != TYPE_visible_string) || (p[1] != set4300_table[i].len)) //类型不匹配
                        {
                            return -DAR_TYPE_NOT_MATCH;
                        }
                        
                        p = p + set4300_table[i].offset;
                        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + set4300_table[i].s_index);
                        nd_set_index(instance, addr, p, set4300_table[i].len); 
                        p = p + set4300_table[i].len ;
                    }
                }
            }
            return (p-buf); 
        }
        break;
        case 0x0600: //协议列表
        case 0x0601: //协议列表
        case 0x0602: //协议列表
        case 0x0603: //协议列表
        {
            if(mbase_cb->factor_mode == false)  return -DAR_RW_REJECT; //工厂模式外，禁止写入
            
            return set_support_protocol_list(instance,id, buf);
        }
        break;

        case 0x0A00: //上报通道
        {
            num = p[1];
            if ((p[0] != TYPE_array) || (num > 3))
            {
                return -DAR_TYPE_NOT_MATCH;
            }
            p += 2;

            tmpbuf[0] = num;
            
            for (i = 0; i < num; i++)
            {
                if(*p++ != TYPE_OAD)
                {
                    return -DAR_OTHERS_ERROR;
                }
            
                chn = buf_to_uint32_msb(p);
                p += 4;
            
                if ((chn == 0xf2020201) || ((chn != 0xf2010201)&&(chn != 0xf2010202) && chn != 0xf2090201))
                {
                    return -DAR_OTHERS_ERROR;
                }
            
                uint32_to_buf_lsb(chn, &tmpbuf[4*i+1]);
            }

            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report_channel);
            nd_set_index(instance, addr, tmpbuf, 13); 

            EventOut_698_check(instance);  
            return (p-buf);            
        }
        break;
        default:
            break;
    }
    
    return -DAR_OBJECT_NOT_EXIST;
}

uint8_t lcd_data_chk(uint8_t *p)
{
    if((*(p+3) < 5) || (*(p+3) > 30))   //全显时间5~30
        return 1;
    if((*(p+14) > 4) || (*(p+16) > 4))   //小数显示0~4
        return 1;
    if(*(p+18) > 1) //液晶12字样 0~1
        return 1;
    return 0;
}

uint8_t CheckData400C(uint8_t *p,uint8_t index)
{
    switch(index)
    {
        case 0:
          if(*(p+1) > 14)   return DAR_TIME_ZONE_EXCEED;
          break;
        case 2:
          if(*(p+1) > 14)   return DAR_DAY_INTERVAL_EXCEED;
          break;
        case 1:
          if(*(p+1) > 8)   return DAR_BUFFER_OVERFLOW;
          break;
        case 3:
          if(*(p+1) > 12)   return DAR_TARIFF_NUM_EXCEED;
          break;
        case 4:
          if(*(p+1) > 20)   return DAR_BUFFER_OVERFLOW;
          break;
    }
    return 0;
}


extern const uint8_t  MonTab[13];// = {	0,31,28,31,30,31,30,31,31,30,31,30,31};

int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,year2;
    uint8_t index,attribute,i,num,buffer[42]={0},temp[114]={0},num_hourtab=0,year,timer[6];
    int8_t dar =0;
    uint8_t* p = buf;
    uint32_t s_index = 0x00,addr=0;
    int len=0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    len = general_set_4_para(instance, id, buf);
    if(len != 0)
    {
        return len;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    switch (oad)
    {
        case 0x4000://时间
            if(attribute == 0x02)
            {
                uint8_t settimebuf[8]={0};
                if(p[0] != TYPE_date_time_s){
                    return -DAR_TYPE_NOT_MATCH;
                }
/*                if(check_date((p[1]<<8)+p[2], p[3], p[4], p[5], p[6], p[7]) < 0){
                    return -DAR_OTHERS_ERROR;
                }*/
                p++;
                
                mem_swap_cpy((uint8_t*)&year2, p, 2);
                if(year2 < 2000) return -DAR_OTHERS_ERROR;
                year2 = year2 - 2000;
                timer[0] = *(p+3); //day
                timer[1] = *(p+2); //month
                timer[2] = (uint8_t)year2;
                timer[3] = *(p+4);  //hour
                timer[4] = *(p+5);  //min
                timer[5] = *(p+6);  //sec
                
                if(Date_Format_Check(timer))  return -DAR_OTHERS_ERROR;
                if(Time_Format_Check(&timer[3])) return -DAR_OTHERS_ERROR;
                
                //保存校时前时间 
                rtc_get_from_tm(instance,temp);
                mem_swap_cpy( &buffer[0], &temp[0], 7);

    log_info("%02x-%02x-%02x-%02x-%02x-%02x\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);

                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                nd_set_index(instance, addr, buffer, 7);
                
                mem_swap_cpy((uint8_t*)&oad, p, 2);
                year = oad - 2000;
                p = p + 2;
                settimebuf[5] = BYTE_BCD(year);//年
                settimebuf[4] = BYTE_BCD(*p++);//月
                settimebuf[3] = BYTE_BCD(*p++);//日
                settimebuf[2] = BYTE_BCD(*p++);//时
                settimebuf[1] = BYTE_BCD(*p++);//分
                settimebuf[0] = BYTE_BCD(*p++);//秒
                settimebuf[6] = GetWeek(&timer[0]);
                write_mcu_rtc(0, settimebuf, 7);

                read_mcu_rtc(0x00, instance);

                if(buffer[3] != instance->time.tm_mday)  //跨日，清除广播校时标志位
                {
                    mbase_cb->boardadjtime = 0;
                }
                
                //保存校时后时间 
                rtc_get_from_tm(instance,temp);
                mem_swap_cpy( &buffer[0], &temp[0], 7);
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                nd_set_index(instance, addr, buffer, 7);

                send_event_mesg_pro(instance, NULL, SetTimeS, NULL, 1);// 校时记录

                record_rollback_after_set_time(instance, 0);
                mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
                mbase_cb->RTCErrS_status = 0;

                clr_last_bill_time(instance);

                return (p-buf);
            }
            else if(attribute == 0x05)  //广播校时参数
            {
                p = p + 3;
                mem_swap_cpy((uint8_t*)&mbase_cb->Limit_T_Min, p, 2);
                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                offsetof(struct mbase_control_block_t, Limit_T_Min));
                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->Limit_T_Min, 2);
                p = p + 2;
                p++;
                mem_swap_cpy((uint8_t*)&mbase_cb->Limit_T_Max, p, 2);
                s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                offsetof(struct mbase_control_block_t, Limit_T_Max));
                nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->Limit_T_Max, 2);
                p = p + 2;
                return (p-buf);
            }
            break;
        case 0x4001://表地址
        case 0x4003://客户编号
            if(attribute == 0x02)
            {
                len = *(p+1);
                if(len <= 6)
                {
                    memset(buffer, 0x00, 6);
                    memcpy((uint8_t*)&buffer[6-*(p+1)], (p+2), len);
                }
                else
                {
                    return -DAR_BUFFER_OVERFLOW;
                }
                p = p + 2;
                if(BCD_Check(buffer, 6) == 1)    return -DAR_TYPE_NOT_MATCH;
                if(oad == 0x4001)
                {
                    mem_swap_cpy(mbase_cb->tsa, buffer, 6);
                    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, tsa));
                    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->tsa, 6);
                }
                else
                {
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + User_Number);
                    nd_set_index(instance, s_index, buffer, 6); 
                }
                p = p + len;
                return (p-buf);
            }
            break;
        case 0x4004://地理位置
            if(attribute != 0x02)   return -DAR_RW_REJECT;
            if(index == 0)
            {
                if((*(p+5) > 1) || (*(p+15) > 1))   return -DAR_BUFFER_OVERFLOW;
                
                p = p + 2;
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location1);
                nd_set_index(instance, addr, p, 10); 
                p = p + 10;
                
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location2);
                nd_set_index(instance, addr, p, 10); 
                p = p + 10;
                
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location3);
                nd_set_index(instance, addr, p, 5); 
                p = p + 5;
                
                return (p-buf);
            }
            else if(index == 0x01)
            {
                s_index = Meter_Location1;
                len = 10;
            }
            else if(index == 0x02)
            {
                s_index = Meter_Location2;
                len = 10;
            }
            else if(index == 0x03)
            {
                s_index = Meter_Location3;
                len = 5;
            }
            else 
            {
                return -DAR_RW_REJECT;
            }
            break;
        case 0x4007://LCD参数
            if(attribute == 0x02)
            {
                uint32_t idbuf[] = {
                (uint32_t)&mbase_cb->lcd_poweron_all_dis_time,            //上电全显时间
                (uint32_t)&mbase_cb->backlight_key_time,                  //背光点亮时间
                (uint32_t)&mbase_cb->lcd_checkdis_keep_sec,               //显示查看背光点亮时间
                (uint32_t)&mbase_cb->lcd_nopower_key_keep_sec,            //无电按键驻留时间
                (uint32_t)&mbase_cb->energy_point,                        //电能小数显示位
                (uint32_t)&mbase_cb->power_point,                         //功率小数显示位
                (uint32_t)&mbase_cb->lcd_12_mean,                         //液晶12字样意义
                };
                uint8_t lens[] = {1, 2, 2, 2, 1, 1, 1};
                uint8_t offset[] = {0, 1, 3, 5, 7, 8, 9};
                if(index == 0)
                {
                    if( lcd_data_chk(p) ) return -DAR_OTHERS_ERROR;
                    
                    p = p + 2;
                    for(i=0; i<7; i++)
                    {
                        p++;
                        mem_swap_cpy((uint8_t*)idbuf[i], p, lens[i]);
                        p = p + lens[i];
                    }
                    
                    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, backlight_key_time));
                    uint16_t s_len = offsetof(struct mbase_control_block_t, Baut_plc) - offsetof(struct mbase_control_block_t, backlight_key_time);
                    nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->backlight_key_time, s_len);
                    
                    return (p-buf);
                }
                else if(index < 8)
                {
                    switch(index)
                    {
                        case 0x01://全屏
                          if((*(p+1)<5)||(*(p+1)>30))  return -DAR_OTHERS_ERROR;
                          break;
                        case 0x05:
                        case 0x06://小数位数
                          if(*(p+1)>4)  return -DAR_OTHERS_ERROR;
                          break;
                        case 0x07://液晶12字样
                          if(*(p+1)>1)  return -DAR_OTHERS_ERROR;
                          break;
                    }
                  
                    p++;
                    mem_swap_cpy((uint8_t*)idbuf[index-1], p, lens[index-1]);
                    
                    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, lcd_poweron_all_dis_time) + offset[index-1]);
                    nd_set_offset(instance, s_index, p, lens[index-1]);
                    
                    p = p + lens[index-1];
                    return (p-buf);
                }
                else
                {
                    return -DAR_OTHERS_ERROR;
                }
            }
            else
            {
                return -DAR_OTHERS_ERROR;
            }
            break;
        case 0x400C://时区时段数
            if(index > 5)   return -DAR_OTHERS_ERROR;
            if(index == 0)
            {
                p = p + 2;
                for(i=0;i<5;i++)
                {
                    dar = CheckData400C(p,i);
                    if( dar != 0 ) return -dar;
                    p++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + i);
                    nd_set_index(instance, s_index, p, 1); 
                    p++;
                }
                mbase_cb->Fee |= F_DAYTABChange;
                return (p-buf);
            }
            else
            {
                dar = CheckData400C(p,index-1);
                if( dar != 0 ) return -dar;
                p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + (index - 1));
                nd_set_index(instance, s_index, p, 1); 
                p++;
                mbase_cb->Fee |= F_DAYTABChange;
                return (p-buf);
            }
            break;
        case 0x4011://公共假日表
            if((index == 0) || (index > 20))    return -DAR_RW_REJECT;
            
            mem_swap_cpy((uint8_t*)&year2, p+3, 2);
            if(year2 < 2000) return -DAR_OTHERS_ERROR;
            year2 = year2 - 2000;
            timer[0] = *(p+6); //day
            timer[1] = *(p+5); //month
            timer[2] = (uint8_t)year2;
            
            if(Date_Format_Check(timer))  return -DAR_OTHERS_ERROR;
            if(*(p+7) != GetWeek(timer))    return -DAR_RW_REJECT;//周错误

            // 编程事件-保存编程前
            nd_get(instance, id, temp, 10, NULL);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_Holiday);
            nd_set_index(instance, s_index, temp, 10);                                          

            temp[0] = TYPE_OAD;
            uint32_to_buf_msb(id, &temp[1]);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Set_Holiday_Oad);
            nd_set_index(instance, s_index, temp, 5);
       
            p = p + 3;
            memcpy(buffer, p, 5);
            p = p + 6;
            buffer[5] = *p++;
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + (index-1));
            nd_set_index(instance, s_index, buffer, 6); 

            send_event_mesg_pro(instance, NULL, HolidayWrS, NULL, 1);// 公共假日编程事件
            mbase_cb->Fee |= F_DAYTABChange;
            return (p-buf);
            break;
        case 0x4014://当前套时区表
        case 0x4015://备用套时区表
            if(index > 14)    return -DAR_OTHERS_ERROR;
            if((oad == 0x4014) && (mbase_cb->factor_mode == false))    return    -DAR_RW_REJECT;
            if(index == 0)
            {
                num = *(p+1);
                if(num>14) num = 14;
                for(i=0; i<num; i++)
                {
                    if((*(p+2+8*i+3) > 12) || (*(p+2+8*i+3) == 0)) return -DAR_RW_REJECT;
                    if((*(p+2+8*i+5) > 31) || (*(p+2+8*i+5) == 0)) return -DAR_RW_REJECT;
                    if(*(p+2+8*i+5) > MonTab[*(p+2+8*i+3)]) return -DAR_RW_REJECT;
                }
            }
            else
            {
                if((*(p+3) > 12) || (*(p+3) == 0))  return -DAR_RW_REJECT;
                if((*(p+5) > 31) || (*(p+5) == 0))  return -DAR_RW_REJECT;
                if(*(p+5) > MonTab[*(p+3)])     return -DAR_RW_REJECT;
            }
            
            if(oad == 0x4014)   
                addr = Season_Table0;
            else 
                addr = Season_Table1;

            // 编程事件-保存编程前
            if (oad == 0x4015)
            {
                nd_get(instance, 0x40150200, temp, 114, NULL);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_SeasonT);
                nd_set_index(instance, s_index, temp, 114);
            }

            if(index == 0)
            {
                num = *(p+1);
                if(num>14) num = 14;
                p = p + 2;
                for(i=0; i<num; i++)
                {
                    p = p + 3;
                    buffer[0] = *p++;
                    p++;
                    buffer[1] = *p++;
                    p++;
                    buffer[2] = *p++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
                    nd_set_index(instance, s_index, buffer, 3);
                }
                if(num < 14)
                {
                    for(i=num; i<14; i++)
                    {
                        buffer[0+i*3] = buffer[0+(num-1)*3];
                        buffer[1+i*3] = buffer[1+(num-1)*3];
                        buffer[2+i*3] = buffer[2+(num-1)*3];
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
                        nd_set_index(instance, s_index, buffer, 3);
                    }
                }
            }
            else
            {
                p = p + 3;
                buffer[0] = *p++;
                p++;
                buffer[1] = *p++;
                p++;
                buffer[2] = *p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                nd_set_index(instance, s_index, buffer, 3);
            }
            send_event_mesg_pro(instance, NULL, SeasonTabWrS, NULL, 1);// 
            mbase_cb->Fee |= F_DAYTABChange;
            return (p - buf);
            break;
        case 0x4016://当前套日时段表
        case 0x4017://备用套日时段表
            if((index > 0x08) || (index == 0))    return -DAR_RW_REJECT;
            if((oad == 0x4016) && (mbase_cb->factor_mode == false))    return    -DAR_RW_REJECT;
            
            if(*(p+5) > 23)  return -DAR_RW_REJECT;
            if(*(p+7) > 59)  return -DAR_RW_REJECT;
            
            if(oad == 0x4016)
                addr = Day_Table00;
            else
                addr = Day_Table10;

            // 编程事件-保存编程前
            if (oad == 0x4017)
            {
                nd_get(instance, id, temp, 114, NULL);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_DayTab);
                nd_set_index(instance, s_index, temp, 114);

                temp[0] = TYPE_OAD;
                uint32_to_buf_msb(id, &temp[1]);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Set_DayTab_Oad);
                nd_set_index(instance, s_index, temp, 5);
            }

/*            if(index == 0)
            {
                p++;
                num = *p++;//
                for(j=0;j<num;j++)
                {
                    p++;
                    num_hourtab = *p++; //日时段数，每日切换数
                    for(i=0; i<num_hourtab; i++)
                    {
                        p = p + 3;
                        buffer[0+i*3] = *p++;
                        p++;
                        buffer[1+i*3] = *p++;
                        p++;
                        buffer[2+i*3] = *p++;
                    }
                    if(num_hourtab < 14)
                    {
                        for(i=num_hourtab; i<14; i++)
                        {
                            buffer[0+(num_hourtab-1+i)*3] = buffer[0+(num_hourtab-1)*3];
                            buffer[1+(num_hourtab-1+i)*3] = buffer[1+(num_hourtab-1)*3];
                            buffer[2+(num_hourtab-1+i)*3] = buffer[2+(num_hourtab-1)*3];
                        }
                    }
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + j);
                    nd_set_index(instance, s_index, buffer, 42);
                }
            }
            else*/
            {
                p++;
                num_hourtab = *p++; //日时段数，每日切换数
                for(i=0; i<num_hourtab; i++)
                {
                    p = p + 3;
                    buffer[0+i*3] = *p++;
                    p++;
                    buffer[1+i*3] = *p++;
                    p++;
                    buffer[2+i*3] = *p++;
                }
                if(num_hourtab < 14)
                {
                    for(i=num_hourtab; i<14; i++)
                    {
                        buffer[0+i*3] = buffer[0+(num_hourtab-1)*3];
                        buffer[1+i*3] = buffer[1+(num_hourtab-1)*3];
                        buffer[2+i*3] = buffer[2+(num_hourtab-1)*3];
                    }
                }
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index - 1));
                nd_set_index(instance, s_index, buffer, 42);
                
            }

            send_event_mesg_pro(instance, NULL, DayTabWrS, NULL, 1);// 时段表编程事件
                
            mbase_cb->Fee |= F_TOUChange;
            return (p - buf);
            break;
        
        case 0x4116://结算日
            //-保存编程前
            nd_get(instance, 0x41160200, temp, 20, NULL);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_Balance);
            nd_set_index(instance, s_index, temp, 20);

            if(index == 0)
            {
                if(*(p+1) > 0x03)    return -DAR_RW_REJECT;
                num = *(p+1);
                p = p + 2;
                for(i=0;i<num;i++)
                {
                    p = p + 2;
                    p++;//tag
                    buffer[0] = *p++;
                    if(((buffer[0] > 28) && (buffer[0] != 99)) || (buffer[0] == 0))  return -DAR_OTHERS_ERROR; 
                    p++;//tag
                    buffer[1] = *p++;
                    if((buffer[1] > 23) && (buffer[1] != 99))  return -DAR_OTHERS_ERROR;
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                    nd_set_index(instance, s_index, buffer, 2); 
                    
                    memset(buffer, 0xaa, 2);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                    nd_get_index(instance, s_index, buffer, 2);
                    if(i == 0)
                    {
                        send_event_mesg_pro(instance, NULL, BillFreeze, NULL, 1);//
                    }
                }

                send_event_mesg_pro(instance, NULL, BillDayWrS, NULL, 1);//
                return (p-buf);
            }
            else if(index < 4)
            {
                p = p + 2;
                p++;//tag
                buffer[0] = *p++;
                if(((buffer[0] > 28) && (buffer[0] != 99)) || (buffer[0] == 0))  return -DAR_OTHERS_ERROR; 
                p++;//tag
                buffer[1] = *p++;
                if((buffer[1] > 23) && (buffer[1] != 99))  return -DAR_OTHERS_ERROR;
                
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + (index - 1));
                nd_set_index(instance, s_index, buffer, 2);
                if(index == 1)
                {
                    send_event_mesg_pro(instance, NULL, BillFreeze, NULL, 1);//
                }
                
                send_event_mesg_pro(instance, NULL, BillDayWrS, NULL, 1);//
                return (p-buf);
            }
            else
                return -DAR_OBJECT_NOT_EXIST;
            break;
        case 0x4300://设备信息
            return set4300(instance, id, buf);
            
        case 0x4400:
            switch(attribute)
            {
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05://认证连接机制
                    return -DAR_RW_REJECT;
                    break;
                default:
                    return -DAR_OBJECT_NOT_EXIST;
                    break;
            }
            break;
        default:
            return -DAR_OBJECT_NOT_EXIST;
            break;
    }
    if(len != 0)
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + s_index);
        nd_set_index(instance, addr, p, len); 
        p = p + len;
    }
    return (p-buf);
}

/*
struct inis_para_t 
{
    uint8_t Device_descriptor[8];           979 //设备描述符
    uint8_t Factory_No[8];                  980 //厂家代码
    uint8_t Version_SoftWare[2];            981 //软件版本
      uint32_t xx[];
}


const struct inis_para_t inis_para =
{
  {0,0,0,},  //设备描述符
  {},  //厂家代码
  {},  //软件版本
}
*/
