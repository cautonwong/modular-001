#include "nidavellir.h"
#include <setjmp.h>
#include "cmocka.h"
#include "resource_tool.h"
#include "test_resource.h"


#define test_OverCurrentL2 0xC1
#define test_OverCurrentEndL2 0xC2

const struct event_manager_t  flash_event_manager_item[] =
{
	{ test_OverCurrentL2,       sizeof(struct log_manager_t)},// 管理区
};

int16_t test_flash_event_manager_info(uint16_t id, uint32_t* addr, uint16_t* size)
{
	uint8_t i, k;

	for (i = 0; i < sizeof(flash_event_manager_item) / sizeof(flash_event_manager_item[0]); i++)
	{
		if (flash_event_manager_item[i].event_name == id)
		{
			if (addr != NULL)
			{
				*addr = 0x00;
				for (k = 0x00; k < i; k++)
				{
					*addr += flash_event_manager_item[k].len;
				}
			}

			if (size != NULL)
			{
				*size = flash_event_manager_item[i].len;
			}

			return 0x00;
		}
	}

	return -1;
}

const struct test_record_item_t  test_flash_record_item[] =
{
	{ test_OverCurrentL2, test_OverCurrentEndL2,              5*4096,    341 },
};

int16_t test_flash_event_info(uint16_t event_flag, uint32_t* addr, uint32_t* size, uint32_t* depth)
{
	uint8_t i, k;

	for (i = 0; i < sizeof(test_flash_record_item) / sizeof(test_flash_record_item[0]); i++)
	{
		if ((event_flag == test_flash_record_item[i].event_flag_id_start) || (event_flag == test_flash_record_item[i].event_flag_id_end))
		{
			if (addr != NULL)
			{
				*addr = 0x00;
				for (k = 0x00; k < i; k++)
				{
					*addr += test_flash_record_item[k].len;
				}
			}

			if (size != NULL)
			{
				*size = test_flash_record_item[i].len;
			}

			if (depth != NULL)
			{
				*depth = test_flash_record_item[i].max_depth;
			}

			return 0x00;
		}
	}

	return -1;
}

struct test_flash_evnet_log_output_t
{
	uint32_t index;
	uint32_t start_time;
	uint32_t end_time;
	uint8_t temp[36];
};

#define FLASH_EVENT_LONG_NUM  1000
static struct test_flash_evnet_log_output_t test_flash_event[FLASH_EVENT_LONG_NUM];

void produce_the_flash_event_log(struct instance_t* instance)
{
	uint32_t start_time = 41990397;//rtc_mktime(&instance->time);
	uint32_t index = 0x00;
	uint16_t i;
	uint8_t data;

	for (i = 0x00; i < FLASH_EVENT_LONG_NUM; i++)
	{
		generate_data(&data, 0x01, 0x01, 0x00);
		test_flash_event[i].index = index + i;
		test_flash_event[i].start_time = start_time + (0x0f & data);
		generate_data(&data, 0x01, 0x01, 0x00);
		test_flash_event[i].end_time = test_flash_event[i].start_time + (0x0f & data);
		generate_data(test_flash_event[i].temp, 36, 36, 0x00);
		start_time += 600;
	}
}

void test_nd_log_output_flash_search_all(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;

	//产生数据
	produce_the_flash_event_log(instance);
	//注册事件的管理区
	register_parameter(instance, RES_DEV_EEPROM, test_event_manager_name, 1 * 1024, test_flash_event_manager_info);
	//注册事件
	register_log(instance, RES_DEV_FLASH, test_event_record, RES_DEV_EEPROM, test_event_manager_name, 5 * 4096, test_flash_event_info);
	
	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	ret = nd_log_append(instance, index, (uint8_t *)&test_flash_event[0],sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	ret = nd_log_output(instance, index, (uint8_t *)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	//比对数据
	assert_memory_equal(&temp_event, &test_flash_event[0], sizeof(struct test_flash_evnet_log_output_t));
}

void test_nd_log_output_flash_search_all_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 0x00;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_all_400(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 400; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 59;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_all_426(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 426; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 85;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));


	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_all_500(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 500; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 159;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));


	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_index_100_30(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 32;
	info.end_mark = 61;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 30); i = 32;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_index_100_40(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 60;
	info.end_mark = 200;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 40); i = 60;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 100;
	info.end_mark = 200;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0); 
}

void test_nd_log_output_flash_search_index_100_18(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 17;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 18); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_index_100_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 99;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 0;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 1); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
}

void test_nd_log_output_flash_search_index_400_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 400; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 47;
	info.end_mark = 158;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 59;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 47;
	info.end_mark = 400;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 59;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 58;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0); 

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 400;
	info.end_mark = 420;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);
}

void test_nd_log_output_flash_search_index_426_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 426; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 66;
	info.end_mark = 184;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 85;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 32;
	info.end_mark = 426;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 85;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 226;
	info.end_mark = 426;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 200); i = 226;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 58;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 400;
	info.end_mark = 420;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 21);
}

void test_nd_log_output_flash_search_index_500_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 500; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 426;
	info.end_mark = 500;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 74); i = 426;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 159;
	info.end_mark = 426;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 268); i = 159;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 600;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 159;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x01;
	info.start_mark = 401;
	info.end_mark = 472;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 72); i = 401;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 0;
	info.end_mark = 58;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x01;
	info.start_mark = 159;
	info.end_mark = 425;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 267);
}

void test_nd_log_output_flash_search_time_100_30(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 32 * 600;
	info.end_mark = start_time + 61 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 30); i = 32;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_time_100_40(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 60 * 600;
	info.end_mark = start_time + 200 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 40); i = 60;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time + 100 * 600;
	info.end_mark = start_time + 200 * 600;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);
}

void test_nd_log_output_flash_search_time_100_18(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 17 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 18); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}
}

void test_nd_log_output_flash_search_time_100_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 100; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time ;
	info.end_mark = start_time + 99 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 1); i = 0;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
}

void test_nd_log_output_flash_search_time_400_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 400; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 47 * 600;
	info.end_mark = start_time + 158 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 59;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 47 * 600;
	info.end_mark = start_time + 400 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 59;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 58 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time + 400 * 600;
	info.end_mark = start_time + 420 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);
}

void test_nd_log_output_flash_search_time_426_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 426; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 66 * 600;
	info.end_mark = start_time + 184 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 100); i = 85;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 32 * 600;
	info.end_mark = start_time + 426 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 85;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 226 * 600;
	info.end_mark = start_time + 426 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 200); i = 226;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 58 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time + 400 * 600;
	info.end_mark = start_time + 420 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 21);
}

void test_nd_log_output_flash_search_time_500_100(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE2;
	uint8_t test_event_record = 0xE3;
	struct circular_info_t info;
	uint32_t start_time = 41990397;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint16_t i = 0x00;

	index = ((uint32_t)(RES_DEV_FLASH << 24) + ((uint32_t)test_event_record << 16) + test_OverCurrentL2);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_true(ret > 0x00);
	//写数据
	for (i = 0x00; i < 500; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	}
	//读数据
	struct test_flash_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 426 * 600;
	info.end_mark = start_time + 500 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 74); i = 426;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 159 * 600;
	info.end_mark = start_time + 426 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 268); i = 159;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 600 * 600;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 341); i = 159;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
	info.mode = 0x02;
	info.start_mark = start_time + 401 * 600;
	info.end_mark = start_time + 472 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 72); i = 401;
	assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_flash_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_flash_event[i], sizeof(struct test_flash_evnet_log_output_t));
	}

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time;
	info.end_mark = start_time + 58 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 0);

	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	info.mode = 0x02;
	info.start_mark = start_time + 159 * 600;
	info.end_mark = start_time + 425 * 600 + 15;
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_flash_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_flash_evnet_log_output_t));
	assert_int_equal(info.valid_num, 267);
}
