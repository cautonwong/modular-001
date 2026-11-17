#ifndef ND_DL698_ENV_H
#define ND_DL698_ENV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct tm;
struct nd_dl698_msg;

/* Legacy services the adapter consumes; implemented by kernel or target. */
void ndv_memcopy(void *tb, void *sb, uint16_t length);
uint8_t mem_compare(void *data1, void *data2, uint16_t length);
void mem_swap_cpy(void *dest, void *src, unsigned short length);
uint32_t buf_to_uint32_msb(uint8_t *buf);
uint16_t buf_to_uint16_msb(uint8_t *buf);
uint16_t buf_to_uint16_lsb(uint8_t *buf);
uint8_t uint16_to_buf_msb(uint16_t data, uint8_t *buf);
uint8_t uint32_to_buf_msb(uint32_t data, uint8_t *buf);
uint32_t rtc_mktime(struct tm *t);
void rtc_localtime(uint32_t time, struct tm *t);

#ifndef n_assert
#define n_assert(x) ((void)0)
void *nd_instance_upgrade_info(void *instance);

#endif
#ifndef log_info
#define log_info(...) ((void)0)
void *nd_instance_upgrade_info(void *instance);

#endif
#ifndef log_hex
#define log_hex(...) ((void)0)
void *nd_instance_upgrade_info(void *instance);

#endif

int update_plc_baut_tick(void *instance,
                         void *pipeline,
                         struct nd_dl698_msg *mesg);
void app_handle_mac_suspend(void *instance, uint32_t oad);
void fr_698_MeterWr_program_judge(void *instance,
                                  void *pipeline,
                                  struct nd_dl698_msg *mesg);
int16_t nd_size(void *instance, uint8_t *buf, uint32_t id);

struct tm *nd_fr698_time(void *instance);
uint8_t *nd_fr698_apdu_cache(void *instance);
void *nd_fr698_plc_ctx(void *instance);
uint8_t *nd_fr698_power_state(void *instance);
uint8_t *nd_fr698_reset_flag(void *instance);
uint32_t nd_fr698_ms(void *instance);
uint32_t *nd_fr698_ac_power_off_sec_cnt(void *instance);

void *nd_fr698_pipeline_ctx(const void *pipeline);
uint32_t nd_fr698_pipeline_id(const void *pipeline);
void *nd_fr698_pipeline_in_queue(const void *pipeline);
void *nd_fr698_pipeline_out_queue(const void *pipeline);

void *nd_instance_upgrade_info(void *instance);

#endif
