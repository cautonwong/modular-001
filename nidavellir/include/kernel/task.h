#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

struct instance_t;
typedef void (*filter_t)(struct instance_t* instance, struct task_t* task, struct message_t* mesg);

//如何实现每个filter的资源自动管理
filter_t __mbase_filters_on_command[] =
{
    NULL
    //更新瞬时量数据
    //更新电量数据
    //更新需量数据
    //更新瞬时量状态
    //更新事件状态
    //更新系统故障状态
    //更新报警状态(报警继电器/背光/报警led)
};

//过滤器模式和职责链模式和观察者模式和消息路由
//0x00  系统消息
//0x01  timer
//0x02  command
//0x03  query
struct filter_chain_t
{
    //head(0xa5) from(1) to(1) type(1) ts(4) len(2) data(len) tail（0x5a） 
    uint32_t mesg_id;//消息编号 
    uint8_t* bits;//当前链上的过滤器个数
    filter_t* filters;//关注此消息编号的过滤器列表  不关心顺序  职责呢？
};

/*
每个task存在至少两个IO数据流：标准输入和标准输出
管道pipe将一个task的标准输出连接到另一个task的标准输入
其思想跟unix中的管道和重定向相似
*/
struct task_t
{
    pid_t id;  
    #ifdef WIN32
    char* name;
    #endif
    void* std_in;
    void* std_out;
    void* control_block;
    void (*entry)(struct instance_t* instance, struct task_t* task,struct message_t* mesg);
    struct filter_chain_t* filter_chain;
    struct instance_t* instance;
    struct list_head siblings;
};

int set_std_in(struct task_t* task,void* std_in);
int set_std_out(struct task_t* task, void* std_out);
int set_control_block(struct task_t* task, void* control_block);
int get_std_in(struct task_t* task, void* std_in);
int get_std_out(struct task_t* task, void* std_out);
int get_control_block(struct task_t* task, void* control_block);
int task_init(struct instance_t* instance,struct task_t* task);
int task_add_filter(struct task_t* task, uint32_t mesg_id,filter_t filter);
int send_kernel_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data);
int send_timeout_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data);
int send_command_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data);
int send_query_mesg(struct instance_t* instance, struct task_t* task, struct message_t* mesg, uint8_t* data);
int recv_mesg_v2(struct instance_t* instance, struct task_t* task, struct message_t* mesg);
#endif