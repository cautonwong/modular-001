#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"
extern int app_698_rx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_698_tx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int close_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_tx_data_to_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);



void test_app_698_server_class_id_1_eng(void** state)
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

