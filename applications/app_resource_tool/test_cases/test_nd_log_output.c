#include "nidavellir.h"
#include <setjmp.h>
#include "cmocka.h"
#include "resource_tool.h"

extern int generate_data(uint8_t* data, size_t n, size_t len, size_t type);

struct event_manager_t
{
	uint8_t event_name;
	uint16_t len;
};

const struct event_manager_t  event_manager_item[] =
{
	{ OverCurrentL1,       sizeof(struct log_manager_t)},// 管理区
};

int16_t stest_event_manager_info(uint16_t id, uint32_t* addr, uint16_t* size)
{
	uint8_t i, k;

	for (i = 0; i < sizeof(event_manager_item) / sizeof(event_manager_item[0]); i++)
	{
		if (event_manager_item[i].event_name == id)
		{
			if (addr != NULL)
			{
				*addr = 0x00;
				for (k = 0x00; k < i; k++)
				{
					*addr += event_manager_item[k].len;
				}
			}

			if (size != NULL)
			{
				*size = event_manager_item[i].len;
			}

			return 0x00;
		}
	}

	return -1;
}

struct test_record_item_t
{
	uint16_t  event_flag_id_start;  // 事件代码
	uint16_t  event_flag_id_end;    // 事件代码2
	uint16_t len;
	uint16_t max_depth;     // 默认最大深度
};

const struct test_record_item_t  test_record_item[] =
{
	{ OverCurrentL1, OverCurrentEndL1,              800,    10 },// 待调整
};

int16_t test_event_info(uint16_t event_flag, uint32_t* addr, uint16_t* size, uint16_t* depth)
{
	uint8_t i, k;

	for (i = 0; i < sizeof(test_record_item) / sizeof(test_record_item[0]); i++)
	{
		if ((event_flag == test_record_item[i].event_flag_id_start) || (event_flag == test_record_item[i].event_flag_id_end))
		{
			if (addr != NULL)
			{
				*addr = 0x00;
				for (k = 0x00; k < i; k++)
				{
					*addr += test_record_item[k].len;
				}
			}

			if (size != NULL)
			{
				*size = test_record_item[i].len;
			}

			if (depth != NULL)
			{
				*depth = test_record_item[i].max_depth;
			}

			return 0x00;
		}
	}

	return -1;
}


struct test_evnet_log_output_t
{
	uint32_t index;
	uint32_t start_time;
	uint32_t end_time;
	uint8_t temp[16];
};
#define EVENT_LONG_NUM  1000
static struct test_evnet_log_output_t test_event[EVENT_LONG_NUM];
void produce_the_event_log(struct instance_t* instance)
{
	uint32_t start_time = rtc_mktime(&instance->time);
	uint32_t index = 0x00;
	uint16_t i;
	uint8_t data;

	for (i = 0x00; i < EVENT_LONG_NUM; i++)
	{
		generate_data(&data, 0x01, 0x01, 0x00);
		test_event[i].index = index + i;
		test_event[i].start_time = start_time + (0x0f & data);
		generate_data(&data, 0x01, 0x01, 0x00);
		test_event[i].end_time = test_event[i].start_time + (0x0f & data);
		generate_data(test_event[i].temp, 16, 0x01, 0x00);
	}
}

void test_nd_log_output_search_all(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE0;
	uint8_t test_event_record = 0xE1;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;

	//产生数据
	produce_the_event_log(instance);
	//注册事件的管理区
	register_parameter(instance, RES_DEV_EEPROM, test_event_manager_name, 1 * 1024, stest_event_manager_info);
	//注册事件
	register_log(instance, RES_DEV_EEPROM, test_event_record, RES_DEV_EEPROM, test_event_manager_name, 10 * 1024, test_event_info);
	
	index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_event_record << 16) + OverCurrentL1);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_int_equal(ret, 0x00);
	//写数据
	ret = nd_log_append(instance, index, (uint8_t *)&test_event[0],sizeof(struct test_evnet_log_output_t));
	assert_int_equal(ret, sizeof(struct test_evnet_log_output_t));
	//读数据
	struct test_evnet_log_output_t temp_event;
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_evnet_log_output_t));
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	ret = nd_log_output(instance, index, (uint8_t *)&temp_event, sizeof(struct test_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_evnet_log_output_t));
	//比对数据
	assert_memory_equal(&temp_event, &test_event[0], sizeof(struct test_evnet_log_output_t));
}


void test_nd_log_output_search_all_06(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	uint8_t test_event_manager_name = 0xE0;
	uint8_t test_event_record = 0xE1;
	struct circular_info_t info;
	uint32_t index = 0x00;
	int16_t ret = 0x00;
	uint8_t i = 0x00;

	index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_event_record << 16) + OverCurrentL1);
	//先清一下管理区
	ret = nd_log_clean(instance, index);
	assert_int_equal(ret, 0x00);
	//写数据
	for (i = 0x00; i < 0x06; i++)
	{
		ret = nd_log_append(instance, index, (uint8_t*)&test_event[i], sizeof(struct test_evnet_log_output_t));
		assert_int_equal(ret, sizeof(struct test_evnet_log_output_t));
	}
	//读数据
	struct test_evnet_log_output_t temp_event;
	memset((uint8_t*)&info, 0x00, sizeof(struct circular_info_t));
	memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_evnet_log_output_t));
	ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_evnet_log_output_t), &info);
	assert_int_equal(ret, sizeof(struct test_evnet_log_output_t));
	assert_memory_equal(&temp_event, &test_event[0], sizeof(struct test_evnet_log_output_t));
	i = 0x00;

	while (info.curr_num < info.valid_num)
	{
		i++;
		memset((uint8_t*)&temp_event, 0x00, sizeof(struct test_evnet_log_output_t));
		ret = nd_log_output(instance, index, (uint8_t*)&temp_event, sizeof(struct test_evnet_log_output_t), &info);
		assert_int_equal(ret, sizeof(struct test_evnet_log_output_t));
		assert_memory_equal(&temp_event, &test_event[i], sizeof(struct test_evnet_log_output_t));
	}
}

