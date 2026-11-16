#define RESOURCE_MAP_GET 0
#define RESOURCE_MAP_SET 1
#define RESOURCE_MAP_ACTION 2

#ifndef ND_TARGET_STATEGRID_MAPPER_698_RESOURCE_MAP_H
#define ND_TARGET_STATEGRID_MAPPER_698_RESOURCE_MAP_H

#include <stdint.h>

struct message_t;
struct nd_dl698_msg;

int16_t supply_parameter_info(void *instance, uint16_t id,
                              uint32_t *addr, uint32_t *size, uint32_t *offset);
int16_t supply_parameter_1_info(void *instance, uint16_t id,
                                uint32_t *addr, uint32_t *size,
                                uint32_t *offset);
int16_t supply_event_info(void *instance, uint16_t event_flag,
                          uint32_t *addr, uint32_t *size, uint32_t *depth);
int16_t supply_event_flash_1_info(void *instance,
                                  uint16_t event_flag, uint32_t *addr,
                                  uint32_t *size, uint32_t *depth);
int16_t supply_event_flash_2_info(void *instance,
                                  uint16_t event_flag, uint32_t *addr,
                                  uint32_t *size, uint32_t *depth);
int resource_map_698(void *instance, uint32_t id, uint8_t *buf,
                     uint16_t buf_size, void *ctx, uint16_t mode);
int check_metertype_OI_valid(void *instance, uint16_t OI);
int general_set_4_para(void *instance, uint32_t id, uint8_t *buf);
void fr_dl698_power_on(void *instance,
                       void *pipeline);
void fr_dl698_sec_task(void *instance,
                       void *pipeline);
int fr_set_event_report_flag_mode(void *instance, uint32_t oad,
                                  uint8_t *buf, uint8_t mode);
int get_event_report_flag_mode(void *instance, uint32_t oad,
                               uint8_t *buf, uint8_t mode);
void fr_698_MeterWr_program_judge(void *instance,
                                  void *pipeline,
                                  struct nd_dl698_msg *mesg);

#endif
