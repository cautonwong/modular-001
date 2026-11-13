#ifndef ND_DL645_ENV_H
#define ND_DL645_ENV_H

#include <stdint.h>

struct instance_t;

int nd_fr645_mbase_valid(struct instance_t *instance);
uint8_t *nd_fr645_tsa(struct instance_t *instance);
uint16_t *nd_fr645_metertype(struct instance_t *instance);
uint32_t *nd_fr645_infrared_auth_timeout(struct instance_t *instance);
uint8_t *nd_fr645_factor_mode(struct instance_t *instance);
uint8_t *nd_fr645_pass02_err_cnt(struct instance_t *instance);
uint8_t *nd_fr645_pass04_err_cnt(struct instance_t *instance);
uint16_t *nd_fr645_backlight_tmr(struct instance_t *instance);
uint8_t *nd_fr645_lcd_autodis_list_num(struct instance_t *instance);
uint16_t *nd_fr645_lcd_autodis_keep_sec(struct instance_t *instance);
uint8_t *nd_fr645_commdisp_ir485(struct instance_t *instance);
uint8_t *nd_fr645_commdisp_plc(struct instance_t *instance);
uint8_t *nd_fr645_handler_code(struct instance_t *instance);
uint8_t *nd_fr645_apdu_cache(struct instance_t *instance);
uint32_t nd_fr645_password_index(struct instance_t *instance, uint8_t level);

uint32_t nd_fr645_pipeline_id(const void *pipeline);
void *nd_fr645_pipeline_ctx(const void *pipeline);
void *nd_fr645_pipeline_in_queue(const void *pipeline);
void *nd_fr645_pipeline_out_queue(const void *pipeline);

#endif
