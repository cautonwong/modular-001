#ifndef __INCLUDE_nidavellir__
#define __INCLUDE_nidavellir__
#include "types.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include "test_utils.h"
#include "log.h"
#endif


#include <stdlib.h>
#include "time.h"
#include "event_flag.h"

#include "utils.h"
#include "list.h"
#include "fifo.h"
#include "pipeline.h"
#include "memory.h"


//to-be-deleted
extern struct fifo_t pipe_msta_rx_from_meter;//rx fifo
extern struct fifo_t pipe_msta_tx_to_meter;//tx fifo
extern struct context_645_t ctx_msta_rx_from_meter;//rx ctx
extern struct pipeline_t pl_msta_rx_from_meter; 
extern struct fifo_t pipe_msta_tx_to_meter_req;
extern struct context_645_t ctx_msta_tx_to_meter;
extern struct pipeline_t pl_msta_tx_to_meter; 
extern struct fifo_t pipe_msta_rx_from_bt;//rx fifo
extern struct fifo_t pipe_msta_tx_to_bt;//tx fifo
extern struct context_645_t ctx_msta_rx_from_bt;//rx ctx
extern struct pipeline_t pl_msta_rx_from_bt; 
extern struct fifo_t pipe_msta_tx_to_bt_req;
extern struct context_645_t ctx_msta_tx_to_bt;
extern struct pipeline_t pl_msta_tx_to_bt; 
extern struct fifo_t pipe_msta_rx_from_sta;//rx fifo
extern struct fifo_t pipe_msta_tx_to_sta;//tx fifo
extern struct context_645_t ctx_msta_rx_from_sta;//rx ctx
extern struct pipeline_t pl_msta_rx_from_sta; 
extern struct fifo_t pipe_msta_tx_to_sta_req;
extern struct context_645_t ctx_msta_tx_to_sta;
extern struct pipeline_t pl_msta_tx_to_sta; 
extern struct fifo_t pipe_msta_tx_to_bt_req;
extern struct context_645_t ctx_msta_tx_to_bt;
extern struct pipeline_t pl_msta_tx_to_bt; 
extern struct fifo_t pipe_meter_rx_from_msta;//rx fifo
extern struct fifo_t pipe_meter_tx_to_msta;//tx fifo
extern struct context_645_t ctx_meter_rx_from_msta;//rx ctx
extern struct context_698_t ctx_698;//rx ctx
extern struct context_dlms_t ctx_dlms;//rx ctx
extern struct context_698_t client_ctx_698;//rx ctx
extern struct pipeline_t pl_meter_rx_from_msta;
extern struct fifo_t pipe_meter_tx_to_msta_req;
extern struct context_645_t ctx_meter_tx_to_msta;
extern struct pipeline_t pl_meter_tx_to_msta; 
extern struct fifo_t pipe_sta_rx_from_msta;//rx fifo
extern struct fifo_t pipe_sta_tx_to_msta;//tx fifo
extern struct context_645_t ctx_sta_rx_from_msta;//rx ctx
extern struct pipeline_t pl_sta_rx_from_msta; 
extern struct fifo_t pipe_sta_tx_to_msta_req;
extern struct context_645_t ctx_sta_tx_to_msta;
extern struct pipeline_t pl_sta_tx_to_msta; 
extern struct fifo_t pipe_bt_rx_from_msta;//rx fifo
extern struct fifo_t pipe_bt_tx_to_msta;//tx fifo
extern struct context_645_t ctx_bt_rx_from_msta;//rx ctx
extern struct pipeline_t pl_bt_rx_from_msta; 
extern struct fifo_t pipe_bt_tx_to_msta_req;
extern struct context_645_t ctx_bt_tx_to_msta;
extern struct pipeline_t pl_bt_tx_to_msta; 
extern struct context_645_t ctx_msta_proxy_to_bt;
extern struct context_645_t ctx_proxy;
extern struct fifo_t pipe_log;
extern struct pipeline_t pl_log;
extern struct rtc_context_t msta_context;
extern struct fifo_t pipe_msta;
extern struct pipeline_t pl_msta;
extern struct pipeline_t pl_kernel;
typedef int (*write__)(uint32_t id, uint8_t* buf, int len);
extern struct fifo_t stdin_698;
extern struct fifo_t stdout_698;
extern struct fifo_t pipe_tx_698_req;
extern struct pipeline_t pl_rx_698;
extern struct pipeline_t pl_tx_698;
extern struct pipeline_t pl_tx_to_serial;
extern struct pipeline_t pl_rx_from_serial;
extern struct pipeline_t pl_rx_dlms;
extern struct pipeline_t pl_tx_dlms;


//
#define MSG_MAGIC  0xe8f9ab39

//to-be-deleted
#define NIDAVELLIR_EPOCH 2020

#define msta_rx_from_meter   0
#define msta_tx_to_meter   1
#define msta_rx_from_bt   2
#define msta_tx_to_bt   3
#define msta_rx_from_sta   4
#define msta_tx_to_sta   5
#define bt_rx_from_msta   6
#define bt_tx_to_msta   7
#define sta_rx_from_msta   8
#define sta_tx_to_msta   9
#define meter_rx_from_msta   10
#define meter_tx_to_msta   11
#define pid_msta   12
#define kernel   13
#define pid_rx_698   14
#define pid_tx_698   15
#define pid_rx_from_serial 16
#define pid_tx_to_serial 17
#define pid_client_rx_698   18
#define pid_client_tx_698   19
#define pid_server_rx_dlms   20
#define pid_server_tx_dlms   21

enum work_type_t 
{
    PERIOD_SEC = 0,//秒变化
    PERIOD_MIN,//分变化
    PERIOD_HOUR,//时变化
    PERIOD_MDAY,//日变化
    PERIOD_MON,//月变化
    PERIOD_WDAY,//周变化
    PERIOD_YEAR,//年变化
    //以上部分放在rtc里面去做吧
    TIMER,//定时器到期  用于超时之类的   在定时器或计数器里做
    DATA_ARRIVE,//数据到达     串口中断
    DATA_PUSH,//数据到达     主动发送请求
    //POWER_CHANGE,//电源状态发生改变
};

struct object_t
{
    object_id_t id;//对象ID
};

#ifdef WIN32
struct context_serial_t
{
    char* file_name;
    HANDLE   handle;
};
#endif

struct context_645_t
{
    //作为从站方用的临时状态
    uint8_t addr_n;
    uint8_t addr[6];
    uint8_t SA;
    uint8_t ctrl;//接收到的控制字
    uint8_t step;
    uint8_t cs;
    uint16_t len;//to-be-deleted
    uint8_t min_len;//发送时和接收时的报文总长度
    uint32_t err_n;
    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint8_t seq;//当前发送的帧序号 每次作为主站方加一
    clock_t waiting_rx;//tx的时标 tx==0 表示空闲
    uint8_t times;//重发次数
    uint8_t seconds;//超时时间
    uint16_t tx_len;
    
    //接收和发送都要用的状态
    struct object_t object;//对象标识
    struct pipeline_t *pipeline;//to-be-deleted
};



struct context_698_t
{
    int rx_count;//暂时用下to-be-deleted
    //配置信息
    struct connect_cfg_info_t *connect_info;
    struct factory_version_t *factory_version;
    //作为从站方用的临时状态
    uint16_t index;//临时计数器
    uint8_t SA;
    uint8_t LA;
    uint8_t ctrl;//接收到的控制字
    uint8_t step;
    uint8_t PIID;
    uint8_t len_unit;
    uint8_t service;
    uint8_t choice;
    uint16_t fcs;
    uint16_t hcs;
    uint16_t min_len;//发送时和接收时的报文总长度
    uint32_t err_n;
    uint8_t addr[16];
    uint8_t CA;
    int32_t fr_err;
    uint16_t segment;//分帧格式域


    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint8_t seq;//当前发送的帧序号 每次作为主站方加一
    clock_t waiting_rx;//tx的时标 tx==0 表示空闲
    uint8_t times;//重发次数
    uint8_t seconds;//超时时间
    uint16_t tx_len;

    struct fifo_t* apdu;

    //接收和发送都要用的状态
    struct object_t object;//对象标识
    struct pipeline_t* pipeline;//to-be-deleted
};

struct period_object_t
{
    uint32_t cron;
    uint32_t id;
    uint8_t len;
    uint8_t ctrl;
};

struct tasklet_t
{
    int (*deffered_work)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    uint8_t interval;
    uint8_t dcount;
    pid_t pid;//自己所属的pipeline
    
    uint8_t count;
    struct tm start;
};

#define upgrade_setsize    16
#define tasklet_setsize    1
#define bit_set(n, p)  ((p)[(n)/8] |=  (1 << ((n) & 7)))
#define bit_clr(n, p)  ((p)[(n)/8] &= ~(1 << ((n) & 7)))
#define bit_isset(n,p) ((p)[(n)/8] &   (1 << ((n) & 7)))
#define tasklet_zero(p)    memset((void*)(p),0,sizeof(*(p)))
struct tasklet_pool_t
{
    uint8_t bits[tasklet_setsize];
    struct tasklet_t tasklets[tasklet_setsize * 8];
};
struct cron_t
{
    uint8_t h;
    uint8_t f;//分和秒的频率
    uint8_t ms;//分钟的开始
    uint8_t ss;//秒的开始
};

struct rtc_context_t
{
    struct object_t object;
    // 0 -- 正常   1--故障 仍未广播对时  2--故障 对对时中
    // todo  重发  状态迁移
    // waiting[跟基表建立通信  跟STA建立通信  跟蓝牙建立通信]
    // 5min < delta < 30min  -->  启动15分钟负荷记录
    // delta > 30min  --> 时段参数 -> 费率电量 -> 15分钟负荷和日冻结
    uint8_t tm_status;
    uint8_t addr[6];//基表的地址
    
    uint8_t CCO;//CCO已对时  过日后清掉
    struct tm meter_poweroff_tm;
    struct tm meter_poweron_tm;
    uint8_t meter_type;
    uint8_t meter_rtc_is_fault;
    uint8_t bauterate;
    uint8_t current;
    uint8_t second;
};

//DTO?
struct message_hdr_t
{
    uint8_t head;
    uint8_t type;
    uint16_t len;
};
/*
#pragma pack(1)
struct message_t//command object
{
    pid_t from;
    pid_t to;
    uint8_t ctrl;//控制字  用于区分  0代表内核消息 比如中断过来的消息或信号
    uint8_t qos;//重发机制  expire
    uint8_t len;//data的长度 含id的长度
    uint32_t id;//请求的数据id
};*/
#pragma pack(1)
struct message_t//command object
{
    uint32_t magic;
    pid_t from;
    pid_t to;
    uint8_t ctrl;//控制字  用于区分  0代表内核消息 比如中断过来的消息或信号
    uint8_t qos;//重发机制  expire
    uint32_t id;//请求的数据id
    uint16_t len;//data的长度 含id的长度
    uint8_t oad_n;
    uint8_t x;
#ifdef WIN32
    int32_t err_no;
#endif // 
    uint16_t check;

};

//日志对象
struct log_object_t
{
    uint8_t subject;
    uint8_t verb;
    uint32_t object;
    clock_t when;
};
extern int send_log(uint8_t s,uint8_t v,uint32_t o);
int send_pipe(struct fifo_t* fifo, uint8_t ctrl, uint32_t id, uint8_t len, uint8_t* payload);
int recv_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* payload);
int send_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* payload);
int recv_mesg_header(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);


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
    int (*dev_initialize)(const struct dev_info_t *dev);
    int (*dev_control)(const struct dev_info_t* dev , uint8_t ctrl , void* ctrl_parameter);
    int (*dev_read)(const struct dev_info_t* dev , uint32_t addr , uint16_t len , void* src);
    int (*dev_write)(const struct dev_info_t* dev, uint32_t addr, uint16_t len, void* src);
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

struct log_item_t
{
    pid_t pipeline_id;//正处于运行态的pipeline
    struct message_t *mesg;//正在运行的pipeline收到和发送的消息
};

struct upgrade_info_t
{
    pid_t pid;//在升级的是哪个pl
    //证书信息  0x07fb02ff
    uint8_t info;//升级状态字  0x07fb0301
    uint16_t tm_remained;//升级使能剩余时间 0x07fb0302
    uint8_t bits[upgrade_setsize];//升级程序位图信息 0x07fb0303
};

struct resources_obj_t
{
    uint32_t offset;                        //地址
    uint16_t length;                        //数据存储长度
    uint8_t device_name;                    //设备标识；
    uint8_t block_name;                     //块名称
    uint8_t(*hook) (uint8_t* buf, void* p);  //resource的钩子函数
    void* p;
    uint32_t max_num;
    uint16_t unit_size;
};

#pragma pack(4)
struct resource_map_t
{
    uint8_t map_name;
    int8_t(*fun)(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
    struct list_head sibling;
};

struct mbase_control_block_t;
//聚合根
#pragma pack(4)
struct instance_t
{ 
    ///resourec需要的一些变量
    struct list_head app_list;
    struct list_head dev_list;
    struct list_head map_list;
    uint8_t ram_device_name;
    uint8_t resource_block_name;
    //////////////////////////
    uint8_t meter_type;
    uint32_t  ms;
    struct recipe_t *recipe;
    struct mbase_control_block_t * mbase_cb;
    struct tm time;//
    struct tasklet_pool_t tasklet_pool;
    struct upgrade_info_t upgrade_info;
    uint8_t* cache;
    struct cache_t* cache2;
    struct fifo_t pipe_rtc;//只能在定时器函数或时钟秒中断函数里写
    int event_group_size;
    uint8_t *event_flags;

    //user extended control block
    void* user_setting;//application_setting
    void* user_data;//application_control_block
    int (*after_poweron)(struct instance_t *instance);
};

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

struct instance_t *create_nidavellir(struct recipe_t *recipe);
/*
组织成多个pipeline,每个pipeline由多个filter组成，
pipeline之间由pipe联接,实现ipc
*/
enum block_device_id_t
{
    RES_DEV_RAM,
    RES_DEV_EEPROM,
    RES_DEV_FLASH
};


enum resource_map_698_e
{
    RESOURCE_MAP_698_GET = 1,
    RESOURCE_MAP_698_SET = 2,
    RESOURCE_MAP_698_ACTION = 3,
};



int nidavellir_loop(struct instance_t *instance);
int nidavellir_n_loop(struct instance_t **instances,size_t n);
int nidavellir_run(struct instance_t* instance);
int nidavellir_loop_n(struct instance_t *instance,int n);
int nidavellir_loop_n_with_rtc_running(struct instance_t* instance, int n);

struct pipeline_t* get_pipeline(struct instance_t* instance, pid_t id);

#endif