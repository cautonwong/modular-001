#ifndef __M_LOG_H__
#define __M_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "time.h"

#ifdef __DEBUG_LOG__

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };
void log_print(int level, const char *file, int line, const char *func, const char* fmt, ...);
void log_print_hex(int level, const char *file, int line, const char *func, uint8_t* data, int data_len);
void log_set_time(struct tm *time);

#define log_debug(...)      log_print(LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_info(...)       log_print(LOG_INFO,  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_warn(...)       log_print(LOG_WARN,  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_err(...)      log_print(LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_hex(buf,len)    log_print_hex(LOG_INFO, __FILE__, __LINE__, __FUNCTION__, buf,len)
#else

#define log_debug(...)
#define log_info(...)
#define log_warn(...)
#define log_err(...)
#define log_hex(buf,len)

#endif

//是否开启仿真终端打印 模式
#define __PRINT_DEBUG   0    //不开启
//#define __PRINT_DEBUG 1    //开启
#if __PRINT_DEBUG
#include "stdio.h"
#define log_printf(...) printf(__VA_ARGS__)
#else
#define log_printf(...)
#endif


#endif // __M_LOG_H__