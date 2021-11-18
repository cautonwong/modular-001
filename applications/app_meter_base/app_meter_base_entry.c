#include "nidavellir.h"
#include "applications.h"

extern int16_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size);
extern int16_t nd_set_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size);
extern int16_t get_block_frist_addr(struct instance_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr);
#if 1  //  GS
//存储器件
#define E2P_MEM  0x01
#define FLASH_MEM 0x02

//关联对象存储基地址
#define CAPTURE_E2P_BASE_ADDR   0x2000
//快照存储基地址
#define SNAPSHOT_E2P_B_ADDR   0x3000

//快照记录头类型
// 标准事件记录头   事件记录序号、事件发生时间、事件结束时间、时间发生源、事件上报状态
const uint32_t record_head_t0[] = {5, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200};

// 分项事件记录头    事件记录序号、事件发生时间、事件结束时间、事件上报状态
const uint32_t record_head_t1[] = {4, 0x20220200, 0x201E0200, 0x20200200, 0x33000200};
// 其他事件记录头种类 待添加




struct record_item_t
{
    uint16_t  event_flag_id_start;  // 事件代码
    uint16_t  event_flag_id_end;    // 事件代码2
    uint16_t len;
    uint16_t max_depth;     // 默认最大深度
};

//一个表格只能放在一个存储器上，注册的时候绑定设备
const struct record_item_t  record_item[] =
{
    { OverCurrentL1, OverCurrentEndL1,              800,    10 },
    { OverCurrentL1_oad, OverCurrentL1_oad,         151,    10 },//oad列表
    { OverCurrentL1_index, OverCurrentL1_index,     1,      10 },//index
};


int16_t register_event_info(uint16_t event_flag, uint32_t *addr, uint16_t* size, uint16_t* depth)
{
    uint8_t i,k;

    for (i = 0; i < sizeof(record_item) / sizeof(record_item[0]); i++)
    {
        if ((event_flag == record_item[i].event_flag_id_start) || (event_flag == record_item[i].event_flag_id_end))
        {
            if (addr != NULL)
            {
                *addr = 0x00;
                for (k = 0x00; k < i; k++)
                {
                    *addr += record_item[k].len;
                }               
            }

            if (size != NULL)
            {
                *size = record_item[i].len;
            }

            if (depth != NULL)
            {
                *depth = record_item[i].max_depth;
            }

            return 0x00;
        }
    }

    return -1;
}


#define START_MODE  10
#define END_MODE  11
//record_head 记录头 首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11位 结束
int get_record_head(struct instance_t* instance, uint8_t* databuf, const uint32_t* record_head, uint8_t mode, uint16_t *remain_size)
{
    uint8_t *p , i, scheme_valid = true;
    uint16_t single_len = 0;
    uint32_t obj;

    p = databuf;
    if (record_head)
    {
        for (i = 0; i < (record_head[0]); i++)
        {
            obj = record_head[i+1];
            switch (obj)
            {
                case 0x20220200: //事件记录序号
                    //空缺
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

            single_len = 0;

            if (true == scheme_valid)
            {
                single_len = nd_get(instance, obj, p, *remain_size);
                if (single_len > 0)
                {
                    p += single_len;
                    *remain_size -= single_len;
                }
            }
            else
            {
                single_len = nd_size(instance, obj);
                if (single_len > 0)
                {
                    memset(p, 0xFF, single_len);
                    p += single_len;
                    *remain_size -= single_len;
                }
            }
        }
    }
    return (p - databuf);
}
//capturelist 记录头 首字节为个数，后续为OAD，  
//mode 0-7 为冻结方案， 10为 事件发生 11位 结束
int get_record_association(struct instance_t* instance, uint8_t* databuf, uint8_t* CaptureList, uint8_t mode, uint16_t* remain_size)
{
    uint8_t* p, i, scheme, scheme_valid = true;
    uint16_t single_len = 0;
    uint32_t obj;

    p = databuf;
    if (CaptureList)
    {
        for (i = 0; i < (CaptureList[0]); i++)
        {
            obj = buf_to_uint32_lsb(&CaptureList[1 + 4 * i]);
            scheme = (obj >> 13) & 0x07; //方案号

            switch (mode)
            {
                case START_MODE:    //发生
                    if ((scheme == 3) || (scheme == 4))
                    {
                        scheme_valid = false;
                    }
                    break;
                case END_MODE:      //结束
                    if ((scheme == 1)|| (scheme == 2))
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


            single_len = 0;

            if (true == scheme_valid)
            {
                single_len = nd_get(instance, obj, p, *remain_size);
                if (single_len > 0)
                {
                    p += single_len;
                    *remain_size -= single_len;
                }
            }
            else
            {
                single_len = nd_size(instance, obj);
                if (single_len > 0)
                {
                    memset(p, 0xFF, single_len);
                    p += single_len;
                    *remain_size -= single_len;
                }
            }
        }
    }
    return (p - databuf);
}
int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint8_t i, mode;
    uint8_t CaptureList[151] = { 0 },  data_buff[400] = { 0 }, payload[100];
    uint8_t *pdata = data_buff;
    uint16_t remain_size, SequenceId, RecordId;
    uint32_t sequence;

    const uint32_t* record_head;
    uint32_t CaptureId;
      
    remain_size = sizeof(data_buff);
    
    //mesg 的id 就是事件代码
    int r = recv_mesg_header(instance, pipeline, mesg);
    if (r > 0)
    {
        fifoBuf_getData(pipeline->in_queue, payload, mesg->len);
        switch (mesg->id)
        {
            case OverCurrentL1:
                record_head = record_head_t0;
                RecordId = OverCurrentL1;
                CaptureId = OverCurrentL1_oad;
                SequenceId = OverCurrentL1_index;
                mode = START_MODE;

                //获取关联对象列表
                nd_get(instance, CaptureId, &CaptureList[0], sizeof(CaptureList));
                //组固定的事件头
                pdata += get_record_head(instance, pdata, record_head, mode, &remain_size);
                //组配置的关联对象
                pdata += get_record_association(instance, pdata, CaptureList, mode, &remain_size);

                // 读取序号 ++
                nd_get(instance, SequenceId, (uint8_t *)&sequence, sizeof(sequence));
                sequence++;
                memcpy(data_buff, (uint8_t*)sequence, 4);

                //调用ND_SET保存数据
                nd_set(instance, RecordId, data_buff, (pdata - data_buff));
                nd_set(instance, SequenceId, &sequence, sizeof(sequence));
                break;
            case OverCurrentEndL1:
                record_head = record_head_t0;
                RecordId = OverCurrentL1;
                CaptureId = OverCurrentL1_oad;
                SequenceId = OverCurrentL1_index;
                mode = START_MODE;

                //获取关联对象列表
                nd_get(instance, CaptureId, &CaptureList[0], sizeof(CaptureList));
                //组固定的事件头
                pdata += get_record_head(instance, pdata, record_head, mode, &remain_size);
                //组配置的关联对象
                pdata += get_record_association(instance, pdata, CaptureList, mode, &remain_size);

                // 读取序号 ++
                nd_get(instance, SequenceId, (uint8_t *)&sequence, sizeof(sequence));
                sequence++;
                memcpy(data_buff, (uint8_t*)sequence, 4);

                //调用ND_SET保存数据
                nd_set(instance, RecordId, data_buff, (pdata - data_buff));
                nd_set(instance, SequenceId, &sequence, sizeof(sequence));
                break;
            default:

                break;
        }
    }
    return 0;//返回0代表结束
}

#endif

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_start) - offsetof(struct mbase_control_block_t, persistent_section_end);
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
          
          instance->time.tm_sec++;
          
          rtc_tm_normalize((struct tm*)&instance->time.tm_sec);
          
          Electrical_measurement(instance, pipeline);
          
          Energy_Meter(instance, pipeline);
          
          Display(instance, pipeline);
          ia_over_event_judge(instance, pipeline, mesg);
          break;
      case MSG_PAGE_KEY_UP:
           
          //Keydisplay(instance, pipeline);
          break;
      case MSG_POWERON:
        //上电需要恢复数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM,0x08,&addr);//RAM对应的地址
        nd_get_block(instance,RES_DEV_EEPROM,0x08,(uint8_t *)addr,mbase_block_persistent_size); //将EPROM中的数据恢复到RAM;        
        Electrical_measurement_poweron(instance, pipeline);
        break;
      case MSG_POWEROFF:
        //掉电需要保存数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM,0x08,&addr);//RAM对应的地址
        nd_set_block(instance,RES_DEV_EEPROM,0x08,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;        
        break;
      default:
        break;
      }
    }

    return 0;
}



