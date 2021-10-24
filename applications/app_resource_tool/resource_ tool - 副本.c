#include "kernel\nidavellir.h"
//#include "kernel\resource_tool.h"

//单一设备的块链表
struct block_device_children_t
{
    uint8_t name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    struct list_head* sibling;
};

//设备的链表(这里把RAM也看成一个块设备来处理)
struct block_device_t
{
    uint8_t device_name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t page_size;
    uint32_t used;
    struct driver_t* driver;
    struct list_head* children;
    struct list_head* sibling;
};

#define EXTENSION_DATA_ATTR   
struct app_data_info_t
{
    uint32_t data_name;
    uint8_t device_name;
    uint8_t block_name;
    uint8_t proces_method;
    uint8_t check_mark;
    uint16_t data_size;
    EXTENSION_DATA_ATTR
};



struct skip_last_node_t
{
    uint8_t key;
    struct skip_last_node_t* silbing;
    struct app_data_info_t info;
};

struct skip_mid_node_t
{
    uint8_t key;
    struct skip_mid_node_t* silbing;
    struct skip_last_node_t* children;
};


struct skip_list_node_t
{
    uint8_t key;
    struct skip_list_node_t* silbing;
    struct skip_list_node_t* children;
};

//应用APP的数据列表
struct app_list_t
{
    uint32_t app_name;
    uint32_t env_array;
    uint32_t env_size;
    uint32_t env_used;
    struct skip_list_node_t* node;
    struct list_head* sibling;
};

struct resource_ctx_t
{
    struct list_head* app_list;
    struct list_head* dev_list;
    uint8_t ram_device_name;
    uint8_t resource_block_name;
};

void *__nd_malloc(uint32_t addr,uint32_t *used, uint32_t size, uint32_t block_size)
{
    uint32_t free = block_size - *used;

    if (free >= size)
    {
        *used += size;
        memset((uint8_t*)(addr + *used), 0x00, size);
        return (addr + *used);
    }

    return NULL;
}


struct block_device_t* look_up_device(struct resource_ctx_t* ctx, uint8_t device_name)
{
    struct block_device_t* device = NULL;
    struct list_head* p;

    list_for_each(p, ctx->dev_list)
    {
        device = list_entry(p, struct block_device_t, sibling);
        if (device->device_name == device_name)
        {
            return device;  //device 找到了
        }
    }

    return NULL;
}

struct block_device_t* look_up_block(struct resource_ctx_t* ctx, struct block_device_t* device, uint8_t block_name)
{
    struct block_device_children_t* block = NULL;
    struct list_head* p;

    list_for_each(p, device->children)
    {
        block = list_entry(p, struct block_device_children_t, sibling);
        if (block->name == block_name)
        {
            return block;  //block 找到了
        }
    }

    return NULL;
}

void* nd_malloc(struct resource_ctx_t *ctx, uint8_t block_name, uint32_t size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;

    device = look_up_device(ctx, ctx->ram_device_name);
    if (device == NULL)
    {
        return NULL;
    }

    block = look_up_block(ctx,device, block_name);
    if (block == NULL)
    {
        return NULL;
    }

    return __nd_malloc(block->first_addr, block->used, size, block->size);
}

//注册API
uint32_t *add_block(struct resource_ctx_t* ctx, uint8_t device_name, uint8_t block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(ctx,device_name);//先获取device
    if (device != NULL)
    {
        block = look_up_block(ctx, device, block_name);//在找block
        if (block == NULL)
        {
            resource_block = look_up_device_block(ctx, ctx->ram_device_name, ctx->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_children_t), resource_block->size);
            block->first_addr = device->first_addr + device->used;
            block->name = block_name;
            block->sibling = NULL;
            block->size = block_size;
            block->used = 0x00;
            device->used += block_size;
            list_add_tail(&block->sibling, device->children);
        }

        return NULL;
    }

    return block->first_addr;
}

uint32_t *add_inside_block(struct resource_ctx_t* ctx, uint8_t device_name,uint8_t block_name, uint8_t inside_block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* inside_block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(ctx, device_name);//先获取device
    if (device != NULL)
    {
        block = look_up_block(ctx, device, block_name);//在找block
        if (block != NULL)
        {
            resource_block = look_up_device_block(ctx, ctx->ram_device_name, ctx->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            inside_block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_children_t), resource_block->size);
            inside_block->first_addr = block->first_addr + block->used;
            inside_block->name = inside_block_name;
            inside_block->sibling = NULL;
            inside_block->size = block_size;
            inside_block->used = 0x00;
            block->used += block_size;
            list_add_tail(&inside_block->sibling, device->children);
        }

        return inside_block->first_addr;
    }

    return NULL;
}

struct resource_ctx_t *init_resource_manager(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size,uint8_t resource_block_name)
{
    struct resource_ctx_t* ctx = (struct resource_ctx_t*)first_addr;
    struct block_device_t* device = NULL; //创建RAM设备
    struct block_device_children_t* block = NULL;
    uint32_t used = 0;

    //创建RAM设备
    device = __nd_malloc(first_addr,&used,sizeof(struct block_device_t), size);
    if (device != NULL)
    {
        device->used = resource_blok_size;
        device->driver = NULL;
        device->device_name = device_name;
        device->first_addr = first_addr;
        device->page_size = 0;
        device->size = size;
        device->sibling = NULL;
        ctx->ram_device_name = device_name;
        list_add_tail(&device->sibling, ctx->dev_list);
        //创建resource_block
        block = __nd_malloc(first_addr, &used, sizeof(struct block_device_children_t), resource_blok_size);
        if (block != NULL)
        {
            block->first_addr = first_addr;
            block->name = resource_block_name;
            block->sibling = NULL;
            block->size = resource_blok_size;
            block->used = used;
            ctx->resource_block_name = resource_block_name;
            list_add_tail(&block->sibling, device->children);

            return ctx;
        }
    }

    return NULL;
}

//注册设备的API 先创建RAM设备；
int8_t add_device(struct resource_ctx_t* ctx, uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t page_size, struct driver_t* driver, uint8_t ram_name)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;

    device = look_up_device(ctx, device_name);//在分配其他的
    if (device == NULL)
    {
        block = look_up_device_block(ctx, ctx->ram_device_name, ctx->resource_block_name);
        if (block == NULL)
        {
            return -1;
        }

        device = __nd_malloc(block->first_addr,&block->used,sizeof(struct block_device_t),block->size);
        if (device == NULL)
        {
            return -1;
        }

        device->used = 0;
        device->driver = driver;
        device->device_name = device_name;
        device->first_addr = first_addr;
        device->page_size = page_size;
        device->size = size;
        device->sibling = NULL;
        device->children = NULL;
        list_add_tail(&device->sibling, ctx->dev_list);
    }
    
    return 0;
}



struct app_list_t* look_up_app(struct resource_ctx_t* ctx, uint8_t app_name)
{
    struct app_list_t* app = NULL;
    struct list_head* p;

    list_for_each(p, ctx->app_list)
    {
        app = list_entry(p, struct app_list_t, sibling);
        if (app->app_name == app_name)
        {
            return app;  //device 找到了
        }
    }

    return NULL;
}

int8_t add_app(struct resource_ctx_t* ctx, uint32_t app_name, uint32_t env_addr, uint32_t env_size)
{
    struct app_list_t* app = NULL;
    struct block_device_children_t* block = NULL;

    app = look_up_app(ctx,app_name);
    if (app == NULL)
    {
        block = look_up_device_block(ctx, ctx->ram_device_name, ctx->resource_block_name);
        if (block == NULL)
        {
            return -1;
        }

        app = __nd_malloc(block->first_addr, &block->used, sizeof(struct block_device_t), block->size);
        if (app == NULL)
        {
            return -1;
        }
        app->app_name = app_name;
        app->env_array = env_addr;
        app->env_size = env_size;
        app->env_used = 0x00;
        app->node = NULL;
        app->sibling = NULL;
        list_add_tail(&app->sibling, ctx->app_list);
    }

    return 0;
}

//应用注册单个数据的API
int8_t add_app_data(struct resource_ctx_t* ctx, uint32_t app_name, struct app_data_info_t *info)
{
    struct app_list_t* app = NULL;
    struct block_device_children_t* resource_block = NULL;
    //先找APP是否存在
    app = look_up_app(ctx, app_name);
    if (app == NULL)
    {
        return -1;
    }

    if (0x00 == skip_list_insert(ctx, app, info))   
    {
        return 0x00;
    }
}


uint8_t look_up_list(struct resource_ctx_t* ctx, struct skip_list_node_t* head_node, struct skip_last_node_t** last_node, uint16_t* key)
{
    uint8_t i = 0, level_size = 0x05, ret = 0x33;

    for (i = 0; i < level_size; i++)                                //遍历层级
    {
        if (head_node != NULL)
        {
            while ((head_node->key != key[i]) && (head_node->silbing != NULL))
            {
                head_node = head_node->silbing;                    //指向下一个节点
            }

            if (head_node->key == key[i])                       //找到了
            {
                ret = 0x34 + i;
                if (NULL != last_node)
                {
                    *last_node = (struct skip_last_node_t*)head_node;
                }                

                if (i == (level_size - 0x01))
                {
                    break;
                }

                head_node = (struct skip_list_node_t*)head_node->children;
                continue;
            }
        }
        break;
    }

    return ret;
}

uint8_t look_up_lists(struct resource_ctx_t* ctx, struct list_head* app_list, struct app_info_list_t** list, struct skip_list_last_node_t** node, uint8_t* key)
{
    struct app_list_t* app = NULL;
    struct list_head* p;
    uint8_t ret = 0x33;

    list_for_each(p, ctx->app_list)
    {
        app = list_entry(p, struct app_list_t, sibling);
        ret = look_up_list(ctx, app->node, node, key);
        if (0x37 == ret)
        {
            if (NULL != list)
            {
                *list = app_list;
            }
            break;
        }
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

struct skip_list_last_node_t* create_new_list(struct resource_ctx_t* ctx, struct skip_list_node_t* head_node, struct skip_list_node_t** node, uint8_t level, uint16_t* key)
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

uint8_t make_key(uint8_t* buf, uint32_t oad)
{
    uint8_t i;

    for (i = 0x00; i < 0x04; i++)
    {
        buf[i] = (uint8_t)(oad >> ((0x03 - i) * 0x08));
    }

    return 0x01;
}

uint8_t skip_list_insert(struct resource_ctx_t* ctx, struct app_list_t* app, struct app_data_info_t* info)
{
    struct skip_list_last_node_t* this_node = NULL, * cur_node = NULL;
    uint8_t key[4] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t ret = 0;

    make_key(key, info->data_name);
    ret = look_up_lists(ctx, ctx->app_list,NULL, &this_node, key);//先查找已有APP中是否已经存在了；

    if ((ret == 0x38) && (NULL != this_node))  //该资源已经存在，需要重新查找
    {
        ret = look_up_list(ctx, p->env_list, &cur_node, key);
        cur_node = create_new_list(ctx, (struct skip_list_node_t*)cur_node, &p->env_list, ret - 0x33, key);
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

uint8_t skip_list_insert(struct resource_ctx_t* ctx, struct app_list_t* app, struct app_data_info_t* info)
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

        make_key(key,info->data_name);
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


