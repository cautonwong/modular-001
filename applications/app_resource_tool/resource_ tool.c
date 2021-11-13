#include "nidavellir.h"

//内存管理的对齐
#define RES_ALIGN_TYPE  uint32_t
//单一设备的块链表
#pragma pack(4)
struct block_device_children_t
{
    uint8_t name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    struct list_head sibling;
};

struct block_device_record_t
{
    uint8_t name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    int16_t(*hook)(uint16_t id, uint16_t* len, uint16_t* depth);
    struct list_head sibling;
};

//设备的链表(这里把RAM也看成一个块设备来处理)
struct dev_info_t;
#pragma pack(4)
struct block_device_t
{
    uint8_t device_name;
    uint32_t first_addr;
    uint32_t size;
    uint32_t page_size;
    uint32_t used;
    const struct dev_info_t *dev;
    struct list_head children;
    struct list_head sibling;
};

struct log_manager_t
{
    uint16_t num;
    uint16_t wr_ptr;
    uint16_t unit_size;
    uint16_t max_num;
    uint16_t check_sum;
};

struct circular_info_t
{
    uint16_t unit_size;                       //单体记录大小
    uint16_t record_period;                   //记录周期
    uint32_t start_mark;                      //搜索时的起始标志
    uint32_t end_mark;                        //搜索时的结束标志
    uint16_t read_ptr;                        //读的位置
    uint16_t valid_num;                       //符合的条数
};

void *__nd_malloc(uint32_t addr,uint32_t *used, uint32_t size, uint32_t block_size)
{
    uint32_t free = block_size - *used;
    uint32_t start = addr + *used;

    size = (((size + (sizeof(RES_ALIGN_TYPE))) - ((RES_ALIGN_TYPE)1)) / (sizeof(RES_ALIGN_TYPE))) * (sizeof(RES_ALIGN_TYPE));
    if (free >= size)
    {
        *used += size;
        memset((uint8_t*)start, 0x00, size);
        return (void *)start;
    }

    return NULL;
}


struct block_device_t* look_up_device(struct instance_t* instance, uint8_t device_name)
{
    struct block_device_t* device = NULL;
    struct list_head* p;

    list_for_each(p, &instance->dev_list)
    {
        device = list_entry(p, struct block_device_t, sibling);
        if (device->device_name == device_name)
        {
            return device;  //device 找到了
        }
    }

    return NULL;
}

struct block_device_children_t* look_up_block(struct instance_t* instance, struct block_device_t* device, uint8_t block_name)
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

void* nd_malloc(struct instance_t *instance, uint8_t block_name, uint32_t size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;

    device = look_up_device(instance, instance->ram_device_name);
    if (device == NULL)
    {
        return NULL;
    }

    block = look_up_block(instance, device, block_name);
    if (block == NULL)
    {
        return NULL;
    }

    return __nd_malloc(block->first_addr, &block->used, size, block->size);
}

//注册API
uint32_t *add_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(instance,device_name);//先获取自己的device
    if (device != NULL)
    {
        block = look_up_block(instance, device, block_name);//在找block

        if (block == NULL)
        {
            resource_block = look_up_block(instance, look_up_device(instance, instance->ram_device_name), instance->resource_block_name);
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
            init_list_head(&block->sibling);
            block->size = block_size;
            block->used = 0x00;
            device->used += block_size;
            list_add_tail(&block->sibling, &device->children);           
        }
        return (uint32_t *)block->first_addr;       
    }
    return NULL;
}

uint32_t *add_inside_block(struct instance_t* instance, uint8_t device_name,uint8_t block_name, uint8_t inside_block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct block_device_children_t* block = NULL;
    struct block_device_children_t* inside_block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(instance, device_name);//先获取device
    if (device != NULL)
    {
        block = look_up_block(instance, device, block_name);//在找block
        ram_device = look_up_device(instance, instance->ram_device_name); //获取ram_device

        if ((block != NULL) && (ram_device != NULL))
        {
            resource_block = look_up_block(instance, ram_device, instance->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            inside_block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_children_t), resource_block->size);
            if (inside_block != NULL)
            {
                inside_block->first_addr = block->first_addr + block->used;
                inside_block->name = inside_block_name;
                init_list_head(&inside_block->sibling);
                inside_block->size = block_size;
                inside_block->used = 0x00;
                block->used += block_size;
                list_add_tail(&inside_block->sibling, &device->children);
                return (uint32_t *)inside_block->first_addr;
            }
        }
    }
    return NULL;
}

struct instance_t* create_instance(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size,uint8_t resource_block_name)
{
    struct instance_t* instance = NULL;
    struct block_device_t* device = NULL; //创建RAM设备
    struct block_device_children_t* block = NULL;
    uint32_t used = 0x00;

    instance = __nd_malloc(first_addr,&used,sizeof(struct instance_t), size);
    //初始化链表头
    init_list_head(&instance->app_list);
    init_list_head(&instance->dev_list);
    init_list_head(&instance->map_list);
    //创建RAM设备
    device = __nd_malloc(first_addr,&used,sizeof(struct block_device_t), size);
    if (device != NULL)
    {
        device->used = resource_blok_size;
        device->dev = NULL;
        device->device_name = device_name;
        device->first_addr = first_addr;
        device->page_size = 0;
        device->size = size;
        init_list_head(&device->sibling);
        init_list_head(&device->children);
        instance->ram_device_name = device_name;
        list_add_tail(&device->sibling, &instance->dev_list);
        //创建resource_block
        block = __nd_malloc(first_addr, &used, sizeof(struct block_device_children_t), resource_blok_size);
        if (block != NULL)
        {
            block->first_addr = first_addr;
            block->name = resource_block_name;
            init_list_head(&block->sibling);
            block->size = resource_blok_size;
            block->used = used;
            instance->resource_block_name = resource_block_name;
            list_add_tail(&block->sibling, &device->children);

            return instance;
        }
    }

    return NULL;
}

//注册设备的API 先创建RAM设备；
int8_t add_device(struct instance_t* instance, uint8_t device_name, const struct dev_info_t *dev)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct block_device_children_t* block = NULL;

    device = look_up_device(instance, device_name);//在分配其他的
    ram_device = look_up_device(instance, instance->ram_device_name); //获取ram_device

    if ((device == NULL) && (ram_device != NULL))
    {
        block = look_up_block(instance, ram_device, instance->resource_block_name);
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
        device->dev = dev;
        device->device_name = device_name;
        device->first_addr = 0x00000000;
        device->page_size = 0x00;
        device->size = dev->data_size;
        init_list_head(&device->sibling);
        init_list_head(&device->children);
        list_add_tail(&device->sibling, &instance->dev_list);
    }
    
    return 0;
}

struct resource_map_t* register_map(struct instance_t* instance,uint8_t map_name,
    int8_t (*fun)(struct instance_t *instance, uint32_t id,uint8_t *buf, uint8_t mode, struct resources_obj_t* obj))
{
    struct block_device_children_t* block = NULL;
    struct resource_map_t* map = NULL;
    struct list_head* p;

    list_for_each(p, &instance->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        if (map->map_name == map_name)
        {
            return map;  //device 找到了
        }
    }

    block = look_up_block(instance, look_up_device(instance, instance->ram_device_name), instance->resource_block_name);
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
    list_add_tail(&map->sibling, &instance->map_list);

    return map;
}

int8_t look_up_map(struct instance_t* instance,uint32_t id, uint8_t *buf, uint8_t mode, struct resources_obj_t* obj)
{
    struct resource_map_t* map = NULL;
    struct list_head* p;
    uint8_t ret = 0x00;

    list_for_each(p, &instance->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        if (map->fun != NULL)
        {
            ret = map->fun(instance, id, buf, mode, obj);
            if (0 < ret)
            {
                return ret;
            }
        }
    }

    return -1;
}


int16_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    const struct block_device_t* device = NULL;
    const struct driver_ops_t *driver = NULL;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;
    
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x01, &obj);

    if (ret >= 0x01)
    {
        return ret;
    }
    //查找设备
    device = look_up_device(instance, obj.device_name);
    driver = device->dev->dev_driver_ops;
    if (device != NULL)
    {
        if ((driver != NULL) && (driver->dev_read != NULL))
        {
            driver->dev_read(device->dev, obj.offset, obj.length, buf);
            if (obj.hook != NULL)
            {
                obj.hook(buf, obj.p);
            }

            return obj.length;
        }
    }
    return -1;
}


int16_t nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    const struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x01, &obj);

    if (ret >= 0x01)
    {
        return ret;
    }
    //查找设备
    device = look_up_device(instance, obj.device_name);
    driver = device->dev->dev_driver_ops;
    if (device != NULL)
    {
        if ((driver != NULL) && (driver->dev_write != NULL))
        {
            driver->dev_write(device->dev, obj.offset, obj.length, buf);
            if (obj.hook != NULL)
            {
                obj.hook(buf, obj.p);
            }

            return 0x01;
        }
    }

    return -1;
}


int16_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    const struct driver_ops_t* driver = NULL;

    //查找设备
    device = look_up_device(instance, device_name);
    driver = device->dev->dev_driver_ops;
    if ((driver != NULL) && (driver->dev_read != NULL))
    {
        block = look_up_block(instance, device, block_name);
        if ((block != NULL) && (buf_size <= block->size))
        {
            return driver->dev_read(device->dev, block->first_addr, block->size, buf);
        }
    }

    return -1;
}

int16_t nd_set_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf, uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct block_device_children_t* block = NULL;
    const struct driver_ops_t* driver = NULL;

    //查找设备
    device = look_up_device(instance, device_name);
    driver = device->dev->dev_driver_ops;
    if ((driver != NULL) && (driver->dev_write != NULL))
    {
        block = look_up_block(instance, device, block_name);
        if ((block != NULL) && (buf_size <= block->size))
        {
            return driver->dev_write(device->dev, block->first_addr, block->size, buf);
        }
    }

    return -1;
}

uint32_t* register_event(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(uint16_t event_flag, uint16_t* size, uint16_t* depth))
{
    struct block_device_t* device = NULL;
    struct block_device_record_t* block = NULL;
    struct block_device_children_t* resource_block = NULL;

    device = look_up_device(instance, device_name);//先获取自己的device
    if (device != NULL)
    {
        block = (struct block_device_record_t*)look_up_block(instance, device, block_name);//在找block

        if (block == NULL)
        {
            resource_block = look_up_block(instance, look_up_device(instance, instance->ram_device_name), instance->resource_block_name);
            if (resource_block == NULL)
            {
                return NULL;
            }

            block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct block_device_record_t), resource_block->size);
            if (block == NULL)
            {
                return NULL;
            }
            block->first_addr = device->first_addr + device->used;
            block->name = block_name;
            init_list_head(&block->sibling);
            block->size = block_size;
            block->used = 0x00;
            block->hook = fun;
            device->used += block_size;
            list_add_tail(&block->sibling, &device->children);
        }
        return (uint32_t*)block->first_addr;
    }
    return NULL;
}

int16_t nd_size(struct instance_t* instance, uint32_t id)
{
    struct resources_obj_t obj;
    uint8_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, NULL, 0x01, &obj);
    if (ret >= 0x01)
    {
        return obj.length;
    }

    return -1;
}

#if 1
uint8_t circular_image_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, struct circular_info_t* info)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct log_manager_t manager;
    struct resources_obj_t obj;
    uint8_t ret = 0x00;
    uint32_t addr = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x01, &obj);

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device = look_up_device(instance, obj.device_name);
    driver = device->dev->dev_driver_ops;

    if (driver->dev_read != NULL)//先读管理区
    {
        if (true != driver->dev_read(device->dev, obj.offset, sizeof(struct log_manager_t), (uint8_t*)&manager))
        {
            return false;
        }

        if (0xA5A5 + manager.num + manager.unit_size + manager.wr_ptr != manager.check_sum)
        {
            manager.max_num = obj.max_num;
            manager.unit_size = obj.unit_size;
            manager.num = 0;
            manager.wr_ptr = 0;
        }
    }

    addr = obj.offset + manager.wr_ptr * manager.unit_size + sizeof(struct log_manager_t);
    if (NULL != driver->dev_write)
    {
        ret = driver->dev_write(device->dev, addr, manager.unit_size, buf);
    }

    manager.wr_ptr++;
    manager.num++;
    if (manager.wr_ptr >= manager.max_num)
    {
        manager.wr_ptr = 0x00;
    }
    if (manager.num >= manager.max_num)
    {
        manager.num = manager.max_num;
    }
    manager.check_sum = 0xA5A5 + manager.num + manager.unit_size + manager.wr_ptr;
    if (NULL != driver->dev_write)
    {
        ret = driver->dev_write(device->dev, obj.offset, sizeof(struct log_manager_t), (uint8_t*)&manager);
    }

    return ret;
}

uint8_t circular_image_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, struct circular_info_t* info)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct log_manager_t manager;
    struct resources_obj_t obj;
    uint32_t addr = 0x00;
    uint8_t ret = true;

    if (info != NULL)
    {
        memset((uint8_t*)&obj, 0x00, sizeof(obj));
        ret = look_up_map(instance, id, buf, 0x01, &obj);

        memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
        device = look_up_device(instance, obj.device_name);
        driver = device->dev->dev_driver_ops;

        if (info->valid_num == 0x00)  //第一次读
        {
            memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
            if (driver->dev_read != NULL)//先读管理区
            {
                if (true != driver->dev_read(device->dev, obj.offset, sizeof(struct log_manager_t), (uint8_t*)&manager))
                {
                    return false;
                }

                if (0xA5A5 + manager.num + manager.unit_size + manager.wr_ptr != manager.check_sum)
                {
                    return false;
                }
            }

            info->valid_num = manager.num;
            if (manager.num == manager.max_num)
            {
                info->read_ptr = manager.wr_ptr;
            }
            else
            {
                info->read_ptr = 0x00;
            }
        }

        addr = obj.offset + info->read_ptr * manager.unit_size + sizeof(struct log_manager_t);
        if (info->read_ptr < info->valid_num)
        {
            info->read_ptr++;
        }

        if (NULL != driver->dev_read)
        {
            ret = driver->dev_read(device->dev, addr, manager.unit_size, buf);
        }
    }

    return ret;
}
#endif




