#ifndef ND_KERNEL_DEVICE_H
#define ND_KERNEL_DEVICE_H

#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "object.h"

struct fifo_t;
struct pipeline_t;
struct driver_t;
struct instance_t;

struct bus_ops_t
{
    size_t bus_type;
    size_t bus_index;
    int (*init)(size_t index);
    void (*tx_enable)(uint8_t en , size_t index);
    int (*tx)(uint8_t byte  , size_t index);
    void (*rx_enable)(uint8_t en , size_t index);
    int (*rx)(uint8_t* byte , size_t index );
    int (*tx_statue)(size_t index);
    int (*rx_statue)(size_t index);
    void (*ctrl)(uint8_t ctrl , size_t index); //对于SPI总线的START 表示要拉低CS， STOP表示要拉高CS。对于IIC总线START 和 STOP 分别表示起始和结束
};

struct dev_info_t;
struct driver_ops_t
{
    int (*dev_initialize)(struct instance_t* instance, const struct dev_info_t *dev);
    int (*dev_control)(struct instance_t* instance, const struct dev_info_t* dev , uint8_t ctrl , void* ctrl_parameter);
    int (*dev_read)(struct instance_t* instance, const struct dev_info_t* dev , uint32_t addr , uint16_t len , void* src);
    int (*dev_write)(struct instance_t* instance, const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src);
};


struct dev_sys_ops_t
{
    int (*dev_input_isr)();
    int (*dev_output_isr)();
    int (*dev_status)(struct dev_info_t* dev);
    int (*dev_termination)(struct dev_info_t* dev);
    int (*dev_entry)(struct dev_info_t* dev);
};

struct dev_info_t
{
    size_t dev_type;
    size_t dev_index;
    size_t data_size;
    size_t dev_timer_type;
    size_t dev_timer_id;
    const struct bus_ops_t *dev_bus_ops;
    const struct driver_ops_t *dev_driver_ops;//给资源管理器用
    const struct dev_sys_ops_t *dev_sys_ops;  //给系统用
    struct fifo_t* dev_input_fifo;
    struct fifo_t* dev_output_fifo;
    const void *cfg;
#ifdef WIN32
    char* name;
#endif
};
//EEPROM，FLASH，EMU，红外，RS4851，RS4852，PLC，RTC

struct hw_def_t
{
    uint32_t name;
    dev_id_t id;
//    struct resource_map_t *map;
    struct driver_t *driver;
    //propery;
};

struct pipe_t
{
    pipe_id_t id;
    struct object_t obj;
    void *pipe;//fifo
};

#define max_pipeline  10

#pragma pack(1)
struct recipe_t
{
    uint32_t name;//产品编号
    uint32_t version;
    //const struct layout_t *layout;//to be deleted
    //uint32_t layout_n;
    struct hw_def_t *hw_def;
    uint16_t hw_def_n;
    uint8_t pipeline_n;
    struct pipeline_t **pipelines;
    //哪个pipeline在什么情况(时间/周期/事件/信号等)下将什么数据打到哪个fifo中去
    //struct router_t *routes;
};
#pragma pack()

#endif
