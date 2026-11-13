#ifndef ND_KERNEL_ENVIRONMENT_H
#define ND_KERNEL_ENVIRONMENT_H

#include <stddef.h>
#include <stdint.h>

#include "object.h"

struct pipeline_t;

struct kernel_context_t
{
    struct object_t object;
};

struct app_context_t
{
    struct object_t object;
};

struct nd_env_t
{
    size_t app_n;
    //filters symbles
    //app entry area 
};

struct nd_app_t
{
    int (*install)();
    int (*uninstall)();
    uint8_t pipeline_n;
    struct pipeline_t **pipelines;
    //todo  MF info
};
typedef uint32_t type_t;
typedef uint32_t offset_t;
//I/O request packet
struct irp_t
{
    type_t type; //请求类型
    //struct device_t *device; //请求的哪个设备
    offset_t offset;         //请求偏移(针对块设备)
    uint8_t *buf;            //responce data
    size_t size;             //request size;responce size通过返回值返回
    void *param;             //request ctrl block 指向全局数据结构  例如串口参数控制结构
};


struct nidavellir_t
{
    int (*entry)();//0x2000
    int (*before_sleep)();
    int (*after_sleep)();
    // code layout   来源于链接脚本
        /*
            1.boot
            2.rt (kernel目录下的内容)
            3.env(fd注册表+worker注册表)
            4.workers()
        */
    // memory layout 来源于链接脚本
        /*
            0.kernel space
            1.pipeline-context space
            2.ipc space
            3.power-safe-data space
            4.cache space
            5.stack space 
        */
    // device layout  来源于程波
        /*
            1.eeprom
            2.flash
        */
};

#endif
