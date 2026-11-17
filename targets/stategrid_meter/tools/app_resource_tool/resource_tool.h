#ifndef __RESOURCE_TOOL_H__
#define __RESOURCE_TOOL_H__
#include <stdint.h>
#include "application/ports/resource_access.h"
#include "infra/kernel/list.h"
#include "targets/stategrid_meter/app/stategrid_context.h"

enum resource_map_698_e
{
    RESOURCE_MAP_GET = 0,
    RESOURCE_MAP_SET = 1,
    RESOURCE_MAP_ACTION = 2,
};

//内存管理的对齐
#define RES_ALIGN_TYPE  uint32_t

//单一设备的块链表
#pragma pack(push, 4)
struct resource_block_t
{
    uint8_t name;
    uint8_t relation_device;  //相关联的device
    uint8_t relation_name;    //相关联的BLOCK
    uint8_t type;  
    uint32_t first_addr;
    uint32_t size;
    uint32_t used;
    int16_t(*hook)(stategrid_context_t *instance, uint16_t id, uint32_t* addr, uint32_t* len, uint32_t* depth);
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
#pragma pack(pop)

struct resource_map_t
{
    uint8_t map_name;
    int (*fun)(stategrid_context_t *instance, uint32_t id, uint8_t *buf,
               uint16_t buf_size, void *ctx, uint16_t mode);
    struct list_head sibling;
};

stategrid_context_t *stategrid_context_create(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size, uint8_t resource_block_name);

int16_t nd_get_block(stategrid_context_t *instance, uint32_t res, uint8_t* buf,uint32_t buf_size);
int16_t nd_set_block(stategrid_context_t *instance, uint32_t res, uint8_t* buf,uint32_t buf_size);

int16_t get_block_frist_addr(stategrid_context_t *instance, uint8_t device_name, uint8_t block_name, uint32_t *addr);


uint32_t* add_inside_block(stategrid_context_t *instance, uint8_t device_name, uint8_t block_name, uint8_t inside_block_name, uint32_t block_size, uint32_t offset);
void* nd_malloc(stategrid_context_t *instance, uint8_t block_name, uint32_t size);
void* nd_calloc(stategrid_context_t *instance, uint8_t block_name, uint32_t size);
uint32_t* add_block(stategrid_context_t *instance, uint8_t device_name, uint8_t block_name, uint32_t block_size);
int8_t add_device(stategrid_context_t *instance, uint8_t device_name, uint32_t sector);

struct resource_map_t* register_map(stategrid_context_t *instance, uint8_t map_name,
    int (*fun)(stategrid_context_t *instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx, uint16_t mode));
uint32_t* register_log(stategrid_context_t *instance, uint8_t device_name, uint8_t block_name, uint8_t relation_device, uint8_t relation_name, uint32_t block_size,
    int16_t(*fun)(stategrid_context_t *instance, uint16_t id, uint32_t* addr, uint32_t* len, uint32_t* depth));
uint32_t* register_parameter(stategrid_context_t *instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(stategrid_context_t *instance, uint16_t id, uint32_t *addr, uint32_t* len, uint32_t *offset));

int16_t nd_size(void *instance, uint8_t *buf, uint32_t id);

void resource_tool_bind(stategrid_context_t *ctx);

#endif
