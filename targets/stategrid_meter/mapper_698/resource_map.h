#ifndef ND_TARGET_STATEGRID_MAPPER_698_RESOURCE_MAP_H
#define ND_TARGET_STATEGRID_MAPPER_698_RESOURCE_MAP_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct message_t;
struct nd_dl698_msg;

int16_t supply_parameter_info(struct instance_t *instance, uint16_t id,
                              uint32_t *addr, uint32_t *size, uint32_t *offset);
int16_t supply_parameter_1_info(struct instance_t *instance, uint16_t id,
                                uint32_t *addr, uint32_t *size,
                                uint32_t *offset);
int16_t supply_event_info(struct instance_t *instance, uint16_t event_flag,
                          uint32_t *addr, uint32_t *size, uint32_t *depth);
int16_t supply_event_flash_1_info(struct instance_t *instance,
                                  uint16_t event_flag, uint32_t *addr,
                                  uint32_t *size, uint32_t *depth);
int16_t supply_event_flash_2_info(struct instance_t *instance,
                                  uint16_t event_flag, uint32_t *addr,
                                  uint32_t *size, uint32_t *depth);
int resource_map_698(struct instance_t *instance, uint32_t id, uint8_t *buf,
                     uint16_t buf_size, void *ctx, uint16_t mode);
int check_metertype_OI_valid(struct instance_t *instance, uint16_t OI);
int general_set_4_para(struct instance_t *instance, uint32_t id, uint8_t *buf);
void fr_dl698_power_on(struct instance_t *instance,
                       struct pipeline_t *pipeline);
void fr_dl698_sec_task(struct instance_t *instance,
                       struct pipeline_t *pipeline);
int fr_set_event_report_flag_mode(struct instance_t *instance, uint32_t oad,
                                  uint8_t *buf, uint8_t mode);
int get_event_report_flag_mode(struct instance_t *instance, uint32_t oad,
                               uint8_t *buf, uint8_t mode);
void fr_698_MeterWr_program_judge(struct instance_t *instance,
                                  struct pipeline_t *pipeline,
                                  struct nd_dl698_msg *mesg);

#endif
