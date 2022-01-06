#include "nidavellir.h"
#include "applications.h"
#include "resource_tool.h"
#include "target.h"

#define     P_BACKLIGHT_ON      GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT |= 0x10; //推挽输出高
#define     P_BACKLIGHT_OFF     GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT &=~0x10; //推挽输出低

#if 1  //  GS




enum scheme_mode
{
    FRZ_1 = 0,  //顺序固定
    FRZ_2,
    FRZ_3,
    FRZ_4,
    FRZ_5,
    FRZ_6,
    FRZ_7,
    FRZ_8,

    START_MODE = 10,
    END_MODE = 11,
};


//record_head 记录头 首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11位 结束
int read_record_head(struct instance_t* instance, uint8_t* databuf, const uint32_t* record_head, enum scheme_mode mode, uint16_t *remain_size,  struct record_storage_id_info_t *record_id_info)
{
    uint8_t *p , i, scheme_valid = true,  normal_type;
    int16_t single_len = 0, transform_len;
    uint32_t obj,seq;
    
    
    p = databuf;
    if (record_head)
    {
        for (i = 0; i < (record_head[0]); i++)
        {
            normal_type = 1;
            scheme_valid = true;
            obj = record_head[i+1];
            switch (obj)
            {
                case 0x20220200: //事件记录序号
#if 0               //序号统一最后补     
                    nd_get_index(instance, record_id_info->sequence_index, (uint8_t *)&seq, sizeof(seq));
                    *p++ = 0x06;
                    memcpy(p, &seq, 4);
                    p += 4;
                    single_len = 1 + 4;
                    //normal_type = 0;
#endif                    
                    break;
                case 0x201E0200: //事件发生时间
                    if (mode == END_MODE)
                    {
                        scheme_valid = false;
                    }
                    break;
                case 0x20200200: //事件结束时间
                    if (mode == START_MODE)
                    {
                        scheme_valid = false;
                    }
                    break;
                default:
                    break;
            }

            if (normal_type)
            {
                single_len = 0;
                
                if (true == scheme_valid)
                {
                    single_len = nd_get(instance, obj, p, *remain_size);
                    if (single_len > 0)
                    {
                        p += single_len;
                    }
                    else
                    {
                        single_len = 1;
                        memset(p, 0, single_len); //找不到填 NULL 
                        p += single_len;
                    }
                }
                else
                {
                    single_len = nd_size(instance, obj);
                    if (single_len > 0)
                    {
                        memset(p, 0xFF, single_len);
                        p += single_len;
                    }
                    else
                    {
                        single_len = 1;
                        memset(p, 0, single_len); //找不到填 NULL 
                        p += single_len;
                        
                    }
                }
                *remain_size -= single_len;
            }
        }
    }
    return (p - databuf);
}
//方案号 & 属性特征 判断
//mode 0-7 为冻结方案， 10为 事件发生 11为 结束
uint8_t check_scheme(enum scheme_mode mode, uint32_t obj)
{
    uint8_t scheme, scheme_valid;
    scheme = (uint8_t)(0x07 & (uint8_t)(obj >> 13));
    scheme_valid = true;
    //记录头
    switch (obj)
    {
    case 0x20210200: //冻结时间
    case 0x20230200: //冻结记录
        break;
    case 0x20220200: //事件记录序号
        if (mode == END_MODE)
        {
            scheme_valid = false;
        }
        break;
    case 0x201E0200: //事件发生时间
        if (mode == END_MODE)
        {
            scheme_valid = false;
        }
        break;
    case 0x20200200: //事件结束时间
        if (mode == START_MODE)
        {
            scheme_valid = false;
        }
        break;
        
    default:
        //配置关联对象
        switch (mode)
        {
        case START_MODE:    //发生
            if ((scheme == 3) || (scheme == 4))
            {
                scheme_valid = false;
            }
            break;
        case END_MODE:      //结束
            if ((scheme == 1) || (scheme == 2))
            {
                scheme_valid = false;
            }
            break;
        default:
            if (mode != scheme)
            {
                scheme_valid = false;
            }
            break;
        }
        break;
    }

    return scheme_valid;

}
//capturelist  关联对象列表  首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11为 结束
int read_record_capture(struct instance_t* instance, uint8_t* databuf, uint32_t* CaptureList, enum scheme_mode mode, uint16_t* remain_size,  struct record_storage_id_info_t *record_id_info)
{
    uint8_t* p, i, scheme, scheme_valid = true;
    int16_t single_len = 0;
    uint32_t obj;
    uint8_t tmpbuf[200];//单个关联对象 最大长度
    int ret;

    p = databuf;
    if (CaptureList)
    {
        for (i = 0; i < (CaptureList[0]); i++)
        {
            obj = CaptureList[1 + i];
            scheme = (obj >> 13) & 0x07; 

            scheme_valid = true;
            scheme_valid = check_scheme(mode,obj); //方案号 & 属性特征 判断
            
            single_len = 0;
 
            if (true == scheme_valid)
            {
                single_len = nd_get(instance, obj, p, *remain_size);
                // format改变，长度改变
                ret = record_change_format(tmpbuf, p, obj, RFC_WRITE);
                if (ret >= 0)
                {
                    single_len = ret;
                    memcpy(p, tmpbuf, single_len);
                }
                if (single_len > 0)
                {
                    p += single_len;
                    *remain_size -= single_len;
                }
                else
                {
                    memset(p, 0, 1); //找不到填 NULL 
                    p += 1;
                    *remain_size -= single_len;
                }
            }
            else
            {
                single_len = nd_size(instance, obj);
                ret = record_change_format(tmpbuf, p, obj, RFC_GET_STORE_LEN);
                if (ret >= 0)
                {
                    single_len = ret;
                }
                if (single_len > 0)
                {
                    memset(p, 0xFF, single_len);
                    p += single_len;
                    *remain_size -= single_len;
                }
                else
                {
                    memset(p, 0, 1); //找不到填 NULL 
                    p += 1;
                    *remain_size -= single_len;
                }
            }
        }
    }
    return (p - databuf);
}

int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    enum scheme_mode mode;
    uint8_t data_buff[400] = { 0 }, payload[100];
    uint8_t *pdata = data_buff;
    uint16_t remain_size;
    uint32_t sequence,Oad;
    uint32_t CaptureList[50 + 1] = { 0 };
    struct record_storage_id_info_t record_id_info; 

    int ret;
      
    remain_size = sizeof(data_buff);
    
    //mesg 的id 就是事件代码
    int r = recv_mesg_header(instance, pipeline, mesg);
    
    if (r > 0)
    {
        Oad = 0xFFFFFFFF;
        fifoBuf_getData(pipeline->in_queue, payload, mesg->len);
        switch (mesg->id)
        {
            case OverCurrentAE:         //过流事件结束
            case OverCurrentAS:         //过流事件发生
                Oad = 0x30050700;
                mode = START_MODE + (mesg->id - OverCurrentAE);
                break;
 
            case CoverOpenE:            //开盖事件结束
            case CoverOpenS:            //开盖事件发生
                Oad = 0x301B0200;
                mode = START_MODE + (mesg->id - OverCurrentAE);
                break;

            case InstantFreeze:         //瞬时冻结
                Oad = 0x50030200;
                mode = FRZ_1;
                break;

            case MinuteFreeze1: //分钟冻结记录方案1
            case MinuteFreeze2: //分钟冻结记录方案2
            case MinuteFreeze3: //分钟冻结记录方案3
            case MinuteFreeze4: //分钟冻结记录方案4
            case MinuteFreeze5: //分钟冻结记录方案5
            case MinuteFreeze6: //分钟冻结记录方案6
            case MinuteFreeze7: //分钟冻结记录方案7
            case MinuteFreeze8: //分钟冻结记录方案8 
                uint8_t freez_scheme = (mesg->id - MinuteFreeze1);//(uint8_t)(0x07 & (uint8_t)(oad >> 13))
                Oad = 0x50020200 + (freez_scheme<<13);
                mode = FRZ_1 + freez_scheme;                
                break;     

            case SeasonTabCutFreeze:    //时区表切换冻结
            case DayTabCutFreeze:       //日时段表切换冻结
            case FeePriceCutFreeze:     //费率电价切换冻结
            case StepCutFreeze:         //阶梯切换冻结
            case MonthFreeze:           //月冻结
            case BillFreeze:            //结算日冻结
            case DayFreeze:             //日冻结
            case HourFreeze:            //小时冻结
                Oad = 0x50030200 + ((mesg->id - HourFreeze)<<16);
                mode = FRZ_1;
                break;

            case StepBillFreeze:        //阶梯结算冻结
                Oad = 0x50110200;
                mode = FRZ_1;
                break;
            default:

                break;
        }

        if (Oad == 0xFFFFFFFF)
        {
            return 0;
        }
        //以下部分共用
        //获取 存储地址信息
        ret = get_record_id_info(instance, Oad, &record_id_info);
        if (ret <= 0)
        {
            return -1;
        }

        //获取 关联对象列表
        get_capture_list(instance, Oad, CaptureList, 3);

        //读取关联对象数据
        pdata += read_record_capture(instance, pdata, CaptureList, mode, &remain_size, &record_id_info);

        // 读取序号 ++
        nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
        mem_swap_cpy(data_buff + 1, (uint8_t*)&sequence, 4);
        sequence++;
        nd_set_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));

        //调用ND_SET保存数据
        nd_log_append(instance, record_id_info.save_index, data_buff, (pdata - data_buff));

    }
    return 0;//返回0代表结束
}

#endif

void SecTick_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    
    
    if(mbase_control_block->AutoInv > 0)   mbase_control_block->AutoInv--;
    
    if(mbase_control_block->KeyInv > 0)    mbase_control_block->KeyInv--;
    
    if(mbase_control_block->BackLightTmr > 0) mbase_control_block->BackLightTmr--;

/*    if(mbase_control_block->BackLightTmr == 0)    
    {
        P_BACKLIGHT_OFF;
    }
    else
    {
        P_BACKLIGHT_ON;
    }*/
    
    backlight_pro(instance, pipeline);
}

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_start) - offsetof(struct mbase_control_block_t, persistent_section_end);
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start);
    uint32_t addr = 0;
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景
    for (int i = 0; i < max_event_flag; i++)
    {
        if (event_flags_get(instance,i) ==  1)
        {
            //call event_handler(instance,pipeline,mesg);

            event_flags_clr(instance, i);
        }
    }
    
    int r = recv_mesg_header(instance,pipeline,mesg);
    uint8_t buf[20];
    if(r > 0 )
    {
      if((mesg->len > 0) && (mesg->len <20))
      {
        fifoBuf_getData(pipeline->in_queue,buf,mesg->len);
      }

      switch (mesg->ctrl)
      {
      case 0x86://采样的外设数据抵达
          //backlight_pro();
          break;
      case 0x87:
          break;
      case 0x88://control_block控制命令 比如清零
          //controlData(instance,pipeline,mesg);
          break;
      case MSG_SECOND:
          SecTick_Task(instance, pipeline);
          
          Rtc_Prossecc(instance, pipeline);
          
          Electrical_measurement(instance, pipeline);
          
          Energy_Pluse_Count(instance, pipeline);
          
          Energy_Meter(instance, pipeline);

          ia_over_event_judge(instance, pipeline, mesg); //TEST
          
          freeze_pro(instance, pipeline, mesg);

          Display(instance, pipeline);
          
          do_relay(instance, pipeline);
          
          IOStateCheck(instance, pipeline);

          break;
      case MSG_PAGE_KEY_UP:
           Key_Display(instance, pipeline);
          break;
      case MSG_POWERON:
        //上电恢复参数
        get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
        nd_get_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  
        //上电需要恢复数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
        nd_get_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将EPROM中的数据恢复到RAM;        
        Electrical_measurement_poweron(instance, pipeline);
        AllDisplayLCD(instance, pipeline);
        Magnetic_hold_relay_Init();
        break;
      case MSG_POWEROFF:
        //掉电需要保存数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
        nd_set_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;        
        break;
      default:
        break;
      }
    }

    return 0;
}



