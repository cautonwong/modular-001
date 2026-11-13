#ifndef ND_APPS_RESOURCE_ACCESS_H
#define ND_APPS_RESOURCE_ACCESS_H

#include <stdint.h>

#define get_res(device_name, block_name) (uint32_t)(((device_name) << 24) + ((block_name) << 16))

#define CIRCULAR_OFFSET_SEQUENCE 0x01
#define CIRCULAR_OFFSET_TIME 0x06

struct instance_t;

struct circular_info_t
{
    uint8_t mode;
    uint16_t unit_size;
    uint32_t start_mark;
    uint32_t end_mark;
    uint16_t read_ptr;
    uint16_t curr_num;
    uint16_t valid_num;
};

int16_t nd_log_output(struct instance_t *instance, uint32_t index,
                      uint8_t *buf, uint16_t buf_size,
                      struct circular_info_t *info);

int16_t nd_get_index(struct instance_t *instance, uint32_t index,
                     uint8_t *buf, uint16_t size);
int16_t nd_set_index(struct instance_t *instance, uint32_t index,
                     uint8_t *buf, uint16_t size);
int16_t nd_get_offset(struct instance_t *instance, uint32_t offset,
                      uint8_t *buf, uint16_t len);
int16_t nd_set_offset(struct instance_t *instance, uint32_t offset,
                      uint8_t *buf, uint16_t len);
int16_t nd_get_index_e2p(struct instance_t *instance, uint8_t *buf,
                         uint8_t block_name, uint16_t id, uint16_t len);
int16_t nd_set_index_e2p(struct instance_t *instance, uint8_t *buf,
                         uint8_t block_name, uint16_t id, uint16_t len);
int16_t nd_set_offset_flash(struct instance_t *instance, uint8_t blockname,
                            uint32_t addr_offset, uint8_t *buf, uint16_t len,
                            uint8_t check_erase);
int16_t nd_get_offset_flash(struct instance_t *instance, uint8_t blockname,
                            uint32_t addr_offset, uint8_t *buf, uint16_t len);

#endif
