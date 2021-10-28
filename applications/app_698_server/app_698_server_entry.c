#include "kernel\nidavellir.h"

int update_instance_rtc(struct instance_t* instance)
{
    uint32_t ts;
    ts = rtc_mktime(&instance->time);
    ts += 1;
    rtc_localtime(ts, &instance->time);
    return 0;
}

//某路uart的接收服务应用程序  调用和重排package的api
//跟该路uart强相关的事务在此函数内完成
//要线程化时  可将此函数用while(1)包住 即可完成前后台到os的打包过程
int app_698_rx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    //1.接入校验  接收缓冲中有数据 
    if (fr_rx_698_entry(instance, pipeline, mesg))
    {
        //2.switch case 处理各通信命令
        return fr_gatekeeper_server(instance, pipeline, mesg);
    }

    return 0;
}

//某路uart的主动发送服务应用程序
//比如主动上报  主动抄表  停电上报等
int app_698_tx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    //1.接收别的app或者系统发来的上报请求
    if (fr_tx_698_entry(instance, pipeline, mesg))
    {
        //2.组帧  
        //3.使能串口发送
        return fr_gatekeeper_client(instance, pipeline, mesg);
    }
    return 0;
}

