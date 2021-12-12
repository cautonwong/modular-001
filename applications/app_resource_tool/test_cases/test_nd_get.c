#include "nidavellir.h"
#include <setjmp.h>
#include "cmocka.h"
#include "resource_tool.h"
#include "test_resource.h"


struct resource_map_t* register_map(struct instance_t* instance, uint8_t map_name,
    int (*fun)(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode));

int test_resource_map_get(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    uint8_t* p = mock_ptr_type(void*);
    int r = mock_type(int);
    memcpy(buf, p, r);
    return r;
}

int test_resource_map_set(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    check_expected(id);
    check_expected(buf);
    int v = mock_type(int);
    return v;
}

int test_resource_map(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode)
{
    switch (mode)
    {
    case RESOURCE_MAP_GET:
        return test_resource_map_get(instance, id, buf);
        break;
    case RESOURCE_MAP_SET:
        return test_resource_map_set(instance, id, buf);
        break;
    case RESOURCE_MAP_ACTION:
        break;
    }
}

void test_nd_get(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    uint8_t test_res_map_x = 0xC1;
    struct resource_map_t* map = NULL;
    uint8_t test_buf[16];
    uint8_t s_buf[16];
    int16_t ret = 0;

    map = register_map(instance, test_res_map_x, test_resource_map);
    assert_non_null(map);

    generate_data(test_buf, 16, 16, 0x00);
    will_return(test_resource_map_get, test_buf); //打桩第一个参数
    will_return(test_resource_map_get, 16);      //打桩第二个参数
    memset(s_buf, 0x00, 16);
    ret = nd_get(instance, 0x25670, s_buf, 20);
    assert_int_equal(ret, 16);
    assert_memory_equal(s_buf, test_buf, 16);

    generate_data(test_buf, 16, 16, 0x00);
    expect_value(test_resource_map_set, buf, test_buf);
    expect_value(test_resource_map_set, id, 0x25670);
    will_return(test_resource_map_set, 16);
    ret = nd_set(instance, 0x25670, test_buf, 20);
    assert_int_equal(ret, 16);
}

struct test_para_info_t
{
    uint16_t name;
    uint16_t len;
};

struct test_para_info_t test_para_tab[] =
{
    { 0x11, 0x05}, { 0x12, 0x05}, { 0x13, 0x09}, { 0x14, 0x07}, { 0x15, 0x05}, { 0x16, 0x0C}, { 0x17, 0x05}
};

int16_t test_parameter_info(uint16_t id, uint32_t* addr, uint16_t* size)
{
    uint8_t i, k;

    for (i = 0; i < sizeof(test_para_tab) / sizeof(test_para_tab[0]); i++)
    {
        if (test_para_tab[i].name == id)
        {
            if (addr != NULL)
            {
                *addr = 0x00;
                for (k = 0x00; k < i; k++)
                {
                    *addr += test_para_tab[k].len;
                }
            }

            if (size != NULL)
            {
                *size = test_para_tab[i].len;
            }

            return 0x00;
        }
    }

    return -1;
}

void test_nd_get_index(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    uint8_t test_parameter_name = 0xD3;
    uint8_t buf[50],test_buf[50];
    uint32_t index = 0x00;
    uint32_t* p = NULL;
    int16_t ret = 0x00;
    uint8_t i;

    //注册一个参数
    p = register_parameter(instance, RES_DEV_EEPROM, test_parameter_name, 1 * 1024, test_parameter_info);
   // assert_non_null(p);

    for (i = 0x00; i < sizeof(test_para_tab) / sizeof(test_para_tab[0]); i++)
    {
        index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) + test_para_tab[i].name);
        generate_data(test_buf, 50, 50, 0x00);
        ret = nd_set_index(instance, index, test_buf, test_para_tab[i].len);
        assert_true(ret > 0x00);
        memset(buf, 0x00, 50);
        ret = nd_get_index(instance, index, buf, test_para_tab[i].len);
        assert_int_equal(ret, test_para_tab[i].len);
        assert_memory_equal(test_buf, buf, test_para_tab[i].len);
    }
}
struct test_parameter_info_same_t
{
    uint16_t name;
    uint16_t same;
    uint16_t len;
};

const struct test_parameter_info_same_t   parameter_info_same[] =
{
    {0x01, 0x05, 0x04},
    {0x10, 0x05, 0x04},
    {0x20, 0x05, 0x04},
    {0x30, 0x05, 0x04},
    {0x40, 0x05, 0x04}
};

#define TEST_POINTER_EMPTY(a) if(a == NULL) {return NULL;}
int16_t test_parameter_info_same(uint16_t id, uint32_t* addr, uint16_t* size)
{
    uint16_t i, k;
    uint16_t offset;

    for (i = 0; i < sizeof(parameter_info_same) / sizeof(parameter_info_same[0]); i++)
    {
        if ((parameter_info_same[i].name + (parameter_info_same[i].same - 0x01) >= id) &&
            (parameter_info_same[i].name <= id))
        {
            TEST_POINTER_EMPTY(addr);
            *addr = 0x00;

            for (k = 0x00; k < i; k++)
            {
                *addr += parameter_info_same[k].len * parameter_info_same[k].same + 0x02;  //加两个字节的校验码；
            }
            offset = (id - parameter_info_same[i].name) * parameter_info_same[k].len;
            *addr += offset;

            TEST_POINTER_EMPTY(size);
            *size = parameter_info_same[i].len;

            return 1;
        }
    }

    return -1;
}

void test_nd_get_index_same(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    uint8_t test_parameter_name = 0xD4;
    uint8_t buf[50], test_buf[50];
    uint32_t index = 0x00;
    uint32_t* p = NULL;
    int16_t ret = 0x00;
    uint8_t i,k;

    //注册一个参数
    p = register_parameter(instance, RES_DEV_EEPROM, test_parameter_name, 1 * 1024, test_parameter_info_same);
    // assert_non_null(p);

    for (i = 0x00; i < sizeof(parameter_info_same) / sizeof(parameter_info_same[0]); i++)
    {
        for (k = 0x00; k < parameter_info_same[i].same; k++)
        {
            index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) + parameter_info_same[i].name + k);
            generate_data(test_buf, 50, 50, 0x00);
            ret = nd_set_index(instance, index, test_buf, parameter_info_same[i].len);
            assert_true(ret > 0x00);
            memset(buf, 0x00, 50);
            ret = nd_get_index(instance, index, buf, parameter_info_same[i].len);
            assert_int_equal(ret, parameter_info_same[i].len);
            assert_memory_equal(test_buf, buf, parameter_info_same[i].len);
        }
    }
}


struct test_ctrl_parameter_t
{
    uint32_t para1;
    uint8_t sss1;
    uint16_t ttt6;
    uint32_t para5;
    uint8_t aaa1[5];
};

void test_nd_get_offset(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    uint8_t test_parameter_name = 0xD5;
    uint8_t test_parameter_name_e2 = 0xD6;
    uint8_t buf[50], test_buf[50];
    uint32_t index = 0x00;
    uint16_t len = 0x00;
    uint32_t* p = NULL;
    int16_t ret = 0x00;
    uint8_t i;

    //增加一个BLOCK
    add_block(instance, RES_DEV_RAM, test_parameter_name_e2, sizeof(struct test_ctrl_parameter_t));
    add_block(instance, RES_DEV_EEPROM, test_parameter_name, sizeof(struct test_ctrl_parameter_t));
    //第一个参数
    index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) + 
        offsetof(struct test_ctrl_parameter_t, para1));
    generate_data(test_buf, 50, 50, 0x00);
    len = offsetof(struct test_ctrl_parameter_t, sss1) - offsetof(struct test_ctrl_parameter_t, para1);
    ret = nd_set_offset(instance, index, test_buf, len);
    assert_true(ret > 0x00);
    memset(buf, 0x00, 50); 
    ret = nd_get_offset(instance, index, buf, len);
    assert_int_equal(ret, len);
    assert_memory_equal(test_buf, buf, len);
    //第二参数
    index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) +
        offsetof(struct test_ctrl_parameter_t, sss1));
    generate_data(test_buf, 50, 50, 0x00);
    len = offsetof(struct test_ctrl_parameter_t, ttt6) - offsetof(struct test_ctrl_parameter_t, sss1);
    ret = nd_set_offset(instance, index, test_buf, len);
    assert_true(ret > 0x00);
    memset(buf, 0x00, 50);   
    ret = nd_get_offset(instance, index, buf, len);
    assert_int_equal(ret, len);
    assert_memory_equal(test_buf, buf, len);
    //第三参数
    index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) +
        offsetof(struct test_ctrl_parameter_t, ttt6));
    generate_data(test_buf, 50, 50, 0x00);
    len = offsetof(struct test_ctrl_parameter_t, para5) - offsetof(struct test_ctrl_parameter_t, ttt6);
    ret = nd_set_offset(instance, index, test_buf, len);
    assert_true(ret > 0x00);
    memset(buf, 0x00, 50);  
    ret = nd_get_offset(instance, index, buf, len);
    assert_int_equal(ret, len);
    assert_memory_equal(test_buf, buf, len);
    //第四参数
    index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) +
        offsetof(struct test_ctrl_parameter_t, para5));
    generate_data(test_buf, 50, 50, 0x00);
    len = offsetof(struct test_ctrl_parameter_t, aaa1) - offsetof(struct test_ctrl_parameter_t, para5);
    ret = nd_set_offset(instance, index, test_buf, len);
    assert_true(ret > 0x00);
    memset(buf, 0x00, 50);
    ret = nd_get_offset(instance, index, buf, len);
    assert_int_equal(ret, len);
    assert_memory_equal(test_buf, buf, len);
    //第五参数
    index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)test_parameter_name << 16) +
        offsetof(struct test_ctrl_parameter_t, aaa1));
    generate_data(test_buf, 50, 50, 0x00);
    len = sizeof(struct test_ctrl_parameter_t) - offsetof(struct test_ctrl_parameter_t, aaa1);
    ret = nd_set_offset(instance, index, test_buf, len);
    assert_true(ret > 0x00);
    memset(buf, 0x00, 50);
    ret = nd_get_offset(instance, index, buf, len);
    assert_int_equal(ret, len);
    assert_memory_equal(test_buf, buf, len);
}
