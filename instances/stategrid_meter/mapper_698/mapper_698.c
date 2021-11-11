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

