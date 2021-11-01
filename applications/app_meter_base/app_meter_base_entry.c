#include "nidavellir.h"
#include "applications.h"

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景
    for (int i = 0; i < max_event_flag; i++)
    {
        if (event_flags_get(instance,i) ==  1)
        {
            //call event_handler(instance,pipeline,mesg);

            event_flags_clr(instance, i);
        }
    }

    switch (mesg->ctrl)
    {
    case 0x86://采样的外设数据抵达
        //backlight_pro();
        break;
    case 0x87:
        break;
    case 0x88://control_block控制命令 比如清零
        //controlData(instance,pipeline,mesg);
        break;
    default:
        break;
    }

    //sync_data()
    return 0;
}



