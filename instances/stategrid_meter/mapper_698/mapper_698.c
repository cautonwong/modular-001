#include "nidavellir.h"
#include "fr_698.h"

extern int resource_map_698_OIA1_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_6_Collection(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_e_Monitor(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern int resource_map_698_OIA1_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);


int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb = instance->mbase_cb;

    switch (get_OAD_OIA1(id))
    {
    case 0x0://电量
		return resource_map_698_OIA1_0_Energy(instance, id, buf, mode, obj);
		break;
    case 0x1://需量
		return resource_map_698_OIA1_1_Demand(instance, id, buf, mode, obj);
		break;
    case 0x2://变量
		return resource_map_698_OIA1_2_Variable(instance, id, buf, mode, obj);
		break;
    case 0x3://事件
		return resource_map_698_OIA1_3_Event(instance, id, buf, mode, obj);
		break;
    case 0x4://参变量
		return resource_map_698_OIA1_4_Parameter(instance, id, buf, mode, obj);
		break;
    case 0x5://冻结
		return resource_map_698_OIA1_5_Frozen(instance, id, buf, mode, obj);
		break;
    case 0x6://采集
		return resource_map_698_OIA1_6_Collection(instance, id, buf, mode, obj);
		break;
    case 0x7://集合
		return resource_map_698_OIA1_7_Aggregate(instance, id, buf, mode, obj);
		break;
    case 0x8://控制
		return resource_map_698_OIA1_8_Control(instance, id, buf, mode, obj);
		break;
    case 0xe://监控
		return resource_map_698_OIA1_e_Monitor(instance, id, buf, mode, obj);
		break;
	case 0xf://其他
		return resource_map_698_OIA1_f_Other(instance, id, buf, mode, obj);
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
     {  DTC_FLOATING_POINT          ,  4    },
     {  DTC_OCTET_STRING            ,  0xFF    },
     {  DTC_VISIBLE_STRING          ,  0xFF    },
     {  DTC_UTF8_STRING             ,  0xFF    },
     {  DTC_BCD                     ,  1    },
     {  TYPE_integer                ,  1    },
     {  TYPE_long                   ,  2    },
     {  TYPE_unsigned               ,  1    },
     {  TYPE_long_unsigned          ,  2    },
     {  DTC_COMPACT_ARRAY           ,  0    },
     {  TYPE_long64                 ,  8    },
     {  TYPE_long64_unsigned        ,  8    },
     {  DTC_ENUM                    ,  0    },
     {  DTC_FLOAT32                 ,  4    },
     {  DTC_FLOAT64                 ,  8    },
     {  DTC_DATE_TIME               ,  10    },
     {  DTC_DATE                    ,  0    },
     {  DTC_TIME                    ,  0    },
     {  DTC_DATETIME_S              ,  7    },
     {  DTC_OI                      ,  2    },
     {  DTC_OAD                     ,  4    },
     {  DTC_ROAD                    ,  0    },
     {  DTC_OMD                     ,  4    },
     {  DTC_TI                      ,  0    },
     {  DTC_TSA                     ,  0    },
     {  DTC_MAC                     ,  0    },
     {  DTC_RN                      ,  0    },
     {  DTC_REGION                  ,  0    },
     {  DTC_SCALER_UNIT             ,  0    },
     {  DTC_RSD                     ,  0    },
     {  DTC_CSD                     ,  0    },
     {  DTC_MS                      ,  0    },
     {  DTC_SID                     ,  0    },
     {  DTC_SID_MAC                     ,  0    },
     {  DTC_COMDCB                      ,  0    },
     {  DTC_RCSD                        ,  0    },
     {  DTC_VQDS                        ,  0    },
     {  DTC_USER_HIGH_ENERGY_INTEGER    ,  0    },
     {  DTC_USER_HIGH_ENERGY_UNSIGNED   ,  0    },
     {  DTC_USER_NOT_CARE               ,  0    },
     {  DTC_USER_SPECIAL_PROCESS        ,  0    },
     {  DTC_USER_PUSH_PROCESS           ,  0    },
     {  DTC_USER_ERROR                  ,  0    },
};



const uint8_t _0000_02_tag[] = { TYPE_array, 13, TYPE_double_long, DT_END };
const uint8_t _0000_04_tag[] = { TYPE_array, 13, TYPE_long64, DT_END };
const uint8_t _0000_06_tag[] = { TYPE_array, 13, TYPE_integer, DT_END };
const struct attr_tag_t _0000_tag[] = { {2, _0000_02_tag}, {4, _0000_04_tag}, {6, _0000_06_tag} };

const uint8_t _2022_02_tag[] = { TYPE_double_long_unsigned, DT_END };
const struct attr_tag_t _2022_tag[] = { {2, _2022_02_tag}};

const uint8_t _201E_02_tag[] = { DTC_DATETIME_S, DT_END };
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