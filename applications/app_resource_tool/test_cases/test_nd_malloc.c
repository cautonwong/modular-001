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
		0x00,0x01,0x5555,0x00,0x00, NULL, NULL, NULL, NULL, NULL, NULL,".//e2prom.dat"
	};

	//×¢²átest_dev 
	ret = add_device(instance, test_dev_name, &test_dev);
	assert_true(ret > 0);

	device = look_up_device(instance, test_dev_name);
	assert_null(device);

	assert_int_equal(device->size,test_dev.data_size);

}


