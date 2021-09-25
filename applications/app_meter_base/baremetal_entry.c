#include "kernel\nidavellir.h"
//dependencies
    //1.user define
    //2.mesg[秒]
struct mbase_control_block_t
{
    int id;
    //电量数据
    //瞬时量数据
    //需量数据
    //事件负荷冻结数据
    //费率数据
    //费用数据
};
struct mbase_control_block_t mbase_cb;
int meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景
    switch (mesg->ctrl)
    {
    case 0x86://采样的外设数据抵达
        //refresh_control_data(instance,pipeline,mesg);
        break;
    case 0x87://外设控制命令  比如校表  可以异步的话就单独弄个tx_entry?
        //controlDevice(instance,pipeline,mesg);
        break;
    case 0x88://control_block控制命令 比如清零
        //controlData(instance,pipeline,mesg);
        break;
    case 0x89://秒信号  刷新内部计时器  比如需量和事件
        //controlStatus(instance,pipeline,mesg);
        break;
    default:
        break;
    }

    return 0;
}

struct filter_t fr_meter_base[] =
{
    {0,meter_base_entry,1,13},
};

struct pipeline_t pl_meter_base =
{
    2,
    "meter_tx_to_msta",
    NULL,
    &pipe_sta_tx_to_msta_req,
    &pipe_msta_rx_from_sta,
    sizeof(fr_meter_base) / sizeof(fr_meter_base[0]),
    fr_meter_base,
};

