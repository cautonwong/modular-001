#include "nidavellir.h"
//#include "resource_tool.h"

//单一设备的块链表
struct block_device_children_t
{
    uint8_t name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    struct list_head sibling;
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
    struct list_head children;
    struct list_head sibling;
};


struct resources_obj_t
{
    uint32_t offset;                        //地址
    uint16_t length;                        //数据存储长度
    uint8_t device_name;                    //设备标识；
    uint8_t block_name;                     //块名称
    uint8_t(*hook) (uint8_t* buf,void *p);  //resource的钩子函数
    void* p;
};

void *__nd_malloc(uint32_t addr,uint32_t *used, uint32_t size, uint32_t block_size)
{
    uint32_t free = block_size - *used;
    uint32_t start = addr + *used;

    if (free >= size)
    {
        *used += size;
        memset((uint8_t*)start, 0x00, size);
        return start;
    }

    return NULL;
}


struct block_device_t* look_up_device(struct resource_ctx_t* ctx, uint8_t device_name)
{
    struct block_device_t* device = NULL;
    struct list_head* p;

    list_for_each(p, &ctx->dev_list)
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

    if (device != NULL)
    {
        list_for_each(p, &device->children)
        {
            block = list_entry(p, struct block_device_children_t, sibling);
            if (block->name == block_name)
            {
                return block;  //block 找到了
            }
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

    return __nd_malloc(block->first_addr, &block->used, size, block->size);
}

//注册API
uint32_t *add_block(struct resource_ctx_t* ctx, uint8_t device_name, uint8_t block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(ctx,device_name);//先获取自己的device
    if (device != NULL)
    {
        block = look_up_block(ctx, device, block_name);//在找block

        if (block == NULL)
        {
            resource_block = look_up_block(ctx, look_up_device(ctx, ctx->ram_device_name), ctx->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_children_t), resource_block->size);
            if (block == NULL)
            {
                return NULL;
            }
            block->first_addr = device->first_addr + device->used;
            block->name = block_name;
            INIT_LIST_HEAD(&block->sibling);
            //memset((uint8_t*)&block->sibling, 0x00, sizeof(struct list_head));
            block->size = block_size;
            block->used = 0x00;
            device->used += block_size;
            list_add_tail(&block->sibling, &device->children);           
        }
        return block->first_addr;       
    }
    return NULL;
}

uint32_t *add_inside_block(struct resource_ctx_t* ctx, uint8_t device_name,uint8_t block_name, uint8_t inside_block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* inside_block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(ctx, device_name);//先获取device
    if (device != NULL)
    {
        block = look_up_block(ctx, device, block_name);//在找block
        ram_device = look_up_device(ctx, ctx->ram_device_name); //获取ram_device

        if ((block != NULL) && (ram_device != NULL))
        {
            resource_block = look_up_block(ctx, ram_device, ctx->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            inside_block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_children_t), resource_block->size);
            if (inside_block != NULL)
            {
                inside_block->first_addr = block->first_addr + block->used;
                inside_block->name = inside_block_name;
                INIT_LIST_HEAD(&inside_block->sibling);
                //memset((uint8_t*)&inside_block->sibling, 0x00, sizeof(struct list_head));
                inside_block->size = block_size;
                inside_block->used = 0x00;
                block->used += block_size;
                list_add_tail(&inside_block->sibling, &device->children);
                return inside_block->first_addr;
            }
        }
    }
    return NULL;
}

struct resource_ctx_t *init_resource_manager(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size,uint8_t resource_block_name)
{
    struct resource_ctx_t* ctx = (struct resource_ctx_t*)first_addr;
    struct block_device_t* device = NULL; //创建RAM设备
    struct block_device_children_t* block = NULL;
    uint32_t used = sizeof(struct resource_ctx_t);

    INIT_LIST_HEAD(&ctx->app_list);
    INIT_LIST_HEAD(&ctx->dev_list);
    INIT_LIST_HEAD(&ctx->map_list);
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
        INIT_LIST_HEAD(&device->sibling);
        INIT_LIST_HEAD(&device->children);
        ctx->ram_device_name = device_name;
        list_add_tail(&device->sibling, &ctx->dev_list);
        //创建resource_block
        block = __nd_malloc(first_addr, &used, sizeof(struct block_device_children_t), resource_blok_size);
        if (block != NULL)
        {
            block->first_addr = first_addr;
            block->name = resource_block_name;
            INIT_LIST_HEAD(&block->sibling);
            block->size = resource_blok_size;
            block->used = used;
            ctx->resource_block_name = resource_block_name;
            list_add_tail(&block->sibling, &device->children);

            return ctx;
        }
    }

    return NULL;
}

//注册设备的API 先创建RAM设备；
int8_t add_device(struct resource_ctx_t* ctx, uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t page_size, struct driver_t* driver, uint8_t ram_name)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct block_device_children_t* block = NULL;

    device = look_up_device(ctx, device_name);//在分配其他的
    ram_device = look_up_device(ctx, ctx->ram_device_name); //获取ram_device

    if ((device == NULL) && (ram_device != NULL))
    {
        block = look_up_block(ctx, ram_device, ctx->resource_block_name);
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
        INIT_LIST_HEAD(&device->sibling);
        INIT_LIST_HEAD(&device->children);
        //memset((uint8_t*)&device->sibling, 0x00, sizeof(struct list_head));
        //memset((uint8_t*)&device->children, 0x00, sizeof(struct list_head));
        list_add_tail(&device->sibling, &ctx->dev_list);
    }
    
    return 0;
}

int8_t resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj)
{
    struct mbase_control_block_t* mbase_cb = instance->mbase_cb;
    switch (id)
    {
    case 0x20000200:
    {
        obj->block_name = 0x0A;//只是举例
        obj->device_name = 0x0D;//只是举例
        //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
        switch (mode)
        {
        case 0x01://读数据
            //1、直接读缓存的值
         //   memcpy(buf, (uint8_t*)mbase_cb->UA, 0x04);
            return 0x01;
            //2、需要读EPROM
            return 0x02;
            break;
        case 0x02://写数据
            break;
        deault:
            break;
        }
        break;
    }
    default:
        break;
    }
    return 0x01;
}

struct resource_map_t* register_map(struct resource_ctx_t* ctx,uint8_t map_name,
    int8_t (*fun)(struct instance_t *instance, uint32_t id,uint8_t mode, struct resources_obj_t* obj))
{
    struct block_device_children_t* block = NULL;
    struct resource_map_t* map = NULL;
    struct list_head* p;

    list_for_each(p, &ctx->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        if (map->map_name == map_name)
        {
            return map;  //device 找到了
        }
    }

    block = look_up_block(ctx, look_up_device(ctx, ctx->ram_device_name), ctx->resource_block_name);
    if (block == NULL)
    {
        return NULL;
    }
    map = __nd_malloc(block->first_addr, &block->used, sizeof(struct block_device_t), block->size);
    if (map == NULL)
    {
        return NULL;
    }
    map->map_name = map_name;
    map->fun = fun;
    list_add_tail(&map->sibling, &ctx->map_list);

    return map;
}

int8_t look_up_map(struct instance_t* instance,uint32_t id, uint8_t *buf, uint8_t mode, struct resources_obj_t* obj)
{
    struct resource_map_t* map = NULL;
    struct resource_ctx_t* ctx = instance->ctx;
    struct list_head* p;
    uint8_t ret = 0x00;

    list_for_each(p, &ctx->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        if (map->fun != NULL)
        {
            ret = map->fun(instance, id, buf,mode, obj);
            if (0 < ret)
            {
                return ret;
            }
        }
    }

    return -1;
}


int8_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_ctx_t* ctx = instance->ctx;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;
    
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x01, &obj);

    if (ret <= 0x01)
    {
        return ret;
    }
    //查找设备
    device = look_up_device(ctx, obj.device_name);
    if (device != NULL)
    {
        if ((device->driver != NULL) && (device->driver->read_dev != NULL))
        {
            device->driver->read_dev(instance, obj.offset, buf, obj.length);
            if (obj.hook != NULL)
            {
                obj.hook(buf, obj.p);
            }

            return 0x01;
        }
    }
    return -1;
}

int8_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_ctx_t* ctx = instance->ctx;
    struct block_device_children_t* block = NULL;

    //查找设备
    device = look_up_device(ctx, device_name);
    if ((device != NULL) && (device->driver != NULL) && (device->driver->read_dev != NULL))
    {
        block = look_up_block(ctx, device, block_name);
        if ((block != NULL) && (buf_size <= block->size))
        {
            return device->driver->read_dev(instance, block->first_addr, buf, block->size);
        }
    }

    return -1;
}

int8_t nd_set_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf, uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_ctx_t* ctx = instance->ctx;
    struct block_device_children_t* block = NULL;

    //查找设备
    device = look_up_device(ctx, device_name);
    if ((device != NULL) && (device->driver != NULL) && (device->driver->read_dev != NULL))
    {
        block = look_up_block(ctx, device, block_name);
        if ((block != NULL) && (buf_size <= block->size))
        {
            return device->driver->write_dev(instance, block->first_addr, buf, block->size);
        }
    }

    return -1;
}

int8_t nd_set(struct instance_t* instance, struct dlms_obj_t* obj, uint8_t* buf, uint16_t buf_size)
{
    return -1;
}


