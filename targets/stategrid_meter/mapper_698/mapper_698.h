#ifndef ND_TARGET_STATEGRID_MAPPER_698_MAPPER_698_H
#define ND_TARGET_STATEGRID_MAPPER_698_MAPPER_698_H

#include <stdint.h>


int GET_0_Energy(void *instance, uint32_t id, uint8_t *buf,
                 uint8_t mode, void *ctx);
int GET_1_Demand(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_2_Variable(void *instance, uint32_t id, uint8_t *buf,
                   uint8_t mode, void *ctx);
int SET_2_Variable(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_3_Event(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int SET_3_Event(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_4_Parameter(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int SET_4_Parameter(void *instance, uint32_t id, uint8_t *buf,
                    uint16_t buf_size, void *ctx);
int ACTION_2_Variable(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int ACTION_3_Event(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int ACTION_4_Parameter(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int ACTION_5_Frozen(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int ACTION_8_Parameter(void *instance, uint32_t id, uint8_t *buf, void *pctx);
int ACTION_F_Other(void *instance, uint32_t id, uint8_t *buf,
                   uint16_t buf_size, void *ctx);
int GET_5_Frozen(void *instance, uint32_t oad, uint8_t *buf, void *ctx);
int SET_5_Frozen(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_6_Collection(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_7_Aggregate(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_8_Control(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int SET_8_Control(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_e_Monitor(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int GET_f_Other(void *instance, uint32_t id, uint8_t *buf, void *ctx);
int SET_f_Other(void *instance, uint32_t id, uint8_t *buf, void *ctx);
uint8_t get_current_plc_baud(void *instance);

#endif
