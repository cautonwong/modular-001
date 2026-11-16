#ifndef ND_TARGET_STATEGRID_METERING_APPLICATIONS_H
#define ND_TARGET_STATEGRID_METERING_APPLICATIONS_H

#include <stdint.h>

struct mbase_control_block_t;
struct factor_adj_meter_t;

void Electrical_measurement_poweron(void *instance,
                                    void *pipeline);
void Electrical_measurement(void *instance,
                            void *pipeline);
void Energy_Pluse_Count(void *instance,
                        void *pipeline);
void Energy_Meter(void *instance, void *pipeline);
void Energy_Meter_PowerON(void *instance,
                          void *pipeline);
void Energy_Meter_PowerOFF(void *instance,
                           void *pipeline);
int64_t Get_Cal_Active_Energy_T(void *instance,
                                uint8_t FeeNo, uint8_t type);
int64_t Get_Active_Energy_TABC(void *instance,
                               uint8_t phase, uint8_t FeeNo, uint8_t type,
                               uint8_t pn);
void Energy_clr(void *instance);
void Meter_Run_State(void *instance, void *pipeline);
int16_t Check_para_Pro(void *instance);
void emu_calibration_clear(void *instance);
void meterchip_adj_self(void *instance,
                        struct factor_adj_meter_t *adj_data);
int rn8209c_set(void *instance, uint8_t *p_rx);
uint8_t rn8209c_get(void *instance, uint8_t *p_rx, uint8_t *p_tx);
unsigned char IxRMSOS_Adjust(void *instance, unsigned char CH);
unsigned char ft_adj_meter(void *instance, uint8_t *p_rx);
void Energy_Set(void *instance, uint32_t basedata, uint8_t dir);
uint32_t do_checksum(uint8_t *buf, uint16_t len);
uint32_t do_block_static_checksum(void *instance);
void Poffset_adj_oneself(void *instance);
void Energy_Pluse_Add(void);
extern uint32_t over_Imax;
uint8_t scond_execute_for_demand(struct mbase_control_block_t *mbase_block);
uint8_t minute_execute_for_demand(struct mbase_control_block_t *mbase_block);
int8_t add_demand_action(struct mbase_control_block_t *mbase_block,
                         uint32_t *import, uint32_t output, uint8_t max_slide,
                         uint8_t mode);

#define MAX_VOLTAGE 4000
#define MIN_VOLTAGE 500
#define UN_VOLTAGE 2200
#define MAX_CURRENT 120000
#define MIN_CURRENT 17
#define AS_Un 2200
#define AS_Ib 5000
#define AS_P 11000

#endif
