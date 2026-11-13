#ifndef ND_KERNEL_SERIAL_H
#define ND_KERNEL_SERIAL_H

#ifdef WIN32
#include <windows.h>
struct context_serial_t
{
    char* file_name;
    HANDLE handle;
};
#endif

#endif
