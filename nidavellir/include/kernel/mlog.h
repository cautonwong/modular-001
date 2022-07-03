#ifndef __M_LOG_H__
#define __M_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "time.h"
#ifdef LOG_DEBUG_EN

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };
void log_print(int level, const char *file, int line, const char *func, const char* fmt, ...);
int log_set_time(struct tm *time);

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


#endif // __M_LOG_H__