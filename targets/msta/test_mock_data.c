#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void demo_how_to_use_cmocka(void **state) 
{
    uint8_t buf[200];
    mock_id_data_ok(0x2000,4);
    uint32_t id1 = 0;
    uint32_t id2 = 0;
    uint8_t test_data[256];
    __read(0x2000, buf, 4);
    __write(0x2000, buf, 4);     //arrange
    struct instance_t* instance;
    instance = (struct instance_t*)*state;
    generate_data(&id1, 8, 4, 0);
    generate_data(&id2, 8, 4, 0);
    generate_data(test_data, sizeof(test_data), 4, 0);
    will_return(__read, test_data);
    will_return(__read, 4);
    expect_value(__write, id, id1);
    expect_memory(__write, buf, test_data, 4);
    expect_value(__write, len, 4);
    generate_data(test_data+4, sizeof(test_data)-4, 4, 0);
    will_return(__read, test_data+4);
    will_return(__read, 4);
    expect_value(__write, id, id2);
    expect_memory(__write, buf, test_data+4, 4);
    expect_value(__write, len, 4);


    __read(id1,buf,4);
    __write(id1,buf,4);    
    assert_memory_equal(buf,test_data,4);

    __read(id2,buf,4);
    __write(id2,buf,4);
    assert_memory_equal(buf,test_data+4,4);
}
