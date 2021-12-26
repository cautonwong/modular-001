#include "nidavellir.h"
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include "applications.h"
#include "resource_tool.h"

int generate_data(uint8_t* data, size_t n, size_t len, size_t type)
{
    time_t t;
    size_t nn = len;
    if (len > n)
    {
        nn = n;
    }
    srand((unsigned)time(&t));
    for (size_t i = 0; i < nn; i++)
    {
        data[i] = rand() % 0xff;
    }
    return 0;
}
#if 0
int eeprom_read_chk(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    uint8_t* p = mock_ptr_type(void *);
    int r = mock_type(int);
    memcpy(src, p, r);
    return r;
}

int eeprom_write_chk(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src)
{
    check_expected(addr);
    check_expected(src);
    check_expected(len);
    return len;
}

int eeprom_control(const struct dev_info_t* dev, uint8_t ctrl, void* ctrl_parameter)
{
    assert(0);
    return 0;
}

const struct driver_ops_t  eeprom_driver =
{
    NULL,
    eeprom_control,
    eeprom_read_chk,
    eeprom_write_chk,
};
#endif

extern const struct driver_ops_t  flash_driver;
extern const struct driver_ops_t  eeprom_driver;
const struct dev_info_t eeprom_dev = 
{
    0x00,0x01,0xFFFF,0x00,0x00, NULL, &eeprom_driver, NULL, NULL, NULL, NULL,".//e2prom.dat"
};
const struct dev_info_t nor_flash_dev = 
{
    0x01,0x01,0xFFFF,0x00,0x00, NULL, &flash_driver, NULL, NULL, NULL, NULL,".//flash.dat"
};

const struct filter_t fr_resource_manager[] =
{
    {0x01,NULL,0,13},
};

struct pipeline_t pl_resource_manager =
{
    pid_resource_manager,
    "app_resource_manager",
    NULL,
    NULL,
    NULL,
    NULL,
    sizeof(fr_resource_manager) / sizeof(fr_resource_manager[0]),
    fr_resource_manager,
};

struct pipeline_t* pipelines[] =
{
    &pl_resource_manager,
};

uint8_t nd_memory[20 * 1024];
int test_resource_setup(void** state)
{
    struct instance_t* instance = NULL;
    struct recipe_t *recipe = NULL;

    instance = create_instance(RES_DEV_RAM, (uint32_t)&nd_memory, 20 * 1024, 2 * 1024, resource_block_name);//用资源描述创建instance
    if (instance == NULL)
    {
        return -1;
    }

    instance->recipe = nd_malloc(instance, instance->resource_block, sizeof(struct recipe_t));
    recipe = instance->recipe;
    if (recipe == NULL)
    {
        return -1;
    }
    recipe->pipelines = &pipelines[0];
    recipe->pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe->name = 0;

    //初始化pl_resource_manager
    pl_resource_manager.in_queue = nd_malloc(instance, instance->resource_block, sizeof(struct fifo_t));
    fifoBuf_init(pl_resource_manager.in_queue, nd_malloc(instance, instance->resource_block, 256), 256);

    //初始化时间
    instance->ms = 0;
    struct tm time =
    {
        .tm_year = 2021 - 1900,   /* years are counted from 1900 */
        .tm_mon = 3,                      /* 0 = January, 11 = December */
        .tm_mday = 30,
        .tm_hour = 23,
        .tm_min = 59,
        .tm_sec = 57
    };
    memcpy(&instance->time, &time, sizeof(struct tm));

    ///////////////////////////////申请一个cache的BLOCK = 02/////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_cache, 500))
    {
        return -1;
    }
    //指向cache space
    instance->cache = nd_malloc(instance, control_block_cache, 256);
    instance->cache2 = nd_malloc(instance, control_block_cache, sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    
    instance->event_flags = nd_malloc(instance,control_block_cache,event_flags_group_size);
    memset(instance->event_flags, 0, event_flags_group_size);
    instance->event_group_size = event_flags_group_size;//初始化软时钟

    uint8_t* buf = nd_malloc(instance, control_block_cache, 4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);
#if 0
    ////////////////////////////申请一个mbase应用的BLOCK = 04/////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, mbase_control_block_name, 8 * 1024))
    {
        return -1;
    }
    //mbase分三个区
    //mbase_block_static
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start);
    uint32_t static_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_static, mbase_block_static_size);
    //mbase_block_cache
    size_t mbase_block_cache_size = offsetof(struct mbase_control_block_t, cache_section_end) - offsetof(struct mbase_control_block_t, cache_section_start);
    uint32_t cache_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_cache, mbase_block_cache_size);
    //mbase_block_persistent
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_end) - offsetof(struct mbase_control_block_t, persistent_section_start);
    uint32_t persistent_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_persistent, mbase_block_persistent_size);
#endif
    //注册EPROM  
    add_device(instance, RES_DEV_EEPROM, &eeprom_dev,0x00);
    //注册FLASH
    add_device(instance, RES_DEV_FLASH, &nor_flash_dev,4096);
#if 0
    //申请一块参数保存区的EPROM区域
    add_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, mbase_block_static_size);
    uint32_t addr = 0x00;
    //上电恢复参数
    get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
    nd_get_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  
#endif // 0

    *state = instance;
    return 0;
}

#include "log.h"

extern void test_add_device(void** state);
extern void test_add_block_one_byte(void** state);
extern void test_add_block_same_block_name(void** state);
extern void test_add_block_500_byte(void** state);
extern void test_add_block_10K_byte(void** state);
extern void test_add_block_out_of_gauge(void** state);
extern void test_nd_get(void** state);
extern void test_nd_get_index(void** state);
extern void test_nd_get_index_same(void** state);
extern void test_nd_get_offset(void** state);
extern void test_nd_log_output_search_all(void** state);
extern void test_nd_log_output_search_all_06(void** state);
extern void test_nd_log_output_search_all_full(void** state);
extern void test_nd_log_output_search_all_over(void** state);
extern void test_nd_log_output_search_index_06_03(void** state);
extern void test_nd_log_output_search_index_06_04(void** state);
extern void test_nd_log_output_search_index_06_02(void** state);
extern void test_nd_log_output_search_index_06_06(void** state);
extern void test_nd_log_output_search_index_10_05(void** state);
extern void test_nd_log_output_search_index_10_01(void** state);
extern void test_nd_log_output_search_index_10_10(void** state);
extern void test_nd_log_output_search_index_over_05(void** state);
extern void test_nd_log_output_search_time_06_03(void** state);
extern void test_nd_log_output_search_time_06_04(void** state);
extern void test_nd_log_output_search_time_06_02(void** state);
extern void test_nd_log_output_search_time_06_06(void** state);
extern void test_nd_log_output_search_time_10_05(void** state);
extern void test_nd_log_output_search_time_10_01(void** state);
extern void test_nd_log_output_search_time_10_10(void** state);
extern void test_nd_log_output_search_time_over_05(void** state);
extern void test_nd_log_output_flash_search_all(void** state);
extern void test_nd_log_output_flash_search_all_100(void** state);
extern void test_nd_log_output_flash_search_all_400(void** state);
extern void test_nd_log_output_flash_search_all_426(void** state);
extern void test_nd_log_output_flash_search_all_500(void** state);
extern void test_nd_log_output_flash_search_index_100_30(void** state);
extern void test_nd_log_output_flash_search_index_100_40(void** state);
extern void test_nd_log_output_flash_search_index_100_18(void** state);
extern void test_nd_log_output_flash_search_index_100_100(void** state);
extern void test_nd_log_output_flash_search_index_400_100(void** state);
extern void test_nd_log_output_flash_search_index_426_100(void** state);
extern void test_nd_log_output_flash_search_index_500_100(void** state);
extern void test_nd_log_output_flash_search_time_100_30(void** state);
extern void test_nd_log_output_flash_search_time_100_40(void** state);
extern void test_nd_log_output_flash_search_time_100_18(void** state);
extern void test_nd_log_output_flash_search_time_100_100(void** state);
extern void test_nd_log_output_flash_search_time_400_100(void** state);
extern void test_nd_log_output_flash_search_time_426_100(void** state);
extern void test_nd_log_output_flash_search_time_500_100(void** state);

int main(int argc, char** argv)
{
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
    nor_flash_dev.dev_driver_ops->dev_initialize(&nor_flash_dev);

    const struct CMUnitTest tests[] = {
        //测试设备增加，和查找
        cmocka_unit_test(test_add_device),     
        cmocka_unit_test(test_add_block_one_byte),     
        cmocka_unit_test(test_add_block_same_block_name),     
        cmocka_unit_test(test_add_block_500_byte),     
        cmocka_unit_test(test_add_block_10K_byte),     
        cmocka_unit_test(test_add_block_out_of_gauge),
        //测试nd_get 和 nd_set
        cmocka_unit_test(test_nd_get),
        cmocka_unit_test(test_nd_get_index),
        cmocka_unit_test(test_nd_get_offset),  
        cmocka_unit_test(test_nd_get_index_same),       
        //测试循环存储 EPROM 的搜索
#if 1
        cmocka_unit_test(test_nd_log_output_search_all),
        cmocka_unit_test(test_nd_log_output_search_all_06),
        cmocka_unit_test(test_nd_log_output_search_all_full),
        cmocka_unit_test(test_nd_log_output_search_all_over),
        cmocka_unit_test(test_nd_log_output_search_index_06_03),
        cmocka_unit_test(test_nd_log_output_search_index_06_04),
        cmocka_unit_test(test_nd_log_output_search_index_06_02),
        cmocka_unit_test(test_nd_log_output_search_index_06_06),
        cmocka_unit_test(test_nd_log_output_search_index_10_05),
        cmocka_unit_test(test_nd_log_output_search_index_10_01),
        cmocka_unit_test(test_nd_log_output_search_index_10_10),
        cmocka_unit_test(test_nd_log_output_search_index_over_05),
        cmocka_unit_test(test_nd_log_output_search_time_06_03),
        cmocka_unit_test(test_nd_log_output_search_time_06_04),
        cmocka_unit_test(test_nd_log_output_search_time_06_02),
        cmocka_unit_test(test_nd_log_output_search_time_06_06),
        cmocka_unit_test(test_nd_log_output_search_time_10_05),
        cmocka_unit_test(test_nd_log_output_search_time_10_01),
        cmocka_unit_test(test_nd_log_output_search_time_10_10),
        cmocka_unit_test(test_nd_log_output_search_time_over_05),
        //测试循环存储 FLASH 的搜索
        cmocka_unit_test(test_nd_log_output_flash_search_all),
        cmocka_unit_test(test_nd_log_output_flash_search_all_100),
        cmocka_unit_test(test_nd_log_output_flash_search_all_400),
        cmocka_unit_test(test_nd_log_output_flash_search_all_426),
        cmocka_unit_test(test_nd_log_output_flash_search_all_500),
        cmocka_unit_test(test_nd_log_output_flash_search_index_100_30),
        cmocka_unit_test(test_nd_log_output_flash_search_index_100_40),
        cmocka_unit_test(test_nd_log_output_flash_search_index_100_18),
        cmocka_unit_test(test_nd_log_output_flash_search_index_100_100),
        cmocka_unit_test(test_nd_log_output_flash_search_index_400_100),
        cmocka_unit_test(test_nd_log_output_flash_search_index_426_100),
        cmocka_unit_test(test_nd_log_output_flash_search_index_500_100),
        cmocka_unit_test(test_nd_log_output_flash_search_time_100_30),
        cmocka_unit_test(test_nd_log_output_flash_search_time_100_40),
        cmocka_unit_test(test_nd_log_output_flash_search_time_100_18),
        cmocka_unit_test(test_nd_log_output_flash_search_time_100_100),
        cmocka_unit_test(test_nd_log_output_flash_search_time_400_100),
        cmocka_unit_test(test_nd_log_output_flash_search_time_426_100),
        cmocka_unit_test(test_nd_log_output_flash_search_time_500_100),
#endif // 0
    };

    return cmocka_run_group_tests(tests, test_resource_setup, NULL);

}

