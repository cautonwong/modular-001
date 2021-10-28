#include "kernel\nidavellir.h"
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <WinBase.h>

struct context_serial_t* create_serial_port(char* filename)
{
    struct context_serial_t* serial_port = 
        (struct context_serial_t*)malloc(sizeof(struct context_serial_t));
    serial_port->file_name = filename;

    return serial_port;
}

int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    int nError;
    struct context_serial_t* ctx = (struct context_serial_t*)pipeline->ctx;

    ctx->handle = CreateFile((ctx->file_name),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (ctx->handle == (HANDLE)-1)
    {
        nError = GetLastError();
        return nError;
    }

    GetCommState(ctx->handle, &dcb);
    dcb.BaudRate = 9600;
    //dcb.BaudRate = (DWORD)find_device_baud_rate_default((uint16)dev_id);
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(ctx->handle, &dcb);
    SetupComm(ctx->handle, 1024, 1024);
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = MAXDWORD;// 20;// 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;// 2;
    timeouts.ReadTotalTimeoutConstant = 0;// 1000;
    timeouts.WriteTotalTimeoutMultiplier = 50;// 1;// 50;
    timeouts.WriteTotalTimeoutConstant = 50;//1;// 2000;
 
    SetCommTimeouts(ctx->handle, &timeouts);
    return 0;
}

int close_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_serial_t* ctx = (struct context_serial_t*)pipeline->ctx;
    ctx->handle = (HANDLE)-1;
    return 0;
}

int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_serial_t* ctx = (struct context_serial_t*)pipeline->ctx;

    DWORD rx_len = 128;
    uint8_t buf[128];
    if (ctx->handle == (HANDLE)-1)
    {
        return 0;
        return GetLastError();
    }

    ReadFile((_In_ LPDWORD)(ctx->handle), buf, 128, (_Out_opt_ LPDWORD) &rx_len, 0);

    if (rx_len > 0)
    {
        fifoBuf_putData(pipeline->out_queue, buf, rx_len);
        return 1;//再来一次
    }

    return 0;
}

int fr_tx_data_to_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_serial_t* ctx = (struct context_serial_t*)pipeline->ctx;

    uint8_t buf[128];
    DWORD tx_len = 128;
    DWORD len = 0;
    if (ctx->handle == (HANDLE)-1)
    {
        return 0;
        return GetLastError();
    }

    tx_len = fifoBuf_getData(pipeline->in_queue, buf, 128);
    if (tx_len > 0)
    {
        WriteFile(ctx->handle, buf, tx_len, (_Out_opt_ LPDWORD) & len, NULL);

#ifdef VERBOSE
        printf("                   {%s,send,%s", get_pipeline_name(instance, pipeline->id), get_pipeline_name(instance, mesg->from));
        printf(",");
        for (size_t i = 0; i < tx_len; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("}}\n");
#endif
        return 1;//再来一次
    }
    return 0;
}



