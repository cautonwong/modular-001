#ifndef __TEST_RESOURCE_H__
#define __TEST_RESOURCE_H__

struct event_manager_t
{
	uint8_t event_name;
	uint16_t len;
};

struct test_record_item_t
{
	uint16_t  event_flag_id_start;  // 事件代码
	uint16_t  event_flag_id_end;    // 事件代码2
	uint32_t len;
	uint32_t max_depth;     // 默认最大深度
};

extern int generate_data(uint8_t* data, size_t n, size_t len, size_t type);

#endif