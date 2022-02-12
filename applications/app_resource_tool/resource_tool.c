#include "nidavellir.h"
#include "resource_tool.h"


#define res_conditions_true(a,b) if(a) { return b; } //a的调整满足返回b

struct search_tempinfo_t
{
    const struct dev_info_t* dev;
    uint32_t addr;
    uint32_t max_num;
    uint16_t offset;
    uint16_t unit_size;
};

int do_sum(uint8_t* buff, uint16_t length)
{
    uint16_t result = 0;

    while (length--)
    {
        result += *buff++;
    }

    return result;
}

int16_t verify_data(uint32_t addr, uint16_t offset, uint8_t* buf, uint16_t size, uint16_t len, const struct dev_info_t* dev)
{
    const struct driver_ops_t* driver = NULL;
    uint16_t row[] = { offset, offset + size, len - 0x02, len - 0x01 };
    uint16_t coordinate[4][2] = { {0,0}, {0,0}, {0,0}, {0,0} };
    uint16_t checksum = 0xA5A5, rchecksum = 0;
    uint16_t i, u_len, s, m, n, k, cnt;
    uint8_t s_buf[128];

    driver = dev->dev_driver_ops;
    if (len <= 128)  //小于128字节
    {
        res_conditions_true(0x00 > driver->dev_read(dev, addr, len, s_buf), -1);
        memcpy(buf, s_buf + offset, size);
        checksum += do_sum(s_buf, len - 0x02);
        rchecksum = ((uint16_t)s_buf[len - 0x02] << 8);
        rchecksum += s_buf[len - 0x01];
        res_conditions_true(checksum != rchecksum, -1);
        return 1;
    }

    for (i = 0; i < 0x04; i++)
    {
        coordinate[i][0] = row[i] / 128;
        coordinate[i][1] = row[i] % 128;
    }

    cnt = len / 128;
    s = 0x00; i = 0x00; n = 0x00; m = coordinate[0][1];

    for (k = 0x00; k < cnt + 1; k++)
    {
        u_len = (k == cnt) ? (len % 128) : 128;
        res_conditions_true(0x00 > driver->dev_read(dev, addr + k * 128, u_len, s_buf), -1);
        //更新数据类容
        if ((k >= coordinate[0][0]) && (k <= coordinate[1][0]))
        {
            while (s < size)
            {
                buf[s++] = s_buf[m++];
                if (m == u_len)
                {
                    m = 0x00; break;
                }
            }
        }
        //更新校验码
        while (i < len)
        {
            if (i < len - 0x02)
            {
                checksum += s_buf[n];
            }

            if ((k == coordinate[2][0]) && (n == coordinate[2][1]))
            {
                rchecksum = ((uint16_t)s_buf[n] << 8);
            }

            if ((k == coordinate[3][0]) && (n == coordinate[3][1]))
            {
                rchecksum += s_buf[n];
            }
            i++; n++;

            if (n == u_len)
            {
                n = 0x00; break;
            }
        }
    }
    res_conditions_true(checksum != rchecksum, -1);

    return 1;
}

int16_t update_verify(uint32_t addr, uint16_t offset, uint8_t* buf, uint16_t size, uint16_t len, const struct dev_info_t* dev)
{
    const struct driver_ops_t* driver = NULL;
    uint16_t row[] = { offset, offset + size, len - 0x02, len - 0x01 };
    uint16_t coordinate[4][2] = { {0,0}, {0,0}, {0,0}, {0,0} };
    uint16_t checksum = 0xA5A5;
    uint16_t i,u_len,s, m, n, k, cnt;
    uint8_t s_buf[128];

    driver = dev->dev_driver_ops;
    if (len <= 128)  //小于128字节
    {
        res_conditions_true(0x00 > driver->dev_read(dev,addr,len,s_buf), -1);
        memcpy(s_buf + offset, buf, size);
        checksum += do_sum(s_buf, len - 0x02);
        s_buf[len - 0x02] = (uint8_t)(checksum >> 8);
        s_buf[len - 0x01] = (uint8_t)checksum;
        res_conditions_true(0x00 > driver->dev_write(dev, addr, len, s_buf), -1);

        return 1;
    }

    for (i = 0; i < 0x04; i++)
    {
        coordinate[i][0] = row[i] / 128;
        coordinate[i][1] = row[i] % 128;
    }

    cnt = len / 128;
    s = 0x00; i = 0x00; n = 0x00; m = coordinate[0][1];
    
    for (k = 0x00; k < cnt + 1; k++)
    {
        u_len = (k == cnt) ? (len % 128) : 128;
        res_conditions_true(0x00 > driver->dev_read(dev, addr + k * 128, u_len, s_buf), -1);
        //更新数据类容
        if ((k >= coordinate[0][0]) && (k <= coordinate[1][0]))
        {            
            while (s < size)
            {
                s_buf[m++] = buf[s++];
                if (m == u_len)
                {
                    m = 0x00; break;
                }
            }
        }
        //更新校验码
        while (i < len)
        {
            if (i < len - 0x02)
            {
                checksum += s_buf[n];
            }
            
            if ((k == coordinate[2][0]) && (n == coordinate[2][1]))
            {
                s_buf[n] = (uint8_t)(checksum >> 8);
            }

            if ((k == coordinate[3][0]) && (n == coordinate[3][1]))
            {
                s_buf[n] = (uint8_t)checksum;
            }

            i++; n++;

            if (n == u_len)
            {
                n = 0x00; break;
            }
        }
        //保存数据
        res_conditions_true(0x00 > driver->dev_write(dev, addr + k * 128, u_len, s_buf), -1);
    }


    return 1;
}

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
        res_conditions_true(device->device_name == device_name, device)
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
            res_conditions_true(block->name == block_name, block)
        }
    }

    return NULL;
}

void* nd_malloc(struct instance_t *instance, uint8_t block_name, uint32_t size)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, instance->ram_device);
    res_conditions_true(device == NULL, NULL)

    block = look_up_block(instance, device, block_name);
    res_conditions_true(block == NULL, NULL)

    return __nd_malloc(block->first_addr, &block->used, size, block->size);
}

//注册API
uint32_t *add_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
    struct resource_block_t* re_block = NULL;

    device = look_up_device(instance,device_name);//先获取自己的device
    res_conditions_true(device == NULL, NULL);
    //判断申请的字节有没有超限
    res_conditions_true((device->used + block_size) > device->size, NULL);
    block = look_up_block(instance, device, block_name);//在找block
    res_conditions_true(block != NULL, (uint32_t*)block->first_addr);

    re_block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
    res_conditions_true(re_block == NULL, NULL);

    block = __nd_malloc(re_block->first_addr, &re_block->used, sizeof(struct resource_block_t), re_block->size);
    res_conditions_true(block == NULL, NULL);

    block->first_addr = device->first_addr + device->used;
    block->name = block_name;
    init_list_head(&block->sibling);
    block->size = block_size;
    block->used = 0x00;
    device->used += block_size;
    list_add_tail(&block->sibling, &device->children);   

    return (uint32_t *)block->first_addr;       
}

uint32_t *add_inside_block(struct instance_t* instance, uint8_t device_name,uint8_t block_name, uint8_t inside_block_name, uint32_t block_size, uint32_t offset)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct resource_block_t* block = NULL;
    struct resource_block_t* inside_block = NULL;
    struct resource_block_t* resource_block = NULL;

    device = look_up_device(instance, device_name);//先获取device
    res_conditions_true(device == NULL, NULL);
    //判断申请的字节有没有超限
    res_conditions_true((device->used + block_size) > device->size, NULL);
    block = look_up_block(instance, device, block_name);//在找block
    res_conditions_true(block == NULL, NULL);
    ram_device = look_up_device(instance, instance->ram_device); //获取ram_device
    res_conditions_true(ram_device == NULL, NULL);

    resource_block = look_up_block(instance, ram_device, instance->resource_block);
    res_conditions_true(resource_block == NULL, NULL);

    inside_block = __nd_malloc(resource_block->first_addr, &resource_block->used, sizeof(struct resource_block_t), resource_block->size);
    res_conditions_true(inside_block == NULL, NULL);

    inside_block->first_addr = block->first_addr + offset;
    inside_block->name = inside_block_name;
    init_list_head(&inside_block->sibling);
    inside_block->size = block_size;
    inside_block->used = 0x00;
    block->used += block_size;
    list_add_tail(&inside_block->sibling, &device->children);
    return (uint32_t *)inside_block->first_addr;
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
    res_conditions_true(device == NULL, NULL);

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
    res_conditions_true(block == NULL, NULL);

    block->first_addr = first_addr;
    block->name = resource_block_name;
    init_list_head(&block->sibling);
    block->size = resource_blok_size;
    block->used = used;
    instance->resource_block = resource_block_name;
    list_add_tail(&block->sibling, &device->children);

    return instance;
}

//注册设备的API 先创建RAM设备；
int8_t add_device(struct instance_t* instance, uint8_t device_name, const struct dev_info_t *dev,uint32_t sector)
{
    struct block_device_t* device = NULL;
    struct block_device_t* ram_device = NULL;
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, device_name);//在分配其他的
    res_conditions_true(device != NULL, 1);

    ram_device = look_up_device(instance, instance->ram_device); //获取ram_device
    res_conditions_true(ram_device == NULL, -1);
    block = look_up_block(instance, ram_device, instance->resource_block);
    res_conditions_true(block == NULL, -1);

    device = __nd_malloc(block->first_addr,&block->used,sizeof(struct block_device_t),block->size);
    res_conditions_true(device == NULL, -1);

    device->used = 0;
    device->dev = dev;
    device->device_name = device_name;
    device->first_addr = 0x00000000;
    device->page_size = sector;
    device->size = dev->data_size;
    init_list_head(&device->sibling);
    init_list_head(&device->children);
    list_add_tail(&device->sibling, &instance->dev_list);
    
    return 1;
}

struct resource_map_t* register_map(struct instance_t* instance,uint8_t map_name,
    int (*fun)(struct instance_t *instance, uint32_t id,uint8_t *buf, uint16_t buf_size, void *ctx, uint16_t mode))
{
    struct resource_block_t* block = NULL;
    struct resource_map_t* map = NULL;
    struct list_head* p;

    list_for_each(p, &instance->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        res_conditions_true(map->map_name == map_name, map);
    }

    block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
    res_conditions_true(block == NULL, NULL);

    map = __nd_malloc(block->first_addr, &block->used, sizeof(struct block_device_t), block->size);
    res_conditions_true(map == NULL, NULL);

    map->map_name = map_name;
    map->fun = fun;
    list_add_tail(&map->sibling, &instance->map_list);

    return map;
}

int16_t look_up_map(struct instance_t* instance,uint32_t id, uint8_t *buf, uint16_t buf_size, void *ctx,uint16_t mode)
{
    struct resource_map_t* map = NULL;
    struct list_head* p;

    list_for_each(p, &instance->map_list)
    {
        map = list_entry(p, struct resource_map_t, sibling);
        res_conditions_true(map->fun != NULL, map->fun(instance, id, buf, buf_size, ctx, mode));
    }

    return -1;
}


int16_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{
    return look_up_map(instance, id, buf, buf_size, ctx, RESOURCE_MAP_GET);
}

int16_t nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{
    return look_up_map(instance, id, buf, buf_size, ctx, RESOURCE_MAP_SET);
}

int16_t nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{
    return look_up_map(instance, id, buf, buf_size, ctx, RESOURCE_MAP_ACTION);
}

int16_t nd_get_info(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{
    uint16_t mode = 0x0100;
    return look_up_map(instance, id, buf, buf_size, ctx, mode);
}
int16_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
    const struct driver_ops_t* driver = NULL;

    //查找设备
    device = look_up_device(instance, device_name);
    res_conditions_true(device == NULL, -1);
    driver = device->dev->dev_driver_ops;

    res_conditions_true(driver == NULL, -1);
    res_conditions_true(driver->dev_read == NULL, -1);

    block = look_up_block(instance, device, block_name);

    res_conditions_true((block != NULL) && (buf_size <= block->size),
        driver->dev_read(device->dev, block->first_addr, block->size, buf));

    return 1;
}

int16_t nd_set_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf, uint32_t buf_size)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
    const struct driver_ops_t* driver = NULL;

    //查找设备
    device = look_up_device(instance, device_name);
    res_conditions_true(device == NULL, -1);
    driver = device->dev->dev_driver_ops;

    res_conditions_true(driver == NULL, -1);
    res_conditions_true(driver->dev_write == NULL, -1);

    block = look_up_block(instance, device, block_name);
    res_conditions_true((block != NULL) && (buf_size <= block->size), 
        driver->dev_write(device->dev, block->first_addr, block->size, buf));

    return 1;
}

uint32_t* __register(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(struct instance_t* instance, uint16_t id, uint32_t *addr, uint32_t* len, uint32_t* depth))
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;
    struct resource_block_t* re_block = NULL;

    device = look_up_device(instance, device_name);//先获取自己的device
    res_conditions_true(device == NULL, NULL);

    //判断申请的字节有没有超限
    res_conditions_true((device->used + block_size) > device->size, NULL);
    block = look_up_block(instance, device, block_name);//在找block
    res_conditions_true(block != NULL, (uint32_t*)block->first_addr);

    re_block = look_up_block(instance, look_up_device(instance, instance->ram_device), instance->resource_block);
    res_conditions_true(re_block == NULL, NULL);

    block = __nd_malloc(re_block->first_addr, &re_block->used, sizeof(struct resource_block_t), re_block->size);
    res_conditions_true(block == NULL, NULL);

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
    return (uint32_t*)block->first_addr;
}

uint32_t* register_log(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(struct instance_t* instance, uint16_t id, uint32_t* addr, uint32_t* len, uint32_t* depth))
{
    return __register(instance, device_name, block_name, relation_device, relation_name, block_size, fun);
}

uint32_t* register_parameter(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(struct instance_t* instance, uint16_t id, uint32_t *addr, uint16_t* len, uint16_t *offset))
{
    return __register(instance, device_name, block_name, 0x00, 0x00, block_size, (int16_t(*)(struct instance_t* instance, uint16_t id, uint32_t * addr, uint32_t * len, uint32_t * depth))fun);
}

int16_t nd_size(struct instance_t* instance, uint32_t id)
{
    return look_up_map(instance, id, NULL, 0, NULL, RESOURCE_MAP_GET);  
}

int16_t get_block_frist_addr(struct instance_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr)
{
    struct block_device_t* device = NULL;
    struct resource_block_t* block = NULL;

    device = look_up_device(instance, device_name);//先获取自己的device
    res_conditions_true(device == NULL, -1);

    block = look_up_block(instance, device, block_name);//在找block
    res_conditions_true((block == NULL) || (addr == NULL), -1);

    *addr = block->first_addr;
    return 1;
}

uint32_t get_curr_mark(const struct dev_info_t* dev, uint32_t addr)
{
    const struct driver_ops_t* driver = NULL;
    uint8_t buf[10] = { 0 };

    driver = dev->dev_driver_ops;
    res_conditions_true((driver == NULL) || (driver->dev_read == NULL), 0xFFFFFFFF);

    driver->dev_read(dev, addr, 0x04, buf);
    //return buf_to_uint32_lsb(buf);
    return buf_to_uint32_msb(buf);
}

uint16_t search_enum_way(uint32_t key, uint16_t start, uint16_t end, struct search_tempinfo_t *info, uint8_t direct)
{
    uint16_t ptr, next_ptr;
    uint32_t cmp, next_cmp;

    ptr = end;
    next_ptr = ((start + 1) > end) ? start : (end - 1);
    cmp = get_curr_mark(info->dev,info->addr + info->offset + ptr * info->unit_size);
    while (next_ptr >= start)
    {
        next_cmp = get_curr_mark(info->dev, info->addr + info->offset + next_ptr * info->unit_size);
        res_conditions_true(cmp == key, ptr);

        if ((key >= next_cmp) && (key < cmp))
        {
            res_conditions_true(key == next_cmp, next_ptr);
            return (direct == 0x00) ? ptr : next_ptr;
        }
        else
        {
            ptr = next_ptr;
            cmp = next_cmp;
            res_conditions_true(next_ptr == 0x00, 0xFFFF)
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
        mid_cmp = get_curr_mark(info->dev, info->addr + info->offset + mid_ptr * info->unit_size);
        res_conditions_true(mid_cmp == key, mid_ptr);

        if (key > mid_cmp)
        {
            res_conditions_true(mid_ptr == max_ptr, 0xFFFF);
            min_ptr = mid_ptr + 1;
            temp_cmp = get_curr_mark(info->dev, info->addr + info->offset + min_ptr * info->unit_size);
            res_conditions_true(key == temp_cmp, min_ptr);
            res_conditions_true(key < temp_cmp, (min_ptr - direct));
        }
        else
        {
            res_conditions_true(mid_ptr == min_ptr, 0xFFFF);
            max_ptr = mid_ptr - 0x01;
            temp_cmp = get_curr_mark(info->dev, info->addr + info->offset + max_ptr * info->unit_size);
            res_conditions_true(key == temp_cmp, max_ptr);
            res_conditions_true(key > temp_cmp, (mid_ptr - direct));
        }
    }
    return mid_ptr;
}

int __search_smoke(struct circular_info_t* info, struct search_tempinfo_t* tempinfo, uint16_t *sptr,uint8_t mode)
{
    uint32_t temp1 = get_curr_mark(tempinfo->dev, tempinfo->addr + tempinfo->offset + sptr[0] * tempinfo->unit_size);
    uint32_t temp2 = get_curr_mark(tempinfo->dev, tempinfo->addr + tempinfo->offset + sptr[1] * tempinfo->unit_size);

    res_conditions_true(info->end_mark < info->start_mark, -1);
    switch (mode)
    {
    case 0x00: //没反转
    {
        if (info->start_mark <= temp1)
        {
            info->start_mark = temp1;
        }
        if (info->end_mark >= temp2)
        {
            info->end_mark = temp2;
        }
        res_conditions_true((info->start_mark > temp2) || (info->end_mark < temp1), -1);
    }
        break;
    case 0x02:  //有反转
    {
        uint32_t temp3 = get_curr_mark(tempinfo->dev, tempinfo->addr + tempinfo->offset + sptr[2] * tempinfo->unit_size);
        uint32_t temp4 = get_curr_mark(tempinfo->dev, tempinfo->addr + tempinfo->offset + sptr[3] * tempinfo->unit_size);

        if (info->start_mark <= temp3)
        {
            info->start_mark = temp3;
        }

        if (info->end_mark >= temp2)
        {
            info->end_mark = temp2;
        }

        if ((info->start_mark > temp4) && (info->start_mark <= temp1))
        {
            info->start_mark = temp1;
        }

        if ((info->end_mark >= temp4) && (info->end_mark < temp1))
        {
            info->end_mark = temp4;
        }
        res_conditions_true((info->start_mark > temp2) || (info->end_mark < temp3), -1);
    }
        break;
    }
    
    return 1;
}

int16_t __search_circular(struct circular_info_t* info, struct log_manager_t* manager, struct search_tempinfo_t *tempinfo,
    uint16_t (*fun)(uint32_t key, uint16_t start, uint16_t end, struct search_tempinfo_t* info, uint8_t direct))
{
    uint16_t ptr[4] = { manager->r_ptr, 0x00, 0x00, 0x00 };
    uint16_t start_ptr = 0x00, end_ptr = 0x00;
    uint8_t cnt = 0x00;

    if (manager->w_ptr > 0x00)
    {
        ptr[1] = manager->w_ptr - 0x01;
    }

    if ((manager->r_ptr >= manager->w_ptr) && (manager->num == manager->max_num))//说明已经反转了,反转了需要找2块区域
    {
        ptr[0] = 0x00;
        ptr[2] = manager->r_ptr;
        ptr[3] = tempinfo->max_num - 0x01;
        if (manager->w_ptr == 0x00)  //刚好反转的时候
        {
            ptr[0] = manager->r_ptr;
            ptr[1] = tempinfo->max_num - 0x01;
        }
        cnt = 0x02;  //需要找两次；
    }

    if (0x00 > __search_smoke(info, tempinfo, ptr, cnt))
    {
        info->read_ptr = 0x00;
        info->valid_num = 0x00;

        return 0x00;
    }

    start_ptr = fun(info->start_mark, ptr[0], ptr[1], tempinfo, 0x00);
    if ((start_ptr == 0xFFFF) && (cnt == 0x02))
    {
        start_ptr = fun(info->start_mark, ptr[2], ptr[3], tempinfo, 0x00);
    }

    end_ptr = fun(info->end_mark, ptr[0], ptr[1], tempinfo, 0x01);
    if ((end_ptr == 0xFFFF) && (cnt == 0x02))
    {
        end_ptr = fun(info->end_mark, ptr[2], ptr[3], tempinfo, 0x01);
    }

    if ((start_ptr != 0xFFFF) && (end_ptr != 0xFFFF))
    {
        info->read_ptr = start_ptr;

        if (end_ptr >= start_ptr)
        {
            info->valid_num = end_ptr - start_ptr + 0x01;
        }
        else
        {
            info->valid_num = end_ptr + tempinfo->max_num - start_ptr + 0x01;
        }
    }
    else
    {
        info->read_ptr = 0x00;
        info->valid_num = 0x00;
    }

    return 0x00;
}

int16_t search_circular(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, struct circular_info_t* info, struct log_manager_t* manager,uint32_t max_num)
{
    struct search_tempinfo_t tempinfo;

    memset((uint8_t*)&tempinfo, 0x00, sizeof(struct search_tempinfo_t));
    tempinfo.dev = dev;
    tempinfo.addr = addr;
    tempinfo.unit_size = manager->unit_size;
    tempinfo.max_num = max_num;

    if ((info != NULL) && (dev != NULL) && (dev->dev_driver_ops != NULL))
    {
        switch (info->mode)
        {
        case 0x00:  //全读
            return 0x00;  //直接返回
        case 0x01:  //序号二分查找
            tempinfo.offset = 0x01;
            //tempinfo.offset = 0x00;
            return __search_circular(info, manager, &tempinfo, search_mid_way);
        case 0x02: //时间二分查找
            tempinfo.offset = 0x06;
            //tempinfo.offset = 0x04;
            return __search_circular(info, manager, &tempinfo, search_mid_way);
        case 0x03: //时间枚举法查找
            tempinfo.offset = 0x04;
            return __search_circular(info, manager, &tempinfo, search_enum_way);
        default:
            break;
        }
    }
    return -1;
}
#ifndef WIN32
int application_norflash_erase(uint32_t addr, uint32_t len, const struct driver_ops_t* driver, const struct dev_info_t* dev)
{
    uint32_t temp;
    int ret = -1;

    temp = ((addr + len) / 4096) * 4096;
    if ((addr >= temp) && (addr + len < temp) && (driver->dev_control != NULL))
    {
        ret = driver->dev_control(dev, 0x01, &temp);
    }
    return ret;
}
#endif // WIN32

int16_t nd_log_append(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t unit_size)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL,* block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint32_t len01 = 0x00, len02 = 0x00;
    uint32_t depth = 0x00;
    uint32_t over_num = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    res_conditions_true(device01 == NULL, -1);
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    res_conditions_true((block01 == NULL) || (block01->hook == NULL), -1);
    block01->hook(instance, (uint16_t)index, &addr01, &len01, &depth);

    driver01 = device01->dev->dev_driver_ops;
    res_conditions_true((driver01 == NULL) || (driver01->dev_write == NULL), -1);
    //先获取管理区
    device02 = look_up_device(instance, block01->relation_device);
    res_conditions_true(device02 == NULL, -1);
    block02 = look_up_block(instance, device02, block01->relation_name);
    res_conditions_true((block02 == NULL) || (block02->hook == NULL), -1);
    block02->hook(instance, (uint16_t)index, &addr02, &len02, NULL);
    res_conditions_true(len02 != (sizeof(struct log_manager_t)+2), -1);
    //读管理区
    driver02 = device02->dev->dev_driver_ops;
    res_conditions_true((driver02 == NULL) || (driver02->dev_read == NULL) || (driver02->dev_write == NULL), -1);
    if (0x00 > verify_data(block02->first_addr + addr02, 0x00, (uint8_t*)&manager,
        sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev))
    {
        manager.max_num = depth;
        manager.unit_size = unit_size;
        manager.num = 0;
        manager.r_ptr = 0;
        manager.w_ptr = 0;
    }
    if (manager.unit_size != 0x00)
    {
        over_num = len01 / manager.unit_size;
    }
    //存储数据
    addr01 += manager.w_ptr * manager.unit_size;    
#ifndef WIN32
    //判断是否需要察扇区
    if (device01->dev->dev_type == DEV_FLASH)//FLASH才会擦除
    {
        res_conditions_true(0x00 > application_norflash_erase(block01->first_addr + addr01, 
            unit_size, driver01, device01->dev), -1);
    }
#endif // WIN32
    res_conditions_true(0x00 > driver01->dev_write(device01->dev, block01->first_addr + addr01, 
        unit_size, buf), -1);
    
    //保存管理区

    if (manager.num >= manager.max_num)
    {
        manager.num = manager.max_num;
        manager.r_ptr++;
    }
    else
    {
        manager.num++;
    }
        
    if (manager.r_ptr >= over_num)
    {
        manager.r_ptr = 0x00;
    }
   
    manager.w_ptr++;           
    if (manager.w_ptr >= over_num)
    {
        manager.w_ptr = 0x00;
    }
        
    res_conditions_true(0x00 > update_verify(block02->first_addr + addr02, 0x00, (uint8_t*)&manager, 
        sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev), -1);


    return unit_size;
}

int16_t nd_log_append_revise(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t offset, uint16_t unit_size)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL,* block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint32_t len01 = 0x00, len02 = 0x00;
    uint32_t depth = 0x00;
    uint32_t over_num = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    res_conditions_true(device01 == NULL, -1);
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    res_conditions_true((block01 == NULL) || (block01->hook == NULL), -1);
    block01->hook(instance, (uint16_t)index, &addr01, &len01, &depth);

    driver01 = device01->dev->dev_driver_ops;
    res_conditions_true((driver01 == NULL) || (driver01->dev_write == NULL), -1);
    //先获取管理区
    device02 = look_up_device(instance, block01->relation_device);
    res_conditions_true(device02 == NULL, -1);
    block02 = look_up_block(instance, device02, block01->relation_name);
    res_conditions_true((block02 == NULL) || (block02->hook == NULL), -1);
    block02->hook(instance, (uint16_t)index, &addr02, &len02, NULL);
    res_conditions_true(len02 != (sizeof(struct log_manager_t)+2), -1);
    //读管理区
    driver02 = device02->dev->dev_driver_ops;
    res_conditions_true((driver02 == NULL) || (driver02->dev_read == NULL) || (driver02->dev_write == NULL), -1);
    if (0x00 > verify_data(block02->first_addr + addr02, 0x00, (uint8_t*)&manager,
        sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev))
    {
        manager.max_num = depth;
        manager.unit_size = unit_size;
        manager.num = 0;
        manager.r_ptr = 0;
        manager.w_ptr = 0;
    }
    if (manager.unit_size != 0x00)
    {
        over_num = len01 / manager.unit_size;
    }
    
    //存储数据
    if ((manager.num >= manager.max_num) && (manager.w_ptr == 0x00))
    {
        manager.w_ptr = over_num - 0x01;
    }
    else
    {
        res_conditions_true(manager.w_ptr == 0x00, -1);
        manager.w_ptr--;
    }
    
    addr01 += manager.w_ptr * manager.unit_size + offset;    
    res_conditions_true(0x00 > driver01->dev_write(device01->dev, block01->first_addr + addr01, 
        unit_size, buf), -1);

    return unit_size;
}

int16_t nd_log_output(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size, struct circular_info_t* info)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL, * block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint32_t len01 = 0x00, len02 = 0x00;
    uint32_t depth = 0x00, over_num = 0x00;
    uint16_t ut_ptr = 0x00;

    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    res_conditions_true(device01 == NULL, -1);
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    res_conditions_true((block01 == NULL) || (block01->hook == NULL), -1);
    block01->hook(instance, (uint16_t)index, &addr01, &len01, &depth);

    driver01 = device01->dev->dev_driver_ops;
    res_conditions_true((driver01 == NULL) || (driver01->dev_read == NULL), -1);
    res_conditions_true(info == NULL, -1);
    //先获取管理区
    if (info->valid_num == 0x00)
    {
        device02 = look_up_device(instance, block01->relation_device);
        res_conditions_true(device02 == NULL, -1);
        block02 = look_up_block(instance, device02, block01->relation_name);
        res_conditions_true((block02 == NULL) || (block02->hook == NULL), -1);
        block02->hook(instance, (uint16_t)index, &addr02, &len02, NULL);

        driver02 = device02->dev->dev_driver_ops;
        res_conditions_true((driver02 == NULL) || (driver02->dev_read == NULL), -1);
        res_conditions_true(0x00 > verify_data(block02->first_addr + addr02, 0x00, (uint8_t*)&manager,
            sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev), -1);
        info->unit_size = manager.unit_size;
        info->valid_num = manager.num;
        info->read_ptr = manager.r_ptr;
        res_conditions_true((info->valid_num == 0x00) || (manager.unit_size == 0x00), 0x00);
        search_circular(instance, device01->dev, block01->first_addr + addr01, info, &manager, len01 / manager.unit_size);
    }
    //读取数据
    res_conditions_true(info->curr_num >= info->valid_num, 0);
    res_conditions_true(info->unit_size > buf_size, 0);
    res_conditions_true(info->unit_size == 0x00, -1);
    over_num = len01 / info->unit_size;

    if ((info->read_ptr + info->curr_num) >= over_num)
    {
        ut_ptr = (info->read_ptr + info->curr_num) - over_num;
    }
    else
    {
        ut_ptr = info->read_ptr + info->curr_num;
    }

    addr01 += ut_ptr * info->unit_size;
    res_conditions_true(0x00 > driver01->dev_read(device01->dev, block01->first_addr + addr01, info->unit_size, buf), -1);
    info->curr_num++;

    return info->unit_size;
}

int16_t nd_log_append_rollback(struct instance_t* instance, uint32_t index, uint32_t last_time,uint32_t curr_time)
{
    struct circular_info_t info;
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver01 = NULL, * driver02 = NULL;
    struct resource_block_t* block01 = NULL, * block02 = NULL;
    struct log_manager_t manager;
    uint32_t addr01 = 0x00, addr02 = 0x00;
    uint32_t len01 = 0x00, len02 = 0x00;
    uint32_t depth = 0x00, over_num = 0x00;
    uint16_t ut_ptr = 0x00;
    uint32_t last_num, curr_num;
    
    memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
    memset((uint8_t*)&manager, 0x00, sizeof(struct log_manager_t));
    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备   
    res_conditions_true(device01 == NULL, -1);
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    res_conditions_true((block01 == NULL) || (block01->hook == NULL), -1);
    block01->hook(instance, (uint16_t)index, &addr01, &len01, &depth);

    driver01 = device01->dev->dev_driver_ops;
    res_conditions_true((driver01 == NULL) || (driver01->dev_read == NULL), -1);
    //先获取管理区
    device02 = look_up_device(instance, block01->relation_device);
    res_conditions_true(device02 == NULL, -1);
    block02 = look_up_block(instance, device02, block01->relation_name);
    res_conditions_true((block02 == NULL) || (block02->hook == NULL), -1);
    block02->hook(instance, (uint16_t)index, &addr02, &len02, NULL);

    driver02 = device02->dev->dev_driver_ops;
    res_conditions_true((driver02 == NULL) || (driver02->dev_read == NULL), -1);
    res_conditions_true(0x00 > verify_data(block02->first_addr + addr02, 0x00, (uint8_t*)&manager,
        sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev), -1);
    res_conditions_true((manager.num == 0x00) || (manager.unit_size == 0x00), 0x00);
    over_num = len01 / manager.unit_size;
    
    info.mode = 0x02; //时间的二分查找；
    info.unit_size = manager.unit_size;
    info.valid_num = manager.num;
    info.read_ptr = manager.r_ptr;
    info.start_mark = 0;
    //计算上一次的
    info.end_mark = last_time;
    search_circular(instance, device01->dev, block01->first_addr + addr01, &info, &manager, len01 / manager.unit_size);
    last_num = info.valid_num;
    //当前一次的
    info.end_mark = curr_time;
    search_circular(instance, device01->dev, block01->first_addr + addr01, &info, &manager, len01 / manager.unit_size);
    curr_num = info.valid_num;
    res_conditions_true(last_num < curr_num, -1);
    info.valid_num = last_num - curr_num;
    //计算write_ptr;
    if(info.read_ptr + info.valid_num >= over_num)
    {
        manager.w_ptr = info.read_ptr + info.valid_num - over_num;
    }
    else 
    {
        manager.w_ptr = info.read_ptr + info.valid_num;
    }
    manager.num = info.valid_num;   
    //更新管理区
    res_conditions_true(0x00 > update_verify(block02->first_addr + addr02, 0x00, (uint8_t*)&manager, 
        sizeof(struct log_manager_t), sizeof(struct log_manager_t) + 0x02, device02->dev), -1); 
    //搬运数据
}

int nd_log_clean(struct instance_t* instance, uint32_t index)
{
    struct block_device_t* device01 = NULL, * device02 = NULL;
    const struct driver_ops_t* driver02 = NULL;
    struct resource_block_t* block01 = NULL, * block02 = NULL;
    uint32_t addr02 = 0x00, len02 = 0x00;
    uint8_t buf[20];

    device01 = look_up_device(instance, (uint8_t)(index >> 24));//查找设备  
    res_conditions_true(device01 == NULL, -1);
    //查找块
    block01 = look_up_block(instance, device01, (uint8_t)(index >> 16));
    res_conditions_true(block01 == NULL, -1);

    device02 = look_up_device(instance, block01->relation_device);
    res_conditions_true(device02 == NULL, -1);
    block02 = look_up_block(instance, device02, block01->relation_name);
    res_conditions_true((block02 == NULL) || (block02->hook == NULL), -1);
    block02->hook(instance, (uint16_t)index, &addr02, &len02, NULL);

    memset(buf, 0x00, 20);
    driver02 = device02->dev->dev_driver_ops;
    res_conditions_true((driver02 == NULL) || (driver02->dev_write == NULL), -1);
    res_conditions_true(0x00 > driver02->dev_write(device02->dev, block02->first_addr + addr02, len02, buf), -1);

    return 1;
}

int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint32_t len = 0x00, addr = 0x00, offset = 0;

    device = look_up_device(instance, (uint8_t)(index >> 24));//查找设备
    res_conditions_true(device == NULL,-1);
    //查找块
    block = look_up_block(instance, device, (uint8_t)(index >> 16));
    res_conditions_true((block == NULL) || (block->hook == NULL), -1);
    res_conditions_true(0x00 > block->hook(instance, (uint16_t)index, &addr, &len, &offset), -1);
    addr += block->first_addr;
    res_conditions_true(addr + offset + size > block->first_addr + block->size, -1);
    //获取数据
    driver = device->dev->dev_driver_ops;
    res_conditions_true((driver == NULL) || (driver->dev_read == NULL), -1);  
    res_conditions_true(0x00 > verify_data(addr, offset, buf, size, len, device->dev), -1);

    return size;
}


int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t size)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint32_t len = 0x00, addr = 0x00, offset = 0;

    device = look_up_device(instance, (uint8_t)(index >> 24));//查找设备
    res_conditions_true(device == NULL, -1);
    block = look_up_block(instance, device, (uint8_t)(index >> 16)); //查找块
    res_conditions_true((block == NULL) || (block->hook == NULL), -1);
    res_conditions_true(0x00 > block->hook(instance, (uint16_t)index, &addr, &len, &offset), -1);
    addr += block->first_addr;
    res_conditions_true(addr + offset + size > block->first_addr + block->size, -1);

    driver = device->dev->dev_driver_ops;
    res_conditions_true((driver == NULL) || (driver->dev_write == NULL) || (driver->dev_read == NULL), -1);
    res_conditions_true(0x00 > update_verify(addr, offset, buf, size, len, device->dev), -1);

    return size;
}

int16_t nd_get_offset(struct instance_t* instance, uint32_t offset, uint8_t* buf, uint16_t len)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint32_t addr = 0x00;

    device = look_up_device(instance, (uint8_t)(offset >> 24));//查找设备
    res_conditions_true(device == NULL, -1);
    block = look_up_block(instance, device, (uint8_t)(offset >> 16));//查找块
    res_conditions_true(block == NULL, -1);
    addr = block->first_addr + (uint16_t)offset;
    res_conditions_true(addr + len > block->first_addr + block->size, -1);

    driver = device->dev->dev_driver_ops;
    res_conditions_true((driver == NULL) || (driver->dev_read == NULL), -1);
    res_conditions_true(0x00 > driver->dev_read(device->dev, addr, len, buf), -1);
    //res_conditions_true(0x00 > verify_data(buf, len), -1);

    return len;
}

int16_t nd_set_offset(struct instance_t* instance, uint32_t offset, uint8_t* buf, uint16_t len)
{
    struct block_device_t* device = NULL;
    const struct driver_ops_t* driver = NULL;
    struct resource_block_t* block = NULL;
    uint32_t addr = 0x00;

    device = look_up_device(instance, (uint8_t)(offset >> 24));//查找设备
    res_conditions_true(device == NULL, -1);
    block = look_up_block(instance, device, (uint8_t)(offset >> 16)); //查找块
    res_conditions_true(block == NULL, -1);
    addr = block->first_addr + (uint16_t)offset;
    res_conditions_true(addr + len > block->first_addr + block->size, -1);

    driver = device->dev->dev_driver_ops;
    res_conditions_true((driver == NULL) || (driver->dev_write == NULL), -1);
    //update_verify(buf, len);
    res_conditions_true(0x00 > driver->dev_write(device->dev, addr, len, buf), -1);

    return len;
}

//目前resource manger 数据操作介绍
//全部为数据块的操作，不是单个数据的操作
//需要操作数据块的单个数据，目前有两种情况：
// 情况1：当前块的分布是通过结构体来分配的，通过块的首地址和偏移来访问；这种情况需要通过结构体变量名来获取偏移值
// 情况2：当前块的分布是通过表格来分配的，通过块的收地址和偏移来访问；这种情况需要初始化的时候把数据表格注册到reource；






