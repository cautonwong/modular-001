#include "nidavellir.h"
#include "applications.h"

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
const uint32_t snapshot_head_t1[] = {5, 0x20220200, 0x201E0200, 0x20200200, 0x20240200, 0x33000200};

// 分项事件记录头    事件记录序号、事件发生时间、事件结束时间、事件上报状态
const uint32_t snapshot_head_t2[] = {4, 0x20220200, 0x201E0200, 0x20200200, 0x33000200};

// 其他事件记录头种类 待添加


struct snapshot_item_t
{
    uint16_t  event_flag_id_start;  // 事件代码
    uint16_t  event_flag_id_end;    // 事件代码2
    uint16_t capture_addr;  // 关联对象列表（e2p地址） 内容格式 1（数量n）+ n*4 （n个关联对象） 
    uint16_t save_addr;     // 快照存储地址 e2p：实际地址   flash：起始扇区数
    uint16_t len;           // 存储空间大小 e2p：字节数   flash：扇区数
    uint16_t max_depth;     // 默认最大深度
    const uint32_t*  head_type;     // 快照记录头类型
    uint8_t  info;          // 存储器件 BIT0:e2p  BIT1: flash;    其他保留 
};

const struct snapshot_item_t  snapshot_item[] = 
{
    // 事件代码       // 事件代码           // 关联对象列表（e2p地址）     // 记录存储地址     // 存储空间大小   // 默认最大深度 // 记录头类型     //info   
    {OverCurrentL1,    OverCurrentEndL1,     CAPTURE_E2P_BASE_ADDR,      SNAPSHOT_E2P_B_ADDR,  800,                 10,           snapshot_head_t2,  E2P_MEM},
    
};

int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint8_t i,mode, obj_cnt, scheme, scheme_valid;
    uint8_t BuffTemp[200] = { 0 }, CaptureList[151] = { 0 },  payload[100] = { 0 }, data_buff[400] = { 0 };
    uint8_t *pdata = data_buff;
    uint16_t event_flag_id = 0, data_len = 0, single_len;
    uint32 obj;
    

    int r = recv_mesg(instance, pipeline, mesg, &payload[0]);
    if (r > 0)
    {
        event_flag_id = mesg->id; 
   
        // 查表 
        for (uint8_t i = 0; i < (sizeof(snapshot_item) / sizeof(snapshot_item[0])); i++)
        {
            if (event_flag_id == snapshot_item[i].event_flag_id_start)   
            {
                mode = 0; 
                break;
            }
            
            if (event_flag_id == snapshot_item[i].event_flag_id_end)
            {
                mode = 1;
                break;
            }
        }
        
        if (i == (sizeof(snapshot_item) / sizeof(snapshot_item[0])))
        {
            return 0;   // 未查到
        }
        
        // 获取 关联对象列表
        // read(); snapshot_item[i].capture_addr   to  CaptureList[i]  待添加

        // 捕获记录头  和 配置的关联对象 
        obj_cnt = 0;
        data_len = 0;
        for (i = 0; i < (snapshot_item[i].head_type[0] + CaptureList[0]); i++)
        {
            scheme_valid = false;
            if (i < snapshot_item[i].head_type[0])
            {
                obj = snapshot_item[i].head_type[1 + i];
                scheme = 0xFF; //方案FF 固定要存
                scheme_valid = true;
            }
            else
            {
                obj = read32_lsb(CaptureList[1 + 4 * i]);
                scheme = (obj >> 13) & 0x07; //方案号

                // 事件冻结需要分开处理  部分特殊事件仍需特殊处理  
                if ((mode == 0) && ((scheme == 1) || (scheme == 2))) // 发生前或发生后
                {
                    scheme_valid = true;
                }

                if ((mode == 1) && ((scheme == 3) || (scheme == 4))) // 结束前或结束后
                {
                    scheme_valid = true;
                }

            }
 
            // 如果有payload，优先用payload数据内容。 特殊处理 待添加 

            if (scheme_valid)
            { 
                obj &= 0xFF1FFFFF;
                single_len  = nd_get(instance, obj, &BuffTemp[0], sizeof(BuffTemp)); // 返回值不是长度?
                if ((data_len + single_len) > sizeof(data_buff))
                {
                    break;
                }
                memcopy((pdata + data_len), &BuffTemp[0], single_len);
                data_len += single_len;
                obj_cnt++;
            }
        }

        //write (event_flag_id, pdata, data_len)       待添加
        return 1;
    }
    return 0;
}

#endif

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
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
        
        Electrical_measurement(instance, pipeline);
        
        Energy_Meter(instance, pipeline);
        
        //Autodisplay(instance, pipeline);
        
        break;
    case MSG_PAGE_KEY_UP:
         
        //Keydisplay(instance, pipeline);
        break;
    default:
        break;
    }

    //sync_data()
    return 0;
}



