#include "nidavellir.h"
#include "applications.h"



// 上电运行期间 每秒的调用 
int record_judge(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	ia_over_event_judge(instance, pipeline, mesg);
}


//冻结或事件清零 接口    传入要清除的oad 
int clr_record(struct instance_t* instance, uint32_t oad)
{
    uint32_t temp = 0;
    struct record_storage_id_info_t info;
    int ret = get_record_id_info(oad, &info);

    if (ret < 0)
    {
        return -1;
    }

    //清记录
    nd_log_clean(instance, info.save_index);
    //清序号
    nd_set_index(instance, info.sequence_index, &temp, 4);
}
