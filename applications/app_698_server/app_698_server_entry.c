#include "nidavellir.h"

int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_gatekeeper_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_gatekeeper_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
//某路uart的接收服务应用程序  调用和重排package的api
//跟该路uart强相关的事务在此函数内完成
//要线程化时  可将此函数用while(1)包住 即可完成前后台到os的打包过程
static int fr_rx_num = 0;
int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    ctx->fr_err = 0;
    //1.接入校验  接收缓冲中有数据 
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, mesg);
    if (ctx->fr_err > 0)
    {
        fr_rx_num++;
        //2.switch case 处理各通信命令
        ctx->fr_err = fr_gatekeeper_server(instance, pipeline, mesg);
        return ctx->fr_err;
    }
    return 0;
}

//某路uart的主动发送服务应用程序 比如主动上报  主动抄表  停电上报等
int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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


int app_rx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    ctx->fr_err = 0;
    //1.接入校验  接收缓冲中有数据 
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, mesg);
    if (ctx->fr_err > 0)
    {
        //2.switch case 处理各通信命令
        ctx->fr_err = fr_gatekeeper_client(instance, pipeline, mesg);
        return ctx->fr_err;
    }

    return 0;
}

int app_tx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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



