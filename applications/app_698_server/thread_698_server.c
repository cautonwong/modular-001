#include "tx_api.h"

//tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
//        pointer, DEMO_STACK_SIZE, 
//        1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

void  thread_698_server_entry(ULONG thread_input)
{
    UINT    status;

    while(1)
    {
        //wait_for_mesg
        app_plc_rx_entry();//接收到消息后，调用前后台的task_entry
    }
}
