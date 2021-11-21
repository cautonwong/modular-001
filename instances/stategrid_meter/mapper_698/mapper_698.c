#include "nidavellir.h"
#include "fr_698.h"

extern void* get_pipeline_ctx(struct instance_t* instance, pid_t pipeline_id);
extern int GET_0_Energy(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_2_Variable(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);

extern int GET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_6_Collection(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_8_Control(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_e_Monitor(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);
extern int GET_f_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);


extern int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj);

int resource_map_698_get(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb = NULL;
    
    mbase_cb = get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }

    switch (get_OAD_OIA1(id))
    {
    case 0x0://电量
		return GET_0_Energy(instance, id, buf, obj);
		break;
    case 0x1://需量
		return GET_1_Demand(instance, id, buf, obj);
		break;
    case 0x2://变量
		return GET_2_Variable(instance, id, buf,obj);
		break;
    case 0x3://事件
		return GET_3_Event(instance, id, buf,obj);
		break;
    case 0x4://参变量
		return GET_4_Parameter(instance, id, buf,obj);
		break;
    case 0x5://冻结
		return GET_5_Frozen(instance, id, buf, obj);
		break;
    case 0x6://采集
		return GET_6_Collection(instance, id, buf, obj);
		break;
    case 0x7://集合
		return GET_7_Aggregate(instance, id, buf, obj);
		break;
    case 0x8://控制
		return GET_8_Control(instance, id, buf, obj);
		break;
    case 0xe://监控
		return GET_e_Monitor(instance, id, buf, obj);
		break;
	case 0xf://其他
		return GET_f_Other(instance, id, buf, obj);
		break;
    default:
        break;
    }
    return -1;
}

int resource_map_698_set(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{
	switch (get_OAD_OIA1(id))
	{
    case 0x4://参变量
		return SET_4_Parameter(instance, id, buf,obj);
    case 0x3:// 事件
        return SET_3_Event(instance, id, buf, obj);
	default:
		break;
	}
	return -1;
}

int resource_map_698_action(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{
	switch (get_OAD_OIA1(id))
	{
    case 0x4://参变量
		return ACTION_4_Parameter(instance, id, buf,obj);
	default:
		break;
	}
	return -1;
}

int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj)
{
	switch (mode)
	{
	case RESOURCE_MAP_698_GET:  //读
		return resource_map_698_get(instance,id,buf,obj);
		break;
	case RESOURCE_MAP_698_SET:  //写
		return resource_map_698_set(instance, id, buf, obj);
		break;
	case RESOURCE_MAP_698_ACTION:  //执行
		return resource_map_698_action(instance, id, buf, obj);
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