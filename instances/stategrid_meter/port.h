#ifndef __INSTANCE_PORT_H__
#define __INSTANCE_PORT_H__
//!!!移植时的两种做法之设想!!!
//1. 静态移植  
//删减功能时的步骤  a.去掉下面的某些字段(难点在于还得去掉该字段对应的外设资源)  b. 编译   c. 去掉编译不过的源文件
//添加功能时的步骤  a.在extension中添加字段  b. 增加处理该字段的api源文件   c. 添加该源文件到工程中 d.编译
//2，动态移植
//大量使用链表和环境变量   
#define instance_info           uint32_t            pt;\
                                uint32_t            ct;\

#define U_data_extension1       

#define U_data_extension0       uint32_t  d_time[4];   /**< 日总检测时间 */ \
                                uint16_t  d_ok_rate[4]; /**< 日合格率 */ \
                                uint16_t  d_bad_rate[4];/**< 日超限率 */ \
                                uint32_t  d_up_time[4];/**< 日超上限时间 */ \
                                uint32_t  d_low_time[4];/**< 日超下限时间 */ \
                                uint32_t  m_time[4];/**< 月总检测时间 */ \
                                uint16_t  m_ok_rate[4];/**< 月合格率 */ \
                                uint16_t  m_bad_rate[4];/**< 月超限率 */ \
                                uint32_t  m_up_time[4]; /**< 月超上限时间 */ \
                                uint32_t  m_low_time[4];/**< 月超下限时间 */ \
                                uint16_t  U0;\
                                uint16_t  U1;\
                                uint16_t  U2;\
                                uint16_t  U3;\

#define U_data_fields           uint16_t    Ua;\
                                uint16_t    Ub;\
                                uint16_t    Uc; \
                                U_data_extension0 \
                                U_data_extension1

//过流事件控制块
#define I_over_fields           uint8_t    Ia_over_duration;\
                                int8_t     Ib_over_duration;\
                                int8_t     Ic_over_duration;/*dyn:当前相的过流状态持续时间*/\
                                int32_t    I_over;/*static:电能表过流事件-电流触发下限 */ \
                                uint8_t    I_over_duration;/*static:电能表过流事件-判定延时时间 */\

#define Ia_data_fields          int32_t    Ia;\
#define Ib_data_fields          int32_t    Ib;\
#define Ic_data_fields          int32_t    Ic;/*dyn:c相电流*/\

#define I_common_extension

#define I_common_fields         I_over_fields \
                                I_common_extension

#define I_data_fields           I_common_fields\
                                Ia_data_fields\
                                Ib_data_fields\
                                Ic_data_fields\

#define alert_ctrl_extension    uint8_t     alert_status[4];  \
#define local_ctrl_extension    
//报警相关字段  报警继电器  报警灯  背光
#define alert_ctrl_fields       uint32_t    alert_duration;   \
                                uint32_t    alert_relay_ctrl_word;  \
                                uint32_t    backlight_ctrl_word;   \
                                alert_ctrl_extension
//远程费控字段
#define remote_ctrl_fields      uint8_t      reason; 
//本地费控字段
#define local_ctrl_fields       uint8_t      status; /* 当前状态  0正常 1 达到报警门限1 2 达到报警门限2  3 透支 */  \
                                uint32_t     rate_price;  /* 当前费率电价 */   \
                                uint32_t     stair_price; /* 当前阶梯电价 */    \
                                uint32_t     current_price; /* 当前电价 */        \
                                int32_t      threshold1;    /* 账户告警阀值1 */ \
                                int32_t      threshold2;    /* 账户告警阀值2 */  \
                                int32_t      credit_quota;  /* 透支额度 */  \
                                int32_t      hoard_quota;   /* 囤积额度 */   \
                                int32_t      close_quota;   /* 合闸允许额度 */   \
                                uint8_t      stair_mode;     /* 当前运行阶梯模式 0: 月阶梯模式; 1:年阶梯模式 */  \
                                uint8_t      stair_num_running; /* 当前运行阶梯 */ \
                                int32_t      balance_int;  /* XXXXXX.XX 当前余额的整数部分 */   \
                                int32_t      balance_decimal; /* XXXXXX 厘数部分 */  \
//负荷控制相关字段
#define load_ctrl_fields        uint8_t     load_ctrl_status; \
                                remote_ctrl_fields  \
                                local_ctrl_fields 

//!!!设想!!!
//每个app的control block分区,每个分区在存储介质中存在独立的实际分区，以便支持页对齐地整块读写。分别是
//1.掉电保存区且上电需要读出初始化的区域  如电量数据
//2.上电需要初始化但掉电不需要保存的区域  如参数
//3.上电需要清零的区域                  如中间状态数据
//存在的问题
//1. 要跟协议ID做个映射map
//2. 数据兼容性

#define static_start                 uint8_t static_start;\
#define static_end                   uint8_t static_end;\

#define dyn_cache_start              uint8_t dyn_cache_start;\
#define dyn_cache_end                uint8_t dyn_cache_end;\

#define dyn_persistent_start         uint8_t dyn_persistent_start;\
#define dyn_persistent_end           uint8_t dyn_persistent_end;\

#endif