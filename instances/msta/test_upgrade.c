#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

//用于升级信息查询
#pragma pack(1)
struct device_info_t
{
    uint16_t device_type;//设备模块类型码
    uint8_t hw_version[8];//设备硬件备案号
    uint8_t hw_serial[16];//硬件流水号
    uint8_t fw_version[8];//设备软件备案号
    uint8_t fw_update_date[5];//软件更新日期
    uint8_t max_app;//最多支持的app数量
    uint8_t installed_app;//已下装的app数量
    uint8_t fw_upgrade_supported;//是否支持软件升级
    uint32_t ram_size;//ram总容量
    uint32_t ram_remained_size;//ram剩余容量
    uint32_t code_size;//code总容量
    uint32_t code_remained_size;//code剩余容量
    uint16_t cpu_load;//cpu利用率
    uint16_t mtu;//支持升级传输块的最大长度
    uint32_t max_app_size;//支持升级块的最大长度
    uint8_t algo;//验签算法类型
};

#pragma pack(1)
struct certficate_t
{
    uint8_t random[8];
    uint8_t PA;
    uint8_t P0;
    uint8_t P1;
    uint8_t P2;
    uint32_t FC;//帧序号起始值
    uint8_t AID;
    uint8_t name[10];
    uint16_t device_type;//设备类型码
    uint16_t fcs;
    uint16_t len;//每包字节数
    uint16_t minutes;
    uint32_t file_size;
    uint8_t hw_version[8];//设备硬件备案号
};

uint8_t* ifile;
uint8_t* ofile;
uint8_t blocks[upgrade_setsize];
struct device_info_t info;
struct certficate_t cert;

int write_hook(uint32_t id, uint8_t* buf, int len)
{
    uint32_t fc = 0;
    uint16_t n = 0;
    uint16_t m = 0;

    if (id == 0x07fb0400)
    {
        memcpy(&fc, buf, 4);
        memcpy(ofile + fc * info.mtu, buf + 4, len-4);
    }
    else if (id == 0x07fb0303)
    {
        memcpy(&n, buf+1, 2);
        memcpy(&m, buf+3, 2);
        memcpy(blocks + n/8, buf + 5, len - 5);
    }
}

int send_block(struct instance_t *instance,struct message_t *mesg,size_t fc,int fail_this_block)
{
    uint8_t buf[256];
    size_t block_len = info.mtu;
    uint8_t P[4] = { 0x90,0x11,0x22,0x33 };
    uint8_t C[4] = { 0x90,0x11,0x22,0x33 };
    if (fc * info.mtu + info.mtu > info.max_app_size)
    {
        block_len = info.max_app_size % info.mtu;
    }
    mesg->id = 0x07fb0400;
    mesg->from = kernel;
    mesg->to = sta_tx_to_msta;
    mesg->ctrl = 0x14;
    mesg->len = 4 + 8 + 4 + block_len;
    memcpy(buf, P, 4);
    memcpy(buf + 4, C, 4);
    memcpy(buf + 8, &fc, 4);
    memcpy(buf + 12, ifile + fc * info.mtu, block_len);
    send_mesg(instance, &pl_kernel, mesg, buf);
    if (fail_this_block == 1)
    {
        will_return(__return_error, -4);
    }
    else
    {
        will_return(__return_error, 6);
        expect_value(__write, id, 0x07fb0400);
        expect_memory(__write, buf, buf + 8, block_len + 4);
        expect_value(__write, len, block_len + 4);
    }
    nidavellir_loop_n(instance, 3);
    if (fail_this_block == 1)
    {
        assert_false(bit_isset(fc, instance->upgrade_info.bits));
    }
    else
    {
        assert_true(bit_isset(fc, instance->upgrade_info.bits));
    }

    return 0;
}

void test_upgrade(void **state)
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    instance = (struct instance_t*)*state;
    uint8_t P[4] = { 0x90,0x11,0x22,0x33 };
    uint8_t C[4] = { 0x90,0x11,0x22,0x33 };
    struct message_t mesg;
    uint32_t blocks_to_send;
    uint8_t buf[256];
    uint8_t blocks_failed[40];

    info.device_type = 0x03;
    info.max_app_size = 100 * 1024;
    info.mtu = 192;
    cert.device_type = 0x03;
    ifile = malloc(info.max_app_size);
    ofile = malloc(info.max_app_size);
    memset(ofile, 0xff, info.max_app_size);
    assert_true(info.max_app_size < info.mtu* upgrade_setsize * 8);
    blocks_to_send = (info.max_app_size + info.mtu-1) / info.mtu;
    generate_data(blocks_failed, 40, 40, 0);//40包传输失败

    memcpy(buf, P, 4);
    memcpy(buf+4, C, 4);
    memcpy(buf+8, &cert, sizeof(struct certficate_t));

    init_test_environment();
    extern int set_writer(write__ writer);
    set_writer(write_hook);

    generate_data(ifile, info.max_app_size, info.max_app_size, 0);

    //读取升级信息描述
    mesg.id = 0x07fb0200;
    mesg.ctrl = 0x11;
    mesg.len = 4;
    mesg.from = kernel;
    mesg.to = sta_tx_to_msta;
    send_mesg(instance, &pl_kernel, &mesg, NULL);
    expect_value(__write, id, 0x07fb0200);
    expect_memory(__write, buf, &info, sizeof(struct device_info_t));
    expect_value(__write, len, sizeof(struct device_info_t));
    will_return(__read, &info);
    will_return(__read, sizeof(struct device_info_t));
    nidavellir_loop_n(instance, 3);
    //下发证书
    mesg.id = 0x07fb0201;
    mesg.ctrl = 0x14;
    //4字节:PAP0P1P2  密码权限及其密码
    //4字节:C0C1C2C3  操作者代码
    mesg.len = 4 + 4 + 4 + sizeof(struct certficate_t);//
    mesg.from = kernel;
    mesg.to = sta_tx_to_msta;
    send_mesg(instance, &pl_kernel, &mesg, buf);
    will_return(__return_error, 6);
    expect_value(__write, id, 0x07fb0201);
    expect_memory(__write, buf, &cert, sizeof(struct certficate_t));
    expect_value(__write, len, sizeof(struct certficate_t));
    nidavellir_loop_n(instance, 3);
    
    //传输升级文件
    int fail_this_block = 0;
    for (size_t i = 0; i < blocks_to_send; i++)
    {
        fail_this_block = 0;
        for (size_t j = 0; j < 40; j++)
        {
            if (i == blocks_failed[j])
            {
                //模拟本包传输失败
                fail_this_block = 1;
                break;
            }
        }

        send_block(instance,&mesg,i,fail_this_block);
    }

    for (size_t i = 0; i < 40; i++)
    {
        assert_false(bit_isset(blocks_failed[i], instance->upgrade_info.bits));
    }
    assert_memory_not_equal(ifile, ofile, info.max_app_size);

    //读取升级位图信息 查漏
    uint16_t n = 0;
    uint16_t m = 10;
    uint16_t step = 100;
    while (1)
    {
        mesg.id = 0x07fb0303;
        mesg.ctrl = 0x11;
        mesg.from = kernel;
        mesg.to = sta_tx_to_msta;
        buf[0] = 0;//SA
        memcpy(buf + 1, &n, 2);
        memcpy(buf + 3, &m, 2);
        mesg.len = 4 + 5;
        send_mesg(instance, &pl_kernel, &mesg, buf);
        memcpy(buf + 5, instance->upgrade_info.bits + n / 8, (m + 7) / 8 - n / 8);
        expect_value(__write, id, 0x07fb0303);
        expect_memory(__write, buf, buf, 5 + (m + 7) / 8 - n / 8);
        expect_value(__write, len, 5 + (m + 7) / 8 - n / 8);
        nidavellir_loop_n(instance, 4);
        n = m;
        if (m == blocks_to_send)
        {
            break;
        }
        if (m + step > blocks_to_send)
        {
            m = blocks_to_send;
        }
        else
        {
            m += step;
        }
    }

    assert_memory_equal(blocks, instance->upgrade_info.bits, (blocks_to_send+7)/8);

    //补漏
    for (size_t i = 0; i < blocks_to_send; i++)
    {
        if (!bit_isset(i, blocks))
        {
            send_block(instance, &mesg, i, 0);
        }
        assert_true(bit_isset(i, instance->upgrade_info.bits));
    }

    //升级成功
    assert_memory_equal(ifile, ofile, info.max_app_size);
    free(ifile);
    free(ofile);
    return;
    while (1)
    {
        //读取升级状态字
        mesg.id = 0x07fb0301;
        mesg.ctrl = 0x11;
        mesg.len = 4;
        mesg.from = kernel;
        mesg.to = sta_tx_to_msta;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        nidavellir_loop_n(instance, 3);
    }

    //读取升级使能剩余时间
    mesg.id = 0x07fb0302;
    mesg.ctrl = 0x11;
    mesg.len = 4;
    mesg.from = kernel;
    mesg.to = sta_tx_to_msta;
    //send_mesg(instance, &pl_kernel, &mesg, NULL);
}

