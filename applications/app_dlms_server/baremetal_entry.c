#include "nidavellir.h"
#include "dlms\nx_dlms.h"

/*
## 资源消耗情况
  - 内存消耗分析:  <3.5K
    - 串口接收FIFO 544
    - 串口发送FIFO 544
    - 接收别的pl的主动发送FIFO  200
    - 用来装apdu的cache fifo  2048
    - ctx
*/
struct hdlc_control_block_t ctx_dlms;
struct fifo_t dlms_pdu;

int fr_rx_hdlc_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx_dlms.apdu = &dlms_pdu;
    uint16_t len = 0;
    ctx->step = 0;
    ctx->rx_count++;
#ifdef WIN32
    printf("%s rx try[%d]:\n", get_pipeline_name(instance, pipeline->id),ctx->rx_count);
#endif
    int r = ringbuffer_filter(fifo, __hdlc_filter, ctx);
    if (r > 0)
    {
#ifdef WIN32
        uint8_t buf[256];
        printf("%s catch:", get_pipeline_name(instance, pipeline->id));
        len = fifoBuf_getDataPeek(fifo, buf, r);
        for (size_t i = 0; i < len; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("\n");
#endif
        //client�Ľ���
        ringbuffer_clear(ctx->apdu);
        mesg->len = 0;
        len = fifoBuf_end(fifo, packet_len(ctx->rx_len));
        //serve_hdlc()
        switch (ctx->ctrl)
        {
        case DLMS_COMMAND_SNRM:
            r = serve_snrm(instance, pipeline, mesg);
            break;
        case DLMS_COMMAND_DISC:
            r = serve_disc(instance, pipeline, mesg);
            break;
        case DLMS_COMMAND_RELEASE_REQUEST:
            r = serve_release_request(instance, pipeline, mesg);
            break;
        default:
            r = serve_iframe(instance, pipeline, mesg);
            break;
        }
        
        //�쳣Ӧ��
        while (fifo->rd != len + 2)
        {
            fifoBuf_getByte(fifo);
        }

        /*response 
        1. 应答的命令在mesg->ctrl
        2. 应答的数据在ctx->apdu中
        3. 应答的数据长度在mesg->len
        */
        if (mesg->len > 0)
        {
            fr_pack_hdlc_head(instance, pipeline, mesg);
            fr_pack_hdlc_apdu(instance, pipeline, mesg);
            fr_pack_hdlc_tail(instance, pipeline, mesg);
        }

        return 1;
    }

#ifdef WIN32
    printf("%s error: %s\n", get_pipeline_name(instance, pipeline->id), error_strings[-r]);
#endif
    return 0;
}


const struct filter_t fr_rx_dlms[] =
{
    {0x00,fr_rx_hdlc_entry,1,13},
};

struct pipeline_t pl_rx_dlms =
{
    pid_server_rx_dlms,
    "pl_rx_dlms",
    &ctx_dlms,
    &stdin_698,//serial driver isr put byte into this 
    &stdout_698,//pl_rx_dlms put apdu into this 
    sizeof(fr_rx_dlms) / sizeof(fr_rx_dlms[0]),
    fr_rx_dlms,
};

const struct filter_t fr_tx_dlms[] =
{
    {0x01,fr_rx_hdlc_entry,1,13},
};

struct pipeline_t pl_tx_dlms =
{
    pid_server_rx_dlms,
    "pl_tx_dlms",
    &ctx_dlms,
    &pipe_tx_698_req,
    &stdout_698,
    sizeof(fr_tx_dlms) / sizeof(fr_tx_dlms[0]),
    fr_tx_dlms,
};

//一个进程可以包含多个线程
//一个线程可以包含多个pipeline
#ifdef PROCESS_THIS
//进程资源声明
//module_start()
#endif

#ifdef THREAD_THIS
//线程资源声明 比如栈大小/优先级 等  和同步手段比如信号量  队列  eventflag等
//tx_application_define
    //  queue as signal delivering message 
    //  pipeline entry as thread entry
#endif