#ifndef __RESOURCE_TOOL_H__
#define __RESOURCE_TOOL_H__

//内存管理的对齐
#define RES_ALIGN_TYPE  uint32_t
//单一设备的块链表
#pragma pack(4)
struct resource_block_t
{
    uint8_t name;
    uint8_t relation_device;  //相关联的device
    uint8_t relation_name;    //相关联的BLOCK
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    int16_t(*hook)(uint16_t id, uint32_t* addr, uint16_t* len, uint16_t* depth);
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
    const struct dev_info_t* dev;
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
    uint32_t start_mark;                      //搜索时的起始标志
    uint32_t end_mark;                        //搜索时的结束标志
    uint16_t read_ptr;                        //读的位置
    uint16_t valid_num;                       //符合的条数
};

int16_t nd_get_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size);
int16_t nd_set_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t* buf,uint32_t buf_size);
int16_t get_block_frist_addr(struct instance_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr);

#endif