#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"

extern void* get_pipeline_ctx(struct instance_t* instance, pid_t pipeline_id);

extern int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf);

extern int GET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_6_Collection(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_e_Monitor(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf);
extern int SET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf);



struct record_item_t
{
    uint16_t  event_flag_id_start;  // 事件代码
    uint16_t  event_flag_id_end;    // 事件代码2
    uint32_t len;
    uint32_t max_depth;     // 默认最大深度
};

struct parameter_item_t
{
    uint8_t paramerer_index;
    uint16_t len;
};

//一个表格只能放在一个存储器上，注册的时候绑定设备
const struct record_item_t  record_item[] =
{
    { OverCurrentL1, OverCurrentEndL1,              800,    10 },// 待调整
};

const struct parameter_item_t  parameter_item[] =
{
    { OverCurrentL1,       sizeof(struct log_manager_t)},// 管理区

    { MinuteFreeze1,       sizeof(struct log_manager_t)},// 分冻结8个方案，管理区申请8个？
    { MinuteFreeze2,       sizeof(struct log_manager_t)},
    { MinuteFreeze3,       sizeof(struct log_manager_t)},
    { MinuteFreeze4,       sizeof(struct log_manager_t)},
    { MinuteFreeze5,       sizeof(struct log_manager_t)},
    { MinuteFreeze6,       sizeof(struct log_manager_t)},
    { MinuteFreeze7,       sizeof(struct log_manager_t)},
    { MinuteFreeze8,       sizeof(struct log_manager_t)},


    { OverCurrentL1_oad,   61 }, //oad列表 1+ 4*15     默认8个     先统一预留15个关联
    { MinuteFreeze_oad,    61 }, //oad列表 1+ 4*15     默认7个

    { OverCurrentL1_index, 4  },//记录序号

    { MinuteFreeze_period,  24 }, //分钟冻结 周期   3*8     
    { MinuteFreeze_depth,   31}, //分钟冻结 深度     2*15  统一冻结深度31字节

};

int16_t supply_parameter_info(uint16_t id, uint32_t* addr, uint16_t* size)
{
    uint8_t i, k;

    for (i = 0; i < sizeof(parameter_item) / sizeof(parameter_item[0]); i++)
    {
        if (parameter_item[i].paramerer_index == id)
        {
            if (addr != NULL)
            {
                *addr = 0x00;
                for (k = 0x00; k < i; k++)
                {
                    *addr += parameter_item[k].len;
                }
            }

            if (size != NULL)
            {
                *size = parameter_item[i].len;
            }

            return 0x00;
        }
    }

    return -1;
}


int16_t supply_event_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
    uint8_t i, k;

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




// 通过oad 获取 事件相关的存储地址信息
int get_record_id_info(uint32_t oad, struct record_storage_id_info_t* info)
{
 
    uint16_t CaptureId, SaveId, SeqId;
    uint8_t SaveDevice, SaveBlock;

    switch (oad)
    {
    case 0x30050700:
        SaveId = OverCurrentL1;            // 记录存储地址信息
        SaveDevice = RES_DEV_EEPROM;
        SaveBlock = mbase_block_event;     
        CaptureId = OverCurrentL1_oad;     // 关联对象存储地址信息
        SeqId = OverCurrentL1_index;       // 序号存储地址信息
        break;
    default:
        return -1;
        break;
    }

    info->save_index = ((SaveDevice << 24) + (SaveBlock << 16) + SaveId);
    info->capture_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + CaptureId);
    info->sequence_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeqId);
    return 1;
}


int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode)
{
    uint8_t OIA1 = get_OAD_OIA1(id);
    switch (mode)
	{
    case RESOURCE_MAP_698_GET:
        switch (OIA1)
        {
        case 0x0://电量
            return GET_0_Energy(instance, id, buf);
            break;
        case 0x1://需量
            return GET_1_Demand(instance, id, buf);
            break;
        case 0x2://变量
            return GET_2_Variable(instance, id, buf);
            break;
        case 0x3://事件
            return GET_3_Event(instance, id, buf);
            break;
        case 0x4://参变量
            return GET_4_Parameter(instance, id, buf);
            break;
        case 0x5://冻结
            return GET_5_Frozen(instance, id, buf);
            break;
        case 0x6://采集
            return GET_6_Collection(instance, id, buf);
            break;
        case 0x7://集合
            return GET_7_Aggregate(instance, id, buf);
            break;
        case 0x8://控制
            return GET_8_Control(instance, id, buf);
            break;
        case 0xe://监控
            return GET_e_Monitor(instance, id, buf);
            break;
        case 0xf://其他
            return GET_f_Other(instance, id, buf);
            break;
        default:
            break;
        }
        break;
    case RESOURCE_MAP_698_SET:
      	switch (OIA1)
        {
        case 0x3:// 事件
            return SET_3_Event(instance, id, buf);
        case 0x4://参变量
            return SET_4_Parameter(instance, id, buf);
        case 0x5:// 事件
            return SET_5_Frozen(instance, id, buf);
        case 0xf://其他
            return SET_f_Other(instance, id, buf);
        default:
            break;
        }
        break;
    case RESOURCE_MAP_698_ACTION:
        switch (OIA1)
        {
        case 0x4://参变量
            return ACTION_4_Parameter(instance, id, buf);
        default:
            break;
        }
        break;
	default:
		break;
	}
	return -1;
}

#if 0
#define DT_END 0xED

#define calnum(x)                    (sizeof(x) / sizeof(x[0]))

struct attr_tag_t
{
    uint8_t attr;
    const uint8_t* tags;
};

struct data_tag_t
{
    uint16_t oi1;
    uint16_t oi2;
    const struct attr_tag_t *dtags;
    uint8_t number;
    uint8_t info;
};

struct tag_len_t
{
    uint8_t tag;
    uint8_t datalen;
};


//注意 长度  0的 可能不需要，也可能未 核对。
const struct tag_len_t tag_data_len[] =
{
     {  TYPE_null                   ,  0    },
     {  TYPE_array                  ,  0    },
     {  TYPE_structure              ,  0    },
     {  TYPE_bool                   ,  0    },
     {  TYPE_bit_string             ,  0    },
     {  TYPE_double_long            ,  4    },
     {  TYPE_double_long_unsigned   ,  4    },

     {  TYPE_octet_string            ,  0xFF    },
     {  TYPE_visible_string          ,  0xFF    },
     {  TYPE_UTF8_string             ,  0xFF    },

     {  TYPE_integer                ,  1    },
     {  TYPE_long                   ,  2    },
     {  TYPE_unsigned               ,  1    },
     {  TYPE_long_unsigned          ,  2    },

     {  TYPE_long64                 ,  8    },
     {  TYPE_long64_unsigned        ,  8    },
     {  TYPE_enum                    ,  0    },
     {  TYPE_float32                 ,  4    },
     {  TYPE_float64                 ,  8    },
     {  TYPE_date_time               ,  10    },
     {  TYPE_date                    ,  0    },
     {  TYPE_time                    ,  0    },
     {  TYPE_date_time_s              ,  7    },
     {  TYPE_OI                      ,  2    },
     {  TYPE_OAD                     ,  4    },
     {  TYPE_ROAD                    ,  0    },
     {  TYPE_OMD                     ,  4    },
     {  TYPE_TI                      ,  0    },
     {  TYPE_TSA                     ,  0    },
     {  TYPE_MAC                     ,  0    },
     {  TYPE_RN                      ,  0    },
     {  TYPE_Region                  ,  0    },
     {  TYPE_Scaler_Unit             ,  0    },
     {  TYPE_RSD                     ,  0    },
     {  TYPE_CSD                     ,  0    },
     {  TYPE_MS                      ,  0    },
     {  TYPE_SID                     ,  0    },
     {  TYPE_SID_MAC                     ,  0    },
     {  TYPE_COMDCB                      ,  0    },
     {  TYPE_RCSD                        ,  0    },
     {  TYPE_VQDS                        ,  0    },

     {  TYPE_USER_HIGH_ENERGY_INTEGER    ,  0    },
     {  TYPE_USER_HIGH_ENERGY_UNSIGNED   ,  0    },
     {  TYPE_USER_NOT_CARE               ,  0    },
     {  TYPE_USER_SPECIAL_PROCESS        ,  0    },
     {  TYPE_USER_PUSH_PROCESS           ,  0    },
     {  TYPE_USER_ERROR                  ,  0    },
};



const uint8_t _0000_02_tag[] = { TYPE_array, 13, TYPE_double_long, DT_END };
const uint8_t _0000_04_tag[] = { TYPE_array, 13, TYPE_long64, DT_END };
const uint8_t _0000_06_tag[] = { TYPE_array, 13, TYPE_integer, DT_END };
const struct attr_tag_t _0000_tag[] = { {2, _0000_02_tag}, {4, _0000_04_tag}, {6, _0000_06_tag} };

const uint8_t _2022_02_tag[] = { TYPE_double_long_unsigned, DT_END };
const struct attr_tag_t _2022_tag[] = { {2, _2022_02_tag}};

const uint8_t _201E_02_tag[] = { TYPE_date_time_s, DT_END };
const struct attr_tag_t _201E_tag[] = { {2, _201E_02_tag} };

const uint8_t _NULL_02_tag[] = { TYPE_null, DT_END };
const struct attr_tag_t _NULL_tag[] = { {2, _NULL_02_tag} };

const struct data_tag_t data_tag[] =
{
    {   0x0000, 0x0000,    _0000_tag,  calnum(_0000_tag),  0    }, // 组合有功
    {   0x201E, 0x2021,    _201E_tag,  calnum(_201E_tag),  0    }, // 记录时间
    {   0x2022, 0x2023,    _2022_tag,  calnum(_2022_tag),  0    }, // 记录序号
    {   0x2024, 0x2024,    _NULL_tag,  calnum(_NULL_tag),  0    }, // 事件发生源
    {   0x3300, 0x3300,    _NULL_tag,  calnum(_NULL_tag),  0    }, // 事件上报状态
    //待添加

};

const uint8_t*  get_tag(uint32_t oad)
{
    uint16_t i, oi;
    uint8_t index, attr, num, info;
    const struct attr_tag_t* data_tags;
    
    oi = (oad & 0xFFFF0000) >> 16;
    index = oad & 0x000000FF;
    attr = (oad & 0x0000FF00) >> 8;
    num = 0;

    // -------------get tag start
    for (i = 0; i < calnum(data_tag); i++)
    {
        if ((oi >= data_tag[i].oi1) && (oi <= data_tag[i].oi2))
        {
            data_tags = data_tag[i].dtags;
            num = data_tag[i].number;
            info = data_tag[i].info;
            break;
        }
    }

    if (i >= calnum(data_tag))
    {
        return _NULL;
    }

    // 不同的索引tag 不一样
    for (i = 0; i < num; i++)
    {
        if (attr == data_tags->attr)
        {
            return data_tags->tags;
        }
    }

    return _NULL;

}
int add_tag(uint8_t* des, uint8_t* src, uint8_t* tags)
{
    uint16_t i, oi;
    uint8_t index,attr,num,info;
    const struct attr_tag_t* data_tags;
    uint8_t* attr_tags;

    uint8_t * p, * tmp;

    p = des;
    tmp = src;

    
    //*attr_tags



}
//  obj_num 关联对象个数 , srclen 源数据（不带ta）总长度， des_size目标
int record_data_add_tag(uint8_t* des, uint8_t* src, uint32_t* obj_list, uint8_t obj_num, uint16_t srclen, uint16_t des_size)
{
    uint8_t i, *p, *tmp;
    uint16_t taglen;
    uint32 oad;
    uint8_t* attr_tags;
    
    p = des;
    tmp = src;
    // 逐个查询obj list 获取所有tag长度
    for (i = 0; i < obj_num; i++)
    {
        oad = *obj_list++;
        
        attr_tags = get tag(oad);    
        add_tag(p, tmp, oad, attr_tags);

        
    }



}
// 00000200  00000400 
// 01 13 06 
// 00000001 00000001 00000001 00000001
// 01 13 06  XXXX 06 XXXX

#endif