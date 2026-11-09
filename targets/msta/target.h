#ifndef __TARGET_H__
#define __TARGET_H__

#ifdef WIN32
    #define __kernel_space__size__  0x400
    #define __pipeline_context_space__size__  0x4000
    #define __ipc_space__size__  0x4000
    #define __power_safe_space__size__  0x2000
    #define __app_space__size__  0x0000
    #define __cache_space__size__  0x2000
    #define __stack_space__size__  0x2000
    #define __ram_size__ (__kernel_space__size__+__pipeline_context_space__size__+__ipc_space__size__+__power_safe_space__size__+__app_space__size__+__cache_space__size__+__stack_space__size__)

    #define __kernel_space__start__  0x0000
    #define __kernel_space__end__  (__kernel_space__start__ + __kernel_space__size__)
    #define __pipeline_context_space__start__  __kernel_space__end__
    #define __pipeline_context_space__end__  (__pipeline_context_space__start__ + __pipeline_context_space__size__)
    #define __ipc_space__start__  __pipeline_context_space__end__
    #define __ipc_space__end__  (__ipc_space__start__ + __ipc_space__size__)
    #define __power_safe_space__start__  __ipc_space__end__
    #define __power_safe_space__end__  (__power_safe_space__start__ + __power_safe_space__size__)

    #define __cache_space__start__  __power_safe_space__end__
    #define __cache_space__end__  (__cache_space__start__ + __cache_space__size__)
    #define __stack_space__start__  __cache_space__end__
    #define __stack_space__end__  (__stack_space__start__ + __stack_space__size__)
#endif

#endif