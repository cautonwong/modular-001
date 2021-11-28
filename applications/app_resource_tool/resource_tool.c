#include "nidavellir.h"
#include "resource_tool.h"

struct search_tempinfo_t
{
    const struct dev_info_t* dev;
    uint32_t addr;
    uint16_t offset;
    uint16_t unit_size;
    uint8_t mode;
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

struct resource_block_t* look_up_block(struct instance_t* instance, struct block_device_t* device, uint8_t block_name)
{
    struct resource_block_t* block = NULL;
    struct list_head* p;

    if (device != NULL)
    {
        list_for_each(p, &device->children)
        {
            block = list_entry(p, struct resource_block_t, sibling);
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
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, instance->ram_device);
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
    struct resource_block_t* block = NULL;
    struct resource_block_t* re_block = NULL;

    device = look_up_device(instance,device_name);//先获取自己的device
    if (device != NULL)
    {
        //判断申请的字节有没有超限
        if ((device->used + block_size) > device->size)
        {
            return NULL;
        }

        block = look_up_block(instance, device, block_name);//在找block
        if (block == NULL)
        {
            re_block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
            if (re_block == NULL)
            {
                return NULL;
            }

            block = __nd_malloc(re_block->first_addr, &re_block->used, sizeof(struct resource_block_t), re_block->size);
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
    struct resource_block_t* block = NULL;
    struct resource_block_t* inside_block = NULL;
    struct resource_block_t* resource_block = NULL;

    device = look_up_device(instance, device_name);//先获取device
    if (device != NULL)
    {
        //判断申请的字节有没有超限
        if ((device->used + block_size) > device->size)
        {
            return NULL;
        }
        block = look_up_block(instance, device, block_name);//在找block
        ram_device = look_up_device(instance, instance->ram_device); //获取ram_device

        if ((block != NULL) && (ram_device != NULL))
        {
            resource_block = look_up_block(instance, ram_device, instance->resource_block);
            if (resource_block == NULL)
            {
                return NULL;
            }

            inside_block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct resource_block_t), resource_block->size);
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
    struct resource_block_t* block = NULL;
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
        instance->ram_device = device_name;
        list_add_tail(&device->sibling, &instance->dev_list);
        //创建resource_block
        block = __nd_malloc(first_addr, &used, sizeof(struct resource_block_t), resource_blok_size);
        if (block != NULL)
        {
            block->first_addr = first_addr;
            block->name = resource_block_name;
            init_list_head(&block->sibling);
            block->size = resource_blok_size;
            block->used = used;
            instance->resource_block = resource_block_name;
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
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, device_name);//在分配其他的
    ram_device = look_up_device(instance, instance->ram_device); //获取ram_device

    if ((device == NULL) && (ram_device != NULL))
    {
        block = look_up_block(instance, ram_device, instance->resource_block);
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
    
    return 1;
}

struct resource_map_t* register_map(struct instance_t* instance,uint8_t map_name,
    int (*fun)(struct instance_t *instance, uint32_t id,uint8_t *buf, uint8_t mode, struct resources_obj_t* obj))
{
    struct resource_block_t* block = NULL;
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

    block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
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

int16_t look_up_map(struct instance_t* instance,uint32_t id, uint8_t *buf, uint8_t mode, struct resources_obj_t* obj)
{
    struct resource_map_t* map = NULL;
    struct list_head* p;

    list_for_each(p, &instance->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        if (map->fun != NULL)
        {
            return map->fun(instance, id, buf, mode, obj);
        }
    }

    return -1;
}


int16_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t *driver = NULL;
    struct resource_block_t* block = NULL;
    struct resources_obj_t obj;
    int16_t ret = 0x00;
    uint32_t addr = 0x00;
    
    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x01, &obj);
    if (0x00 == ret)
    {        
        device = look_up_device(instance, obj.device_name);//查找设备
        driver = device->dev->dev_driver_ops;
        //查找块
        block = look_up_block(instance, device, obj.block_name);
        if ((device != NULL) && (block != NULL))
        {
            if ((driver != NULL) && (driver->dev_read != NULL))
            {
                //计算地址
                addr = block->first_addr + obj.offset;
                driver->dev_read(device->dev, addr, obj.length, buf);
                //这里没判断返回值和校验

                if (obj.hook != NULL)
                {
                    obj.hook(buf, obj.p);
                }

                return obj.length;
            }
        }
    }

    return ret;
}


int16_t nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    struct resources_obj_t obj;
    int16_t ret = 0x00;
    uint32_t addr = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, buf, 0x02, &obj);
    if (0x00 == ret)
    {
        device = look_up_device(instance, obj.device_name);//查找设备
        driver = device->dev->dev_driver_ops;
        //查找块
        block = look_up_block(instance, device, obj.block_name);
        if ((device != NULL) && (block != NULL))
        {
            if ((driver != NULL) && (driver->dev_write != NULL))
            {
                //计算地址
                addr = block->first_addr + obj.offset;
                driver->dev_write(device->dev, addr, obj.length, buf);
                //这里没判断返回值和校验

                if (obj.hook != NULL)
                {
                    obj.hook(buf, obj.p);
                }

                return 0x01;
            }
        }
    }

    return ret;
}

int16_t nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
  return -1;
}

int16_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
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
    struct resource_block_t* block = NULL;
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

uint32_t* __register(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(uint16_t id, uint32_t *addr, uint16_t* len, uint16_t* depth))
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
    struct resource_block_t* re_block = NULL;

    device = look_up_device(instance, device_name);//先获取自己的device
    if (device != NULL)
    {
        //判断申请的字节有没有超限
        if ((device->used + block_size) > device->size)
        {
            return NULL;
        }
        block = look_up_block(instance, device, block_name);//在找block
        if (block == NULL)
        {
            re_block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
            if (re_block == NULL)
            {
                return NULL;
            }

            block = __nd_malloc(re_block->first_addr, &re_block->used, sizeof(struct resource_block_t), re_block->size);
            if (block == NULL)
            {
                return NULL;
            }
            block->first_addr = device->first_addr + device->used;
            block->name = block_name;
            init_list_head(&block->sibling);
            block->size = block_size;
            block->used = 0x00;
            block->relation_device = relation_device;
            block->relation_name = relation_name;
            block->hook = fun;
            device->used += block_size;
            list_add_tail(&block->sibling, &device->children);
        }
        return (uint32_t*)block->first_addr;
    }
    return NULL;
}

uint32_t* register_log(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(uint16_t id, uint32_t* addr, uint16_t* len, uint16_t* depth))
{
    return __register(instance, device_name, block_name, relation_device, relation_name, block_size, fun);
}

uint32_t* register_parameter(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(uint16_t id, uint32_t *addr, uint16_t* len))
{
    return __register(instance, device_name, block_name, 0x00, 0x00, block_size, (int16_t(*)(uint16_t id, uint32_t * addr, uint16_t * len, uint16_t * depth))fun);
}

int16_t nd_size(struct instance_t* instance, uint32_t id)
{
    struct resources_obj_t obj;
    int16_t ret = 0x00;

    memset((uint8_t*)&obj, 0x00, sizeof(obj));
    ret = look_up_map(instance, id, NULL, 0x01, &obj);
    if (ret >= 0x01)
    {
        return obj.length;
    }

    return -1;
}

int16_t get_block_frist_addr(struct instance_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, device_name);//先获取自己的device
    if (device != NULL)
    {
        block = look_up_block(instance, device, block_name);//在找block
        if ((block != NULL) && (addr != NULL))
        {
            *addr = block->first_addr;
            return 1;
        }
    }
    
    return -1;
}

#if 1
uint32_t get_curr_mark(const struct dev_info_t* dev, uint32_t addr, uint8_t mode)
{
    uint32_t(*fun[2])(struct tm* t) = { rtc_mktime , (uint32_t(*)(struct tm* t))buf_to_uint32_lsb };
    const struct driver_ops_t* driver = NULL;
    uint8_t buf[10] = { 0 };

    driver = dev->dev_driver_ops;
    if ((driver != NULL) && (driver->dev_read))
    {
        driver->dev_read(dev, addr, 0x07, buf);
        if (mode <= 0x01) 
        {
            return fun[mode]((struct tm*)buf);
        }
    }
    return 0xffffffff;
}

uint16_t search_enum_way(uint32_t key, uint16_t start, uint16_t end, struct search_tempinfo_t *info, uint8_t direct)
{
    uint16_t ptr, next_ptr;
    uint32_t cmp, next_cmp;

    ptr = end;
    next_ptr = ((start + 1) > end) ? start : (end - 1);
    cmp = get_curr_mark(info->dev,info->addr + info->offset + ptr * info->unit_size, info->mode);
    while (next_ptr >= start)
    {
        next_cmp = get_curr_mark(info->dev, info->addr + info->offset + next_ptr * info->unit_size, info->mode);
        if (cmp == key)
        {
            return ptr;
        }
        else if ((key >= next_cmp) && (key < cmp))
        {
            if (key == next_cmp)
            {
                return next_ptr;
            }
            return (direct == 0x00) ? ptr : next_ptr;
        }
        else
        {
            ptr = next_ptr;
            cmp = next_cmp;

            if (next_ptr == 0x00)
            {
                break;
            }
            next_ptr--;
        }
    }

    return 0xFFFF;
}

uint16_t search_mid_way(uint32_t key, uint16_t start, uint16_t end, struct search_tempinfo_t* info, uint8_t direct)
{
    uint16_t min_ptr, max_ptr, mid_ptr = 0xFFFF;
    uint32_t mid_cmp, temp_cmp;

    min_ptr = start, max_ptr = end;
    while ((direct < 0x02) && (max_ptr >= min_ptr))
    {
        mid_ptr = (min_ptr + max_ptr) / 2;
        mid_cmp = get_curr_mark(info->dev, info->addr + info->offset + mid_ptr * info->unit_size, info->mode);
        if (mid_cmp == key)
        {
            break;
        }
        else if (key > mid_cmp)
        {
            if (mid_ptr == max_ptr)
            {
                mid_ptr = 0xFFFF; 
                break;
            }

            min_ptr = mid_ptr + 1;
            temp_cmp = get_curr_mark(info->dev, info->addr + info->offset + min_ptr * info->unit_size, info->mode);
            if (key == temp_cmp)
            {
                return min_ptr;
            }

            if (key <= temp_cmp)
            {
                return (min_ptr - direct);
            }
        }
        else
        {
            if (mid_ptr == min_ptr)
            {
                mid_ptr = 0xFFFF; 
                break;
            }

            max_ptr = mid_ptr - 0x01;
            temp_cmp = get_curr_mark(info->dev, info->addr + info->offset + max_ptr * info->unit_size, info->mode);
            if (key == temp_cmp)
            {
                return max_ptr;
            }

            if (key > temp_cmp)
            {
                return (mid_ptr - direct);
            }
        }
    }

    return mid_ptr;
}

int16_t __search_circular(struct circular_info_t* info, struct log_manager_t* manager, struct search_tempinfo_t *tempinfo,
    uint16_t (*fun)(uint32_t key, uint16_t start, uint16_t end, struct search_tempinfo_t* info, uint8_t direct))
{
    uint16_t ptr[4] = { 0x00,manager->wr_ptr,manager->wr_ptr,manager->max_num };
    uint16_t start_ptr = 0x00, end_ptr = 0x00;
    uint8_t cnt = 0x00;

    if (manager->num == manager->max_num)//说明已经反转了,反转了需要找2块区域
    {
        cnt = 0x02;  //需要找两次；
    }

    start_ptr = fun(info->start_mark, ptr[0], ptr[1], tempinfo, 0x00);
    if ((start_ptr == 0xFFFF) && (cnt == 0x02))
    {
        start_ptr = fun(info->start_mark, ptr[2], ptr[3], tempinfo, 0x00);
    }

    end_ptr = fun(info->end_mark, ptr[0], ptr[1], tempinfo, 0x00);
    if ((end_ptr == 0xFFFF) && (cnt == 0x02))
    {
        end_ptr = fun(info->end_mark, ptr[2], ptr[3], tempinfo, 0x00);
    }

    if ((start_ptr != 0xFFFF) && (end_ptr != 0xFFFF) && (end_ptr >= start_ptr))
    {
        info->read_ptr = start_ptr;
        info->valid_num = end_ptr - start_ptr + 0x01;
    }
    else
    {
        info->read_ptr = 0x00;
        info->valid_num = 0x00;
    }

    return 0x00;
}

int16_t search_circular(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, struct circular_info_t* info, struct log_manager_t* manager)
{
    struct search_tempinfo_t tempinfo;

    memset((uint8_t*)&tempinfo, 0x00, sizeof(struct search_tempinfo_t));
    tempinfo.dev = dev;
    tempinfo.addr = addr;
    tempinfo.unit_size = manager->unit_size;

    if ((info != NULL) && (dev != NULL) && (dev->dev_driver_ops != NULL))
    {
        switch (info->mode)
        {
        case 0x00:  //全读
            return 0x00;  //直接返回
        case 0x01:  //序号二分查找
            tempinfo.mode = 0x01;   //序号
            tempinfo.offset = 0x00;
            return __search_circular(info, manager, &tempinfo, search_mid_way);
        case 0x02: //时间二分查找
            tempinfo.mode = 0x00;  //时间
            tempinfo.offset = 0x04;
            return __search_circular(info, manager, &tempinfo, search_mid_way);
        case 0x03: //时间枚举法查找
            tempinfo.mode = 0x00; //时间
            tempinfo.offset = 0x04;
            return __search_circular(info, manager, &tempinfo, search_enum_way);
        default:
            break;
        }
    }
    return -1;
}

int16_t nd_log_append(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t unit_size)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL,* block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint16_t len01 = 0x00, len02 = 0x00;
    uint16_t depth = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    if ((device01 != NULL) && (block01 != NULL) && (block01->hook != NULL))
    {
        driver01 = device01->dev->dev_driver_ops;
        block01->hook((uint16_t)index, &addr01, &len01, &depth);
        //先获取管理区
        device02 = look_up_device(instance, block01->relation_device);
        block02 = look_up_block(instance, device02, block01->relation_name);
        if ((device02 != NULL) && (block02 != NULL) && (block02->hook != NULL))
        {
            driver02 = device02->dev->dev_driver_ops;
            if ((driver02 != NULL) && (driver02->dev_read != NULL))
            {
                block02->hook((uint16_t)index, &addr02, &len02, NULL);
                if (len02 != sizeof(struct log_manager_t))
                {
                    return -1;
                }
                if (0x00 > driver02->dev_read(device02->dev, block02->first_addr + addr02, sizeof(struct log_manager_t), (uint8_t*)&manager))
                {
                    return -1;
                }
                if (0xA5A5 + manager.num + manager.unit_size + manager.wr_ptr != manager.check_sum)
                {
                    manager.max_num = depth;
                    manager.unit_size = unit_size;
                    manager.num = 0;
                    manager.wr_ptr = 0;
                }
            }
        }
        //存储数据
        addr01 += manager.wr_ptr * manager.unit_size;
        if ((driver01 != NULL) && (driver01->dev_write != NULL))
        {
            if (0x00 > driver01->dev_write(device01->dev, block01->first_addr + addr01, unit_size, buf))
            {
                return -1;
            }
        }
        //保存管理区
        if ((driver02 != NULL) && (driver02->dev_write != NULL))
        {
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
            if (0x00 > driver02->dev_write(device02->dev, block02->first_addr + addr02, sizeof(struct log_manager_t), (uint8_t*)&manager))
            {
                return -1;
            }
            return unit_size;
        }
    }
    return -1;
}

int16_t nd_log_output(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size, struct circular_info_t* info)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL, * block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint16_t len01 = 0x00, len02 = 0x00;
    uint16_t depth = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    if ((device01 != NULL) && (block01 != NULL) && (block01->hook != NULL))
    {
        driver01 = device01->dev->dev_driver_ops;
        block01->hook((uint16_t)index, &addr01, &len01, &depth);
        //先获取管理区
        if ((info != NULL) && (info->valid_num == 0x00))
        {
            device02 = look_up_device(instance, block01->relation_device);
            block02 = look_up_block(instance, device02, block01->relation_name);
            if ((device02 != NULL) && (block02 != NULL) && (block02->hook != NULL))
            {
                driver02 = device02->dev->dev_driver_ops;
                if ((driver02 != NULL) && (driver02->dev_read != NULL))
                {
                    block02->hook((uint16_t)index, &addr02, &len02, NULL);
                    if (0x00 > driver02->dev_read(device02->dev, block02->first_addr + addr02, sizeof(struct log_manager_t), (uint8_t*)&manager))
                    {
                        return -1;
                    }
                    if (0xA5A5 + manager.num + manager.unit_size + manager.wr_ptr != manager.check_sum)
                    {
                        return -1;
                    }
                    info->unit_size = manager.unit_size;
                    info->valid_num = manager.num;
                    info->read_ptr = 0x00;
                    if (manager.num == manager.max_num)
                    {
                        info->read_ptr = manager.wr_ptr;
                    }
                    //搜索信息处理
                    search_circular(instance, device01->dev, block01->first_addr + addr01, info, &manager);
                }
            }
        }
    }   
    //读取数据
    if (info != NULL)
    {
        addr01 = info->read_ptr * info->unit_size;
        if ((driver01 != NULL) && (driver01->dev_read != NULL))
        {
            if (0x00 > driver01->dev_read(device01->dev, block01->first_addr + addr01, info->unit_size, buf))
            {
                return -1;
            }
            if (info->read_ptr < depth)
            {
                info->read_ptr++;
            }
            else
            {
                info->read_ptr = 0x00;
            }
            info->curr_num++;
            return info->unit_size;
        }
    }
    return -1;
}

int nd_log_clean(struct instance_t* instance, uint32_t index)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL, * block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr02 = 0x00;
    uint16_t len02 = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    if ((device01 != NULL) && (block01 != NULL) && (block01->hook != NULL))
    {
        device02 = look_up_device(instance, block01->relation_device);
        block02 = look_up_block(instance, device02, block01->relation_name);
        if ((device02 != NULL) && (block02 != NULL) && (block02->hook != NULL))
        {
            driver02 = device02->dev->dev_driver_ops;
            if ((driver02 != NULL) && (driver02->dev_write != NULL))
            {
                block02->hook((uint16_t)index, &addr02, &len02, NULL);
                if (0x00 > driver02->dev_write(device02->dev, block02->first_addr + addr02, sizeof(struct log_manager_t), (uint8_t*)&manager))
                {
                    return -1;
                }
                return 0x00;
            }
        }
    }
    return -1;
}
#endif

int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint16_t len = 0x00;
    uint32_t addr = 0x00;

    device = look_up_device(instance, (uint8_t)(index >> 24));//查找设备
    //查找块
    block = look_up_block(instance, device, (uint8_t)(index >> 16));
    if ((device != NULL) && (block != NULL))
    {
        driver = device->dev->dev_driver_ops;
        if ((driver != NULL) && (driver->dev_read != NULL))
        {
            //计算地址
            if (block->hook != NULL)
            {
                if (0x00 < block->hook((uint16_t)index, &addr, &len, NULL))
                {
                    addr += block->first_addr;
                }
                driver->dev_read(device->dev, addr, len, buf);
                //这里没判断返回值和校验
            }
            return len;
        }
    }
    return -1;
}


int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint16_t len = 0x00;
    uint32_t addr = 0x00;

    device = look_up_device(instance, (uint8_t)(index >> 24));//查找设备
    //查找块
    block = look_up_block(instance, device, (uint8_t)(index >> 16));
    if ((device != NULL) && (block != NULL))
    {
        driver = device->dev->dev_driver_ops;
        if ((driver != NULL) && (driver->dev_write != NULL))
        {//计算地址            
            if (block->hook != NULL)
            {
                if (0x00 < block->hook((uint16_t)index, &addr, &len, NULL))
                {
                    addr += block->first_addr;
                }
                driver->dev_write(device->dev, addr, len, buf);
                //这里没判断返回值和校验
            }
            return len;
        }
    }
    return -1;
}

//目前resource manger 数据操作介绍
//全部为数据块的操作，不是单个数据的操作
//需要操作数据块的单个数据，目前有两种情况：
// 情况1：当前块的分布是通过结构体来分配的，通过块的首地址和偏移来访问；这种情况需要通过结构体变量名来获取偏移值
// 情况2：当前块的分布是通过表格来分配的，通过块的收地址和偏移来访问；这种情况需要初始化的时候把数据表格注册到reource；






