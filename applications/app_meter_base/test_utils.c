
#include "kernel\nidavellir.h"

struct coder_t
{
    int (*size)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    int (*reader)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    int (*writer)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    int (*generator)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
};
uint8_t test_data[1024];
int offset = 0;
write__ real_write;
int init_test_environment()
{
    offset = 0;
    real_write = NULL;
    time_t t;
    srand((unsigned)time(&t));
    return 0;
}

int set_writer(write__ writer)
{
    real_write = writer;
}
// true false
uint8_t generate_boolean()
{
    uint8_t r = 0;
    r = rand() % (2);
    return r;
}

size_t generate_bit_string(uint8_t *buf,size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = rand() % (2);
    }
    return size;
}

size_t __octet_string(uint8_t* buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = rand() % (255);
    }
    return size;
}

size_t __date_time(uint8_t* buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = rand() % (255);
    }
    return size;
}

size_t __date(uint8_t* buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = rand() % (255);
    }
    return size;
}

size_t __time(uint8_t* buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = rand() % (255);
    }
    return size;
}

// -128~127
int8_t generate_integer8(int8_t min,int8_t max)
{
    int8_t r = 0;
    r = rand() % (max+1-min) + min;
    return r;
}
// 0~255
uint8_t generate_unsigned_integer8(uint8_t min, uint8_t max)
{
    uint8_t r = 0;
    r = rand() % (max + 1 - min) + min;
    return r;
}
// -32768~32767
int16_t generate_integer16(int16_t min, int16_t max)
{
    int16_t r = 0;
    r = rand() % (max + 1 - min) + min;
    return r;
}
// 0~65535
uint16_t generate_unsigned_integer16(uint16_t min, uint16_t max)
{
    uint16_t r = 0;
    r = rand() % (max + 1 - min) + min;
    return r;
}
// -32768~32767
int64_t generate_integer64(int64_t min, int64_t max)
{
    int64_t r = 0;
    r = rand() % (max + 1 - min) + min;
    return r;
}

// 0~65535
uint64_t generate_unsigned_integer64(uint64_t min, uint64_t max)
{
    uint64_t r = 0;
    r = rand() % (max + 1 - min) + min;
    return r;
}

size_t __array_raw(uint8_t* buf, size_t size, uint8_t type)
{
    return size;
}

size_t __array_axdr(uint8_t* buf, size_t size, uint8_t type)
{
    return size;
}

//按照instance的数据资源声明和布局情况  生成对应的结构化的数据
//硬件资源在instance里面  这个函数按照布局情况往硬件资源填充相应符合结构化规范的数据
int generate_data_for_instance(struct instance_t *instance)
{
    return 0;
}

//type表达数据类型  每个数据应该是结构化的 且每个字段应存在最大值和最小值todo 程波
//type == 0  生成一个长度为len的hex序列
//type == 1  time 生成一个符合时间格式的序列
//根据数据模型  此函数生成符合该数据的结构话的随机数据
int generate_data(uint8_t *data,size_t n,size_t len,size_t type)
{
   time_t t;
   size_t nn = len;
   if(len > n)
   {
	   nn = n;
   }
   srand((unsigned) time(&t));
   for(size_t i = 0 ; i < nn ; i++ ) 
   {
	  data[i] = rand() % 0xff;
   } 
   return 0;
}

int __read(uint32_t id, uint8_t *buf, int len)
{
//    uint32_t idd = mock_type(uint32_t);
    uint8_t* p = mock_ptr_type(uint8_t*);
    int r = mock_type(int);
    if (r > 0)
    {
        memcpy(buf, p, r);
    }
    return r;
}

int __write(uint32_t id, uint8_t *buf, int len)
{
#ifdef NIDAVELLIR_TEST
    check_expected(id);
    check_expected(buf);
    check_expected(len);
    if (real_write != NULL)
    {
        real_write(id, buf, len);
    }
    return len;
#else
    return 0;
#endif
}

//模拟读写数据成功的情况
int mock_id_data_ok(uint32_t id,size_t len)
{
    if (offset + len >= 1024)
    {
        offset =  0;
    }
    generate_data(test_data+offset, sizeof(test_data)- offset, len, 0);
    expect_value(__write, id, id);
    expect_memory(__write, buf, test_data+offset, len);
    expect_value(__write, len, len);
  //  will_return(__read, id);
    will_return(__read, test_data+offset);
    will_return(__read, len);
#ifdef VERBOSE
    printf("mock data for 0x%04x:", id);
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x", test_data[i+ offset]);
    }
    printf("\n");
#endif
    offset += len;
    return 0;
}
int mock_data_for_id_always(uint32_t id, size_t len)
{
    generate_data(test_data + offset, sizeof(test_data) - offset, len, 0);
    expect_value(__write, id, id);
    expect_memory(__write, buf, test_data + offset, len);
    expect_value(__write, len, len);
    will_return(__read, test_data + offset);
    will_return(__read, len);
#ifdef VERBOSE
    printf("mock data for 0x%04x:", id);
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x", test_data[i + offset]);
    }
    printf("\n");
#endif
    offset += len;
    return 0;
}

int mock_time(struct tm *time,uint32_t id)
{
    time_to_array_0(time, test_data+offset, 6);
    expect_value(__write, id, id);
    expect_memory(__write, buf, test_data+offset, 6);
    expect_value(__write, len, 6);
    will_return(__read, test_data+offset);
    will_return(__read, 6);
    offset += 6;
    return 0;
}

//模拟读写数据失败的情况
int mock_id_data_error(uint32_t id)
{
    generate_data(test_data, sizeof(test_data), 4, 0);
    expect_value(__write, id, id);
    expect_memory(__write, buf, test_data, 4);
    expect_value(__write, len, 4);
    will_return(__read, id);
    will_return(__read, test_data);
    will_return(__read, 4);
    return 0;
}

int __return_error()
{
    int r = mock_type(int);
    return r;
}

int nidavellir_n_loop(struct instance_t** instances, size_t n)
{
    while (1)
    {
        for (size_t i = 0; i < n; i++)
        {
            nidavellir_run(instances[i]);
        }
    }
}


int nidavellir_loop_n_with_rtc_running(struct instance_t* instance, int n)
{
    struct message_t mesg;
    /*
    for (size_t i = 0; i < n; i++)
    {
        if (msta_context.CCO != 1)
        {
            mesg.from = kernel;
            mesg.to = pid_msta;
            mesg.ctrl = 0x01;
            mesg.id = 0x0000001;
            mesg.len = 4;
            send_mesg(instance, NULL, &mesg, NULL);
        }
        nidavellir_run(instance);
        nidavellir_run(instance);
    }*/
}

int nidavellir_loop_n(struct instance_t* instance, int n)
{
    for (size_t i = 0; i < n; i++)
    {
        nidavellir_run(instance);
    }
}

//从from开始连续运行n seconds
int nidavellir_run_for_n_seconds_from_time(struct instance_t* instance, int n, struct tm* from)
{
    if (from != NULL)
        memcpy(&instance->time, from, sizeof(struct tm));

    uint32_t ts = rtc_mktime(&instance->time);
    uint32_t ts_expected = ts + n;
    clock_t start = clock();
    struct tm tm_expected;
    rtc_localtime(ts + n, &tm_expected);
    struct message_t mesg;

    while (ts <= ts_expected)
    {
        mesg.from = kernel;
        mesg.to = pid_msta;
        mesg.ctrl = 0x01;
        mesg.id = 0x0000001;
        mesg.len = 4;
        ts += 1;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        nidavellir_run(instance);
    }

    return 0;
}
