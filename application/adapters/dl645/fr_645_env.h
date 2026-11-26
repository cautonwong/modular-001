#ifndef ND_DL645_ENV_H
#define ND_DL645_ENV_H

#include <stdint.h>

#include "application/ports/record_storage.h"
#include "application/ports/resource_access.h"

int nd_fr645_mbase_valid(void *instance);
uint8_t *nd_fr645_tsa(void *instance);
uint16_t *nd_fr645_metertype(void *instance);
uint32_t *nd_fr645_infrared_auth_timeout(void *instance);
uint8_t *nd_fr645_factor_mode(void *instance);
uint8_t *nd_fr645_pass02_err_cnt(void *instance);
uint8_t *nd_fr645_pass04_err_cnt(void *instance);
uint16_t *nd_fr645_backlight_tmr(void *instance);
uint8_t *nd_fr645_lcd_autodis_list_num(void *instance);
uint16_t *nd_fr645_lcd_autodis_keep_sec(void *instance);
uint8_t *nd_fr645_commdisp_ir485(void *instance);
uint8_t *nd_fr645_commdisp_plc(void *instance);
uint8_t *nd_fr645_handler_code(void *instance);
uint8_t *nd_fr645_apdu_cache(void *instance);
uint32_t nd_fr645_password_index(void *instance, uint8_t level);
const nd_record_storage_port_t *nd_fr645_record_storage(void *instance);
const nd_resource_port_t *nd_fr645_resource_access(void *instance);

uint32_t nd_fr645_pipeline_id(const void *pipeline);
void *nd_fr645_pipeline_ctx(const void *pipeline);
void *nd_fr645_pipeline_in_queue(const void *pipeline);
void *nd_fr645_pipeline_out_queue(const void *pipeline);

#endif
