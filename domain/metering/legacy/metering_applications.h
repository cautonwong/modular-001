#ifndef ND_DOMAIN_METERING_LEGACY_APPLICATIONS_H
#define ND_DOMAIN_METERING_LEGACY_APPLICATIONS_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct mbase_control_block_t;

void Electrical_measurement_poweron(struct instance_t *instance,
                                    struct pipeline_t *pipeline);
void Electrical_measurement(struct instance_t *instance,
                            struct pipeline_t *pipeline);
void Energy_Pluse_Count(struct instance_t *instance,
                        struct pipeline_t *pipeline);
void Energy_Meter(struct instance_t *instance, struct pipeline_t *pipeline);
void Energy_Meter_PowerON(struct instance_t *instance,
                          struct pipeline_t *pipeline);
void Energy_Meter_PowerOFF(struct instance_t *instance,
                           struct pipeline_t *pipeline);
int64_t Get_Cal_Active_Energy_T(struct mbase_control_block_t *mbase_cb,
                                uint8_t FeeNo, uint8_t type);
int64_t Get_Active_Energy_TABC(struct mbase_control_block_t *mbase_cb,
                               uint8_t phase, uint8_t FeeNo, uint8_t type,
                               uint8_t pn);
void Energy_clr(struct instance_t *instance);
void Meter_Run_State(struct instance_t *instance, struct pipeline_t *pipeline);
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
