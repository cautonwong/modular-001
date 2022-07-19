#include <stdio.h>
#include <stdarg.h>
#include "nidavellir.h"
#include "applications.h"
#include "mlog.h"

#ifdef __DEBUG_LOG__

struct tm logtime;//

void log_set_time(struct tm *time)
{
    memcpy(&logtime, time, sizeof(struct tm));
}

struct fifo_t* log_get_fifo(void)
{
    extern struct pipeline_t pl_plc_tx_698;
    struct fifo_t *tx_fifo = (struct fifo_t*)pl_plc_tx_698.out_queue;
    //extern struct pipeline_t pl_tx_698;
    //struct fifo_t *tx_fifo = (struct fifo_t*)pl_tx_698.out_queue;
    return  tx_fifo;
}
int log_print_buf(char *buf, int size, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return len;
}

void log_print(int level, const char *file, int line, const char *func, const char* fmt, ...)
{
    char buf[512+64] = {0};
    long long time = 0;

    struct fifo_t* tx_fifo = log_get_fifo();
    int len = 0;
#ifdef LOG_USE_COLOR
    if(LOG_INFO == level){
        len = log_print_buf(buf, 512, "\033[0m\033[1;32m%s:[%d]\033[0m", func, line);
    }
    else if(LOG_WARN == level){
        len = log_print_buf(buf, 512, "\033[0m\033[1;33m%s:[%d]\033[0m", func, line);
    }
    else if(LOG_ERROR == level){
        len = log_print_buf(buf, 512, "\033[0m\033[1;31m%s:[%d]\033[0m", func, line);
    }else{
        len = log_print_buf(buf, 512, "%s:[%d]:", func, line);
    }
#else
    len = log_print_buf(buf, 512, "[%d-%d-%d %d:%d:%d]%s:[%d]:", 
                        logtime.tm_year, logtime.tm_mon,logtime.tm_mday,\
                        logtime.tm_hour,logtime.tm_min,logtime.tm_sec,\
                        func, line);
#endif

    va_list ap;
    va_start(ap, fmt);
    len+= vsnprintf(buf+len, 512-len, fmt, ap);
    va_end(ap);
    
    if(len){
        fifoBuf_putData(tx_fifo, buf, len);
    }
}

void log_print_hex(int level, const char *file, int line, const char *func, uint8_t* data, int data_len)
{
    char buf[512+64] = {0};
    struct fifo_t* tx_fifo = log_get_fifo();

    int len = 0;

#ifdef LOG_USE_COLOR
    len = log_print_buf(buf, 512, "\033[0m\033[1;32m%s:[%d]\033[0m", func, line);
#else
    len = log_print_buf(buf, 512, "[%d-%d-%d %d:%d:%d]%s:[%d]:", 
                        logtime.tm_year, logtime.tm_mon,logtime.tm_mday,\
                        logtime.tm_hour,logtime.tm_min,logtime.tm_sec,\
                        func, line);
#endif
    if(data_len > 256){
       data_len = 256;
    }
    
    for(int i = 0; i < data_len; i++){
        log_print_buf(buf+len, 512-len,"%02x", data[i]);
        len+=2;
    }
    len += log_print_buf(buf+len, 512-len,"\n");
    if(len){
        fifoBuf_putData(tx_fifo, buf, len);
    }
}

#endif
