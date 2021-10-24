#include "nidavellir.h"
#include "resource_tool.h"

uint8_t look_up_list(struct instance_t* instance, struct skip_list_node_t* head_node, struct skip_list_last_node_t** last_node, uint16_t* key)
{
    uint8_t i = 0, level_size = 0x05, ret = 0x33;

    for (i = 0; i < level_size; i++)                                //遍历层级
    {
        if (head_node != NULL)
        {
            while ((head_node->key != key[i]) && (head_node->next != NULL))
            {
                head_node = head_node->next;                    //指向下一个节点
            }

            if (head_node->key == key[i])                       //找到了
            {
                ret = 0x34 + i;
                if (NULL != last_node)
                {
                    *last_node = (struct skip_list_last_node_t*)head_node;
                }                

                if (i == (level_size - 0x01))
                {
                    break;
                }

                head_node = (struct skip_list_node_t*)head_node->head_node;
                continue;
            }
        }
        break;
    }

    return ret;
}

uint8_t look_up_lists(struct instance_t* instance, struct app_info_list_t* app_list, struct app_info_list_t** list, struct skip_list_last_node_t** node, uint16_t* key)
{
    uint8_t ret = 0x33;

    while (NULL != app_list)
    {
        ret = look_up_list(instance, app_list->env_list, node, key);
        if (0x38 == ret)
        {
            if (NULL != list)
            {
                *list = app_list;
            }
            break;   
        }
        app_list = app_list->next;   //指向下一个节点
    }

    return ret;
}

struct skip_list_node_t* add_new_node(struct instance_t* instance, struct skip_list_node_t** head_node, uint16_t key, uint16_t size)
{
    struct skip_list_node_t* cur = *head_node, * node = NULL;

    node = nd_malloc(instance, 0x00, size);//这里需要修改，申请CODE的地址函数；
    if (node != NULL)
    {
        node->key = key;                    //这里需要修改 写CODE的函数
        node->next = NULL;                  //这里需要修改 写CODE的函数

        if (cur != NULL)
        {
            while (cur->next != NULL)         //若未到表尾，则移动p指向表尾
            {
                cur = cur->next;              //指向下一个节点
            }
            cur->next = node;                 //这里需要修改 写CODE的函数
        }
        else if(NULL != head_node)
        {
            *head_node = node;              //这里需要修改 写CODE的函数
        }

        return node;
    }
    return NULL;
}

struct skip_list_last_node_t* create_new_list(struct instance_t* instance, struct skip_list_node_t* head_node, struct skip_list_node_t** node, uint8_t level, uint16_t* key)
{
    uint16_t size = sizeof(struct skip_list_node_t);
    uint8_t i = 0x00;
    uint8_t level_size = 0x05;

    if (level > level_size)
    {
        return NULL;
    }

    for (i = level; i < level_size; i++)
    {
        if (i == level_size - 0x01)
        {
            size = sizeof(struct skip_list_last_node_t);
        }

        if ((i != 0x00) && (head_node != NULL))   //每次完成之后都需要和CODE的去绑定；这里需要增加一个CODE的写函数；
        {                                         
            head_node = add_new_node(instance, (struct skip_list_node_t**)&head_node->head_node, key[i], size);
        }
        else
        {
            head_node = add_new_node(instance, node, key[i], size);             
        }
    }
    return (struct skip_list_last_node_t*)head_node;

}

uint32_t eprom_size = 10 * 1024;//临时的
uint8_t eprom_buf[10 * 1024];
uint32_t nd_allocation_by_storage(struct instance_t* instance, enum device_type_t device_id, uint32_t size)
{
    uint32_t head_addr = (uint32_t)eprom_buf;
    uint32_t used = instance->r_ctx._used;
    uint32_t start = head_addr + used;
    uint32_t free = eprom_size - used;

    if (free > size)
    {
        instance->r_ctx._used[device_id] += size;
        return start;
    }

    printf("error--no eprom\n");
    return 0xFFFFFFFF;
}

uint8_t make_dlms_obj(uint16_t* buf, struct dlms_obj_t* obj)
{
    buf[0] = obj->class_idx;
    buf[1] = (obj->logic_name.a << 8) + obj->logic_name.b;
    buf[2] = (obj->logic_name.c << 8) + obj->logic_name.d;
    buf[3] = (obj->logic_name.e << 8) + obj->logic_name.f;
    buf[4] = obj->attribute_idx;

    return 0x01;
}

uint8_t skip_list_insert(struct instance_t* instance, struct app_info_list_t* p)
{
    const struct resource_t* resource = p->info->resources;
    struct skip_list_last_node_t *this_node = NULL, *cur_node = NULL;
    uint32_t app_env_num = p->info->resource_num;
    uint16_t key[5] = { 0x0000, 0x0000, 0x0000, 0x0000 };
    uint16_t i = 0, j = 0;
    uint8_t ret = 0;

    for (i = 0x00; i < app_env_num; i++)
    {
        this_node = NULL;

        make_dlms_obj(key, &resource[i].view.obj);
        ret = look_up_lists(instance, instance->r_ctx.list, NULL, &this_node, key);//先查找已有APP中是否已经存在了；

        if ((ret == 0x38) && (NULL != this_node))  //该资源已经存在，需要重新查找
        {
            ret = look_up_list(instance, p->env_list, &cur_node, key);
            cur_node = create_new_list(instance, (struct skip_list_node_t*)cur_node, &p->env_list, ret - 0x33, key);
            if (NULL == cur_node)
            {
                return 0x00;//返回创建失败
            }
            cur_node->data_addr = this_node->data_addr;
            cur_node->resources = this_node->resources;
        }
        else
        {
            this_node = create_new_list(instance, (struct skip_list_node_t*)this_node, &p->env_list, ret - 0x33, key);
            if (NULL == this_node)
            {
                return 0x00;
            }

            this_node->data_addr = nd_allocation_by_storage(instance, resource[i].device_id, resource[i].length);
            this_node->resources = &resource[i];
        }
    }
    return 0x01;
}

uint8_t app_info_list_insert(struct instance_t* instance, const struct app_info_t* info)
{
    struct app_info_list_t* p = NULL, * list = instance->r_ctx.list;
    uint16_t s_num = 0x00;

    p = nd_malloc(instance, 0x00, sizeof(struct app_info_list_t));//申请内存
    if (p == NULL)                          //若新建节点申请内存失败，则退出程序
    {
        return 0x00;
    }

    p->info = info;
    p->next = NULL;                              //将新建的节点置为表尾

    if (list == NULL)                            //若原链表为空表
    {
        instance->r_ctx.list = p;                //将新建节点置为头节点
    }
    else                                         //若原链表为非空，则将新建节点添加到表尾
    {
        while (list->next != NULL)               //若未到表尾，则移动pr直到pr指向表尾
        {
            list = list->next;                   //指向下一个节点
        }
        list->next = p;                          //让末节点的指针指向新建的节点
    }

    if (0x00 == skip_list_insert(instance, p))   //更新APP的环境变量区
    {
        return 0x00;
    }

    return 0x01;
}

int8_t look_up(struct instance_t* instance, struct dlms_obj_t* obj, struct resources_obj_t* pro)
{
    uint16_t key[5] = { 0x00, 0x00, 0x00, 0x00 };
    struct app_info_list_t* app_list = NULL; 
    struct skip_list_last_node_t *node = NULL;
    uint8_t ret = 0;
    uint16_t j = 0x00;

    make_dlms_obj(key, obj);
    ret = look_up_lists(instance, instance->r_ctx.list, &app_list, &node, key);
  
    if ((ret == 0x38) && (NULL != app_list))
    {
        pro->data_addr = node->data_addr;
        pro->resources = node->resources;

        return 0x01;
    }

    return 0x00;
}

extern int write_dev(struct instance_t* instance, uint32_t addr, uint8_t* buf, uint16_t buf_size);
extern int read_dev(struct instance_t* instance, uint32_t addr, uint8_t* buf, uint16_t buf_size);
int8_t nd_get(struct instance_t* instance, struct dlms_obj_t *obj, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t this_obj;
    int8_t ret = 0x00;

    memset((uint8_t*)&this_obj, 0x00, sizeof(struct resources_obj_t));

    if (0x01 == look_up(instance, obj, &this_obj))
    {
        return read_dev(instance, this_obj.data_addr, buf, this_obj.resources->length);
    }
    return ret;
}

int8_t nd_set(struct instance_t* instance, struct dlms_obj_t* obj, uint8_t* buf, uint16_t buf_size)
{
    struct resources_obj_t this_obj;
    int8_t ret = 0x00;

    memset((uint8_t*)&this_obj, 0x00, sizeof(struct resources_obj_t));

    if (0x01 == look_up(instance, obj, &this_obj))
    {
        return write_dev(instance, this_obj.data_addr, buf, this_obj.resources->length);
    }

    return ret;
}


