#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"

//Get-Requeset数据类型测试用例
//1、读取一个对象属性请求Get-Requeset数据类型测试
//2、读取若干个对象属性请求Get-RequesetList数据类型测试
//3、读取一个记录型对象属性GetRequestRecord数据类型测试
//4、读取若干个记录型对象属性GetRequestRecordList数据类型测试
//5、读取分帧响应的下一个数据块请求GetRequestNext数据类型测试
//6、读取一个对象属的MD5值GetRequestMD5数据类型测试


void test_app_698_server_GET_Request(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct message_t mesg;
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    struct security_t inf[] = {
        {0x00000200, GET_Request},
        {0x40030200, SET_Request},
        {0x20000200, ACTION_Request},
    };

    for (uint8_t i = 0; i < 3; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            mesg.from = kernel;
            mesg.to = pid_client_tx_698;
            mesg.ctrl = inf[i].cmd;;
            mesg.qos = j; //0明文/1明文+MAC/2密文/3密文+MAC
            mesg.len = 4;
            mesg.id = inf[i].oad;
            mesg.oad_n = 1;

            send_mesg(instance, NULL, &mesg, NULL);
            nidavellir_run(instance);
            //判断回应帧是否正常
        }
    }
}

