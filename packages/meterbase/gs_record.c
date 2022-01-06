#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "fr_698.h"

// 获取事件参数配置表
int get_event_para_pack(struct instance_t* instance, uint32_t oad, struct event_para_pack_t *para_pack)
{
    struct mbase_control_block_t* mbase_cb;

    uint16_t OI = ((uint16_t)((oad & 0xFFFF0000) >> 16));

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    switch (OI) //
    {
        case 0x3005:// 过流
        {
            struct para_698_tag_t OverCurrentAS_para[2] =
            {
                {TYPE_double_long,      (uint8_t*)&mbase_cb->OverCurrent_threshold,      4},
                {TYPE_unsigned,         (uint8_t*)&mbase_cb->OverCurrent_duration_para,  1},
            };
            
            memcpy((uint8_t*)para_pack->event_para, (uint8_t*)OverCurrentAS_para , sizeof(OverCurrentAS_para));
            para_pack->para_num = sizeof(OverCurrentAS_para)/ sizeof(OverCurrentAS_para[0]);
            return 1;
        }
        break;

    default:
        break;
    }
}




uint8_t get_activeflag(struct pipeline_t* task, uint16_t RecordId)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    if (ctrl_block->active_flag[RecordId / 8] & (1 << (RecordId % 8)))
    {
        return true;
    }
    return false;

}
void set_activeflag(struct pipeline_t* task, uint16_t RecordId)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    ctrl_block->active_flag[RecordId / 8] |= (1 << (RecordId % 8));
}

void clr_activeflag(struct pipeline_t* task, uint16_t RecordId)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    ctrl_block->active_flag[RecordId / 8] &= ~(1 << (RecordId % 8));
}

void set_recordflag(struct pipeline_t* task, uint16_t RecordId)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    ctrl_block->record_flag[RecordId / 8] |= (1 << (RecordId % 8));
}

void clr_recordflag(struct pipeline_t* task, uint16_t RecordId)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;

    ctrl_block->record_flag[RecordId / 8] &= ~(1 << (RecordId % 8));
}


int event_duration_pro(uint8_t duration_para, uint8_t* duration, uint8_t state, struct pipeline_t* task, uint16_t RecordId)
{
    uint8_t activeflag = get_activeflag(task, RecordId);

    if (state) // 发生
    {
        if (true == activeflag)
        {
            *duration = duration_para;
        }

        if (*duration < duration_para)
        {
            (*duration)++;
        }
        else if (false == activeflag)
        {
            set_activeflag(task, RecordId);
            return 1; // 记录发生
        }
    }
    else // 结束
    {
        if (false == activeflag)
        {
            *duration = 0;
        }

        if (*duration > 0)
        {
            (*duration)--;
        }
        else if (true == activeflag)
        {
            clr_activeflag(task, RecordId);
            return -1; // 记录结束
        }
    }

    return 0; // 不记录
}




























//记录接口
//发送记录标识
void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint16_t FlagId, uint8_t* payload)
{

    mesg->from = kernel;
    mesg->to = pid_resource_manager;
    mesg->ctrl = 0;
    mesg->qos = 0;
    mesg->len = 4;
    mesg->id = FlagId;
    mesg->oad_n = 1;

    set_recordflag(task, mesg->id);

    send_mesg(instance, NULL, mesg, payload);
}

//事件记录接口
// mode 1：记录发生   0 不记录。 -1 记录结束
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint16_t RecordId, uint8_t* payload, int mode)
{
    if (mode == 1) // 发生
    {
        send_recordflag_mesg(instance, task, mesg, RecordId, NULL);
    }
    else if (mode == -1) //结束
    {
        send_recordflag_mesg(instance, task, mesg, RecordId + 1, NULL);
    }
}

// 上电运行期间 每秒的调用
int record_judge(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	ia_over_event_judge(instance, pipeline, mesg);
    CoverOpen_judge(instance, pipeline, mesg);
}


//冻结或事件清零 接口    传入要清除的oad 
int clr_record(struct instance_t* instance, uint32_t oad)
{
    uint32_t temp = 0;
    struct record_storage_id_info_t info;
    int ret = get_record_id_info(instance, oad, &info);

    if (ret < 0)
    {
        return -1;
    }

    //清记录
    nd_log_clean(instance, info.save_index);
    //清序号
    nd_set_index(instance, info.sequence_index, &temp, 4);

    //累计时间清零
    if (info.acc_time)
    {
        *info.acc_time = 0;
    }
}
