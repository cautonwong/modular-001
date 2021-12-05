#include "nidavellir.h"
#include <setjmp.h>
#include "cmocka.h"
#include "resource_tool.h"


void test_add_device(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct block_device_t* device = NULL;
	uint8_t test_dev_name = 0xFE;
	int8_t ret = 0x00;

	const struct dev_info_t test_dev =
	{
		0xF1,0xF0,0xF000,0x00,0x00, NULL, NULL, NULL, NULL, NULL, NULL,".test"
	};

	//注册test_dev 
	ret = add_device(instance, test_dev_name, &test_dev);
	assert_true(ret > 0);

	device = look_up_device(instance, test_dev_name);
	assert_non_null(device);

	assert_int_equal(device->size,test_dev.data_size);
}

void test_add_block_one_byte(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct resource_block_t* block = NULL;
	uint8_t test_dev_name = 0xFE;
	uint8_t test_block_name = 0xF0;
	uint32_t ret = 0;

	//申请1个字节的BOLCK
	ret = add_block(instance, test_dev_name, test_block_name, 0x01);
	assert_true(ret == 0);
	block = look_up_block(instance, look_up_device(instance, test_dev_name), test_block_name);
	assert_non_null(block);
	assert_int_equal(block->size, 0x01);

}

void test_add_block_same_block_name(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct resource_block_t* block = NULL;
	uint8_t test_dev_name = 0xFE;
	uint8_t test_block_name = 0xF0;
	uint32_t ret = 0;

	//同名的Blok
	ret = add_block(instance, test_dev_name, test_block_name, 500);
	assert_true(ret == 0);
	block = look_up_block(instance, look_up_device(instance, test_dev_name), test_block_name);
	assert_non_null(block);
	assert_int_equal(block->size, 0x01);
}

void test_add_block_500_byte(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct resource_block_t* block = NULL;
	uint8_t test_dev_name = 0xFE;
	uint8_t test_block_name = 0xF0 + 1;
	uint32_t block_size = 500;
	uint32_t ret = 0;

	//同名的Blok
	ret = add_block(instance, test_dev_name, test_block_name, block_size);
	assert_true(ret > 0);
	block = look_up_block(instance, look_up_device(instance, test_dev_name), test_block_name);
	assert_non_null(block);
	assert_int_equal(block->size, block_size);
}

void test_add_block_10K_byte(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct resource_block_t* block = NULL;
	uint8_t test_dev_name = 0xFE;
	uint8_t test_block_name = 0xF0 + 2;
	uint32_t block_size = 10 * 1024;
	uint32_t ret = 0;

	//同名的Blok
	ret = add_block(instance, test_dev_name, test_block_name, block_size);
	assert_true(ret > 0);
	block = look_up_block(instance, look_up_device(instance, test_dev_name), test_block_name);
	assert_non_null(block);
	assert_int_equal(block->size, block_size);
}

void test_add_block_out_of_gauge(void** state)
{
	struct instance_t* instance = (struct instance_t*)*state;
	struct resource_block_t* block = NULL;
	uint8_t test_dev_name = 0xFE;
	uint8_t test_block_name = 0xF0 + 3;
	uint32_t block_size = 50 * 1024;
	uint32_t ret = 0;

	//同名的Blok
	ret = add_block(instance, test_dev_name, test_block_name, block_size);
	assert_true(ret == 0);
	block = look_up_block(instance, look_up_device(instance, test_dev_name), test_block_name);
	assert_null(block);
}