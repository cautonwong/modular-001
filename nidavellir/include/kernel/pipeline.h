#ifndef __INCLUDE_PIPELINE_H__
#define __INCLUDE_PIPELINE_H__

struct instance_t;
struct recipe_t;
struct pipeline_t;
struct message_t;

struct filter_t
{
    filter_id_t id;//过滤器id
    //成功  返回1  下一个过滤器的id为ok_id指定的值
    //返回>1 返回值为下一个过滤器的id   
    //失败时 失败时返回-1
    //返回<-1 返回值为下一个过滤器的id   
    //结束  返回0  下一个过滤器的id为err_id指定的值
    int (*on_work)(struct instance_t* instance, struct pipeline_t *pipeline, struct message_t *mesg);
    filter_id_t ok_id;
    filter_id_t err_id;
};

/*
    每个pl存在至少两个IO数据流：标准输入和标准输出
    管道pipe将一个pl的标准输出连接到另一个pl的标准输入
    其思想跟unix中的管道和重定向相似
*/
struct pipeline_t
{
    pid_t id;
    char *name;
    void *ctx;//上下文的地址
    void *in_queue;//stdin 
    void *out_queue;//stdout
    //void *data;//const data
    size_t filter_n;
    const struct filter_t *filters;//本流水线下的所有过滤器共享同一ctx
};

struct task_t
{
    pid_t id;
    char* name;
    void* std_in;
    void* task_data;
    void* task_setting;
    void (*entry)(void* task_data);
};
/*

struct pipeline_t
{
    pid_t id;
    char* name;
    void* ctx;//上下文的地址
    void* in_queue;//stdin 
    void* out_queue;//stdout
    //void *data;//const data
    int (*init)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    int (*entry)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
};*/
const struct filter_t *id_to_filter(const struct filter_t *filters,size_t n, filter_id_t id);

#endif