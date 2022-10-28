#ifndef __RESOURCE_TOOL_H__
#define __RESOURCE_TOOL_H__

//内存管理的对齐
#define RES_ALIGN_TYPE  uint32_t

#define get_res(device_name, block_name)   (uint32_t)(((device_name) << 24) + ((block_name) << 16))

#define CIRCULAR_OFFSET_SEQUENCE  0x01
#define CIRCULAR_OFFSET_TIME    0x06

//单一设备的块链表
#pragma pack(4)
struct resource_block_t
{
    uint8_t name;
    uint8_t relation_device;  //相关联的device
    uint8_t relation_name;    //相关联的BLOCK
    uint8_t type;  
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    int16_t(*hook)(struct instance_t* instance, uint16_t id, uint32_t* addr, uint32_t* len, uint32_t* depth);
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
    uint16_t r_ptr;
    uint16_t w_ptr;
    uint16_t num;
    uint16_t unit_size;
    uint16_t max_num;
};

struct circular_info_t
{
    uint8_t mode;                             // 0- 全部访问   1-按序号搜索    2-按发生时间搜索
    uint16_t unit_size;                       //单体记录大小
    uint32_t start_mark;                      //搜索时的起始标志
    uint32_t end_mark;                        //搜索时的结束标志
    uint16_t read_ptr;                        //读的位置
    uint16_t curr_num;                        //已经读了多少条
    uint16_t valid_num;                       //符合的条数
};
struct instance_t* create_instance(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size, uint8_t resource_block_name);

int16_t nd_get_block(struct instance_t* instance, uint32_t res, uint8_t* buf,uint32_t buf_size);
int16_t nd_set_block(struct instance_t* instance, uint32_t res, uint8_t* buf,uint32_t buf_size);

int16_t get_block_frist_addr(struct instance_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr);


uint32_t* add_inside_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t inside_block_name, uint32_t block_size, uint32_t offset);
void* nd_malloc(struct instance_t* instance, uint8_t block_name, uint32_t size);
void* nd_calloc(struct instance_t* instance, uint8_t block_name, uint32_t size);
uint32_t* add_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size);
int8_t add_device(struct instance_t* instance, uint8_t device_name, const struct dev_info_t* dev, uint32_t sector);

struct resource_map_t* register_map(struct instance_t* instance, uint8_t map_name,
    int (*fun)(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx, uint16_t mode));
uint32_t* register_log(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(struct instance_t* instance, uint16_t id, uint32_t* addr, uint32_t* len, uint32_t* depth));
uint32_t* register_parameter(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(struct instance_t* instance, uint16_t id, uint32_t *addr, uint32_t* len, uint32_t *offset));




int nd_log_clean(struct instance_t* instance, uint32_t index);
int16_t nd_log_append(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t unit_size);
int16_t nd_log_append_revise(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t offset, uint16_t unit_size);
int16_t nd_log_output(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size, struct circular_info_t* info);



int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t size);
int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t size);

int16_t nd_get_offset(struct instance_t* instance, uint32_t offset, uint8_t* buf, uint16_t len);
int16_t nd_set_offset(struct instance_t* instance, uint32_t offset, uint8_t* buf, uint16_t len);

int16_t nd_size(struct instance_t* instance, uint8_t *buf, uint32_t id);


int16_t nd_get_index_e2p(struct instance_t* instance, uint8_t* buf, uint8_t block_name,  uint16_t id, uint16_t len);
int16_t nd_set_index_e2p(struct instance_t* instance, uint8_t* buf, uint8_t block_name,  uint16_t id, uint16_t len);


uint32_t do_checksum(uint8_t *buf, uint16_t len);
#endif
