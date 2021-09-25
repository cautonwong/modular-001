#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
/*
extern int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_gatekeeper_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_gatekeeper_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
struct context_698_t client_ctx_698;

const struct filter_t fr_client_tx_698[] =
{
    {0x01,fr_tx_698_entry,1,13},
};
struct fifo_t client_pipe_tx_698_req;

struct pipeline_t pl_client_tx_698 =
{
    pid_client_tx_698,
    "pl_client_tx_698",
    &client_ctx_698,
    &client_pipe_tx_698_req,
    &stdin_698,
    sizeof(fr_client_tx_698) / sizeof(fr_client_tx_698[0]),
    fr_client_tx_698,
};

const struct filter_t fr_client_rx_698[] =
{
    {0x00,fr_rx_698_entry,1,13},
    {0x01,fr_gatekeeper_client,5,13},
};

struct pipeline_t pl_client_rx_698 =
{
    pid_client_rx_698,
    "pl_client_rx_698",
    &client_ctx_698,
    &stdout_698,
    NULL,
    sizeof(fr_client_rx_698) / sizeof(fr_client_rx_698[0]),
    fr_client_rx_698,
};
*/
///*
void test_dlms_rx_entry(void** state)
{
    struct message_t mesg;
    struct instance_t* instance;
    instance = (struct instance_t*)*state;
    struct fifo_t* fifo = &stdin_698;
    uint8_t buf[1024];
    size_t len = 0;

    nidavellir_loop(instance);


    uint8_t f_ok1[] = { 0x7e, 0xa0, 0x0a, 0x48, 0x68, 0xfe, 0xff, 0x75,
        0x93, 0xd8, 0xf8, 0x7e};
    uint8_t f_ok2[] = { 0x7e, 0xa0, 0x07, 0x03, 0x21, 0x93, 0x0f, 0x01, 0x7e };

    uint8_t f_err1[] = { 0x00, 0x00, 0x00, 0x10, 0x26, 0xf6, 0x05, 0x01, 0x00, 0x60, 0x12, 0x02,0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x26, 0xf6, 0x05, 0x01, 0x00, 0x60, 0x12, 0x02,
        0x00, 0x00, 0x10, 0x86, 0x16 };
    uint8_t f_err2[] = { 0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x26, 0xf6, 0x05, 0x01, 0x00, 0x60, 0x12, 0x02,
        0x00, 0x00, 0x10, 0x86, 0x16 };
    uint8_t f_ok3[] = {
        0x68,0x1E,0x00,0xC3,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0xEA,0x9D,0x85,0x01,0x44,0x00,0x00,
        0x02,0x01,0x01,0x05,0x00,0x00,0x0D,0x59,0x00,0x00,0x28,0x6C,0x16
    };
    uint8_t f_err3[] = { 0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, };
    uint8_t f_err4[] = { 0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, };

    len = 0;
    memcpy(buf + len, f_ok1, sizeof(f_ok1));
    len += sizeof(f_ok1);
    fifoBuf_putData(fifo, buf, len);
    nidavellir_run(instance);
    assert_true(ctx_698.rx_count == 1);
    assert_true(fifoBuf_getUsed(&stdin_698) == 0);
    len = 0;
    memcpy(buf + len, f_err3, sizeof(f_err3));
    len += sizeof(f_err3);
    fifoBuf_putData(fifo, buf, len);
    nidavellir_run(instance);
    assert_true(ctx_698.rx_count == 1);
    assert_true(fifoBuf_getUsed(&stdin_698) == 0);

    len = 0;
    memcpy(buf + len, f_err2, sizeof(f_err2));
    len += sizeof(f_err2);
    fifoBuf_putData(fifo, buf, len);
    nidavellir_run(instance);
    assert_true(ctx_698.rx_count == 1);
    assert_true(fifoBuf_getUsed(&stdin_698) == 0);

    len = 0;
    memcpy(buf + len, f_err1, sizeof(f_err1));
    len += sizeof(f_err1);
    memcpy(buf + len, f_ok1, sizeof(f_ok1));
    len += sizeof(f_ok1);
    fifoBuf_putData(fifo, buf, len);
    nidavellir_run(instance);
    assert_true(ctx_698.rx_count == 2);
    assert_true(fifoBuf_getUsed(&stdin_698) == 0);
    for (size_t i = 0; i < 30; i++)
    {
        len = 0;
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err3, sizeof(f_err3));
        len += sizeof(f_err3);
        memcpy(buf + len, f_ok1, sizeof(f_ok1));
        len += sizeof(f_ok1);
        memcpy(buf + len, f_err3, sizeof(f_err3));
        len += sizeof(f_err3);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_ok2, sizeof(f_ok2));
        len += sizeof(f_ok2);
        memcpy(buf + len, f_err3, sizeof(f_err3));
        len += sizeof(f_err3);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_ok2, sizeof(f_ok2));
        len += sizeof(f_ok2);
        memcpy(buf + len, f_err3, sizeof(f_err3));
        len += sizeof(f_err3);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_ok2, sizeof(f_ok2));
        len += sizeof(f_ok2);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_err1, sizeof(f_err1));
        len += sizeof(f_err1);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        memcpy(buf + len, f_ok1, sizeof(f_ok1));
        len += sizeof(f_ok1);
        memcpy(buf + len, f_err3, sizeof(f_err3));
        len += sizeof(f_err3);
        memcpy(buf + len, f_ok3, sizeof(f_ok3));
        len += sizeof(f_ok3);
        memcpy(buf + len, f_err2, sizeof(f_err2));
        len += sizeof(f_err2);
        fifoBuf_putData(fifo, buf, len);
        nidavellir_run(instance);
        assert_true(ctx_698.rx_count == 8 + 6 * i);//解析出来的正确帧数
        assert_true(fifoBuf_getUsed(&stdin_698) == 0);
    }
}
//*/
/*
void test_698_rx_entry(void **state) 
{
    struct message_t mesg;
    struct instance_t* instance;
    instance = (struct instance_t*)*state;
    struct fifo_t* fifo = &stdin_698;
    uint8_t buf[1024];
    size_t len = 0;

    init_test_environment();
    uint8_t *p = malloc(2048);
    client_ctx_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(client_ctx_698.apdu, p, 2048);

    p = malloc(2048);
    fifoBuf_init(&client_pipe_tx_698_req, p, 2048);

    extern int close_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    close_serial_port(instance, &pl_rx_from_serial, NULL);

    instance->recipe->pipelines[instance->recipe->pipeline_n] = &pl_client_tx_698;
    instance->recipe->pipelines[instance->recipe->pipeline_n+1] = &pl_client_rx_698;
    instance->recipe->pipeline_n += 2;

    for (size_t i = 0; i < 30; i++)
    {
        mesg.len = 4;
        mesg.from = kernel;
        mesg.to = pid_client_tx_698;
        mesg.ctrl = 0x05;
        mesg.qos = 0x01;
        mesg.len = 9;
        buf[0] = 0x00;
        buf[1] = 0x00;
        buf[2] = 0x02;
        buf[3] = 0x01;
        buf[4] = 0x00;
        send_mesg(instance, &pl_kernel, &mesg, buf);
        //mock_id_data_ok(0x00000201, 4);
        nidavellir_loop_n(instance, 4);
    }
}
*/
enum data_type_t
{
    DT_NULL = 0,
    DT_ARRAY = 1,
    DT_STRUCTURE = 2,
    DT_BOOLEAN = 3,
    DT_BIT_STRING = 4,
    DT_DOUBLE_LONG = 5,
    DT_DOUBLE_LONG_UNSIGNED = 6,
    DT_FLOATING_POINT = 7,
    DT_OCTET_STRING = 9,
    DT_VISIBLE_STRING = 10,
    DT_UTF8_STRING = 12,
    DT_BCD = 13,
    DT_INTEGER = 15,
    DT_LONG = 16,
    DT_INTEGER_UNSIGNED = 17,
    DT_LONG_UNSIGNED = 18,
    DT_COMPACT_ARRAY = 19,
    DT_LONG64 = 20,
    DT_LONG64_UNSIGNED = 21,
    DT_ENUM = 22,
    DT_FLOAT32 = 23,
    DT_FLOAT64 = 24,
    DT_DATE_TIME = 25,
    DT_DATE = 26,
    DT_TIME = 27,
    DT_DATETIME_S = 28,
    DT_OI = 80,
    DT_OAD = 81,
    DT_ROAD = 82,
    DT_OMD = 83,
    DT_TI = 84,
    DT_TSA = 85,
    DT_MAC = 86,
    DT_RN = 87,
    DT_REGION = 88,
    DT_SCALER_UNIT = 89,
    DT_RSD = 90,
    DT_CSD = 91,
    DT_MS = 92,
    DT_SID = 93,
    DT_SID_MAC = 94,
    DT_COMDCB = 95,
    DT_RCSD = 96,

    DT_NOT_CARE = 243,
    DT_SPECIAL_PROCESS = 244,
    DT_PUSH_PROCESS = 245,
    DT_ERROR = 255,
};

/*

{
    //oi    address
    0x0000,0x2000,
}

struct oad_t
{
    uint8_t len;
    uint8_t classid;
    uint8_t property_n;
};

struct property_t
{
    uint8_t len;
    uint8_t property_id;
    uint8_t op;//get/set/action
    uint8_t src;//数据来源
    enum data_type_t type;//数据类型
    uint32_t address;//地址
};

struct  array_t
{
    uint8_t obj_n;//子对象的个数
    enum data_type_t type;//子对象的类型列表  
};
struct  structure_t
{
    uint8_t obj_n;//子对象的个数
    enum data_type_t type[n];//子对象的类型列表  
};
0000 02 01
L 01 07 [L 01 01 00 00 00 ] [L 02 01 01 NNNNNNNN 64 5] [L 03 03 59]

*/
