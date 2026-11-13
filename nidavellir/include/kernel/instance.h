#ifndef ND_KERNEL_INSTANCE_H
#define ND_KERNEL_INSTANCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "types.h"
#include "list.h"
#include "fifo.h"
#include "resource_ids.h"
#include "mlog.h"
#include "pipeline.h"

struct recipe_t;
struct pipeline_t;

#ifdef __DEBUG_ASSERT__
#include <assert.h>
#define n_assert(test) ((test) ? (void)0 : (__CALL(test), _STEPPOINT))
#else
#define n_assert(test)
#endif

#define upgrade_setsize 40

#pragma pack(4)
struct resource_map_t
{
    uint8_t map_name;
    int (*fun)(struct instance_t *instance, uint32_t id, uint8_t *buf,
               uint16_t buf_size, void *ctx, uint16_t mode);
    struct list_head sibling;
};
#pragma pack()


struct upgrade_info_t
{
    uint16_t magic_num;
    uint16_t file_verification;
    uint16_t frm_size;
    uint16_t frm_num;
    uint32_t file_size;

    uint16_t cur_verification;
    uint16_t frm_no_last;
    uint32_t flash_addr;
    uint8_t type;
    uint8_t status;
    uint8_t remained_1;
    uint8_t remained_2;
    uint8_t bits[upgrade_setsize];
};

#pragma pack(4)
struct instance_t
{
    struct list_head app_list;
    struct list_head dev_list;
    struct list_head map_list;
    uint8_t ram_device;
    uint8_t resource_block;

    uint8_t meter_type;
    uint32_t ms;
    uint8_t *apdu_cache;
    struct recipe_t *recipe;
    struct tm time;
    struct upgrade_info_t upgrade_info;
    struct fifo_t pipe_rtc;
    struct fifo_t pipe_key;
    struct fifo_t pipe_uart[4];
    uint8_t ac_power_statue;
    uint8_t reset_flag;
    uint8_t need_data_recovery;
    uint32_t ac_power_off_sec_cnt;
};
#pragma pack()

struct instance_t *create_nidavellir(struct recipe_t *recipe);
int nidavellir_loop(struct instance_t *instance);
int nidavellir_n_loop(struct instance_t **instances, size_t n);
int nidavellir_run(struct instance_t *instance);
int nidavellir_run_baremetal(struct instance_t *instance);
int nidavellir_loop_n(struct instance_t *instance, int n);
int nidavellir_loop_n_with_rtc_running(struct instance_t *instance, int n);
struct pipeline_t *get_pipeline(struct instance_t *instance, pid_t id);
void *get_pipeline_ctx(struct instance_t *instance, pid_t pipeline_id);
uint16_t nd_get_crc16(uint8_t *buf, size_t len);
void irq_transform_to_instance(struct instance_t *instance);
int16_t nd_get(struct instance_t *instance, uint32_t id, uint8_t *buf,
               uint16_t buf_size, void *ctx);
int fr_uart_timeout(struct instance_t *instance, struct pipeline_t *pipeline);

#endif
