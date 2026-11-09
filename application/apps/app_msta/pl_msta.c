#include "nidavellir.h"

extern int update_instance_rtc(struct instance_t* instance);
extern int stop_tasklet(struct instance_t* instance, int tid);
extern int start_tasklet(struct instance_t* instance, int tid);

const struct period_object_t crontab[] =
{
        {0x00002300, 0x0506ff01,100},
        {0x18f00001, 0x0201FF00,6},//电压  xxx.x
        {0x18f00003, 0x0202FF00,9},//电流  xxx.xxx
        {0x18f00005, 0x02800001,3},//零线电流  xxx.xxx
        {0x18f00007, 0x02800002,2},//电网频率  xx.xx
        {0x18f00009, 0x0203ff00,12},//瞬时有功功率 xx.xxxx
        {0x18f0000b, 0x0204ff00,12},//瞬时无功功率 xx.xxxx
        {0x18f0000d, 0x0206ff00,6},//功率因素 x.xxx
        {0x18f0000f, 0x0001ff00,20},//正向有功电能 xxxxxx.xx
        {0x18f00011, 0x00160000,4},//A反向有功电能 xxxxxx.xx
        {0x18f00013, 0x002a0000,4},//B反向有功电能 xxxxxx.xx
        {0x18f00015, 0x003e0000,4},//C反向有功电能 xxxxxx.xx
        {0x18f00017, 0x0002ff00,20},//反向有功电能 xxxxxx.xx
        {0x18f00019, 0x00150000,4},//A相正向有功电能 xxxxxx.xx
        {0x18f0001b, 0x00290000,4},//B相正向有功电能 xxxxxx.xx
        {0x18f0001d, 0x003d0000,4},//C相正向有功电能 xxxxxx.xx
        {0x18f0001f, 0x00030000,4},//组合无功1总电能 xxxxxx.xx
        {0x18f00021, 0x00040000,4},//组合无功2总电能 xxxxxx.xx
        {0x18f00023, 0x00050000,4},//第一象限无功总电能 xxxxxx.xx
        {0x18f00025, 0x00060000,4},//第二象限无功总电能 xxxxxx.xx
        {0x18f00027, 0x00070000,4},//第三象限无功总电能 xxxxxx.xx
        {0x18f00029, 0x00080000,4},//第四象限无功总电能 xxxxxx.xx
        {0x18f0002b, 0x02800004,3},//当前有功需量 xx.xxxx
        {0x18f0002d, 0x02800005,3},//当前无功需量 xx.xxxx
        {0x18f0002f, 0x0219ff00,6},//基波电压相角数据块 xxx.x
        {0x0100001b, 0x0400010c,7},//日期、星期(其中0代表星期天)及时间 YYMMDDWWhhmmss
        //{0x18001427, 0x0400010c},//广播校时
        {0x18f00101, 0x03300000,3},//编程总次数  xxxxxx
        {0x18f00103, 0x03300001,50},//上一次编程记录 YYMMDDhhmmss C0C1C2C3 XXXXXXXX*10 不足补FF
        {0x18f00105, 0x04000201,1},//年时区数  NN <=14
        {0x18f00107, 0x04000202,1},//日时段数  NN <=8
        {0x18f00109, 0x04010000,3*14},//第一套时区表数据 起始日期及日时段表号  MMDDNN
        {0x18f0010b, 0x04010001,3*14},//第一套第1日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0010d, 0x04010002,3 * 14},//第一套第2日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0010f, 0x04010003,3 * 14},//第一套第3日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00111, 0x04010004,3 * 14},//第一套第4日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00113, 0x04010005,3 * 14},//第一套第5日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00115, 0x04010006,3 * 14},//第一套第6日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00117, 0x04010007,3 * 14},//第一套第7日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00119, 0x04010008,3 * 14},//第一套第8日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0011b, 0x04020000,3 * 14},//第二套时区表数据 起始日期及日时段表号  MMDDNN
        {0x18f0011d, 0x04020001,3 * 14},//第二套第1日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0011f, 0x04020002,3 * 14},//第二套第2日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00121, 0x04020003,3 * 14},//第二套第3日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00123, 0x04020004,3 * 14},//第二套第4日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00125, 0x04020005,3 * 14},//第二套第5日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00127, 0x04020006,3 * 14},//第二套第6日时段表数据 起始时间及费率号 hhmmNN
        {0x18f00129, 0x04020007,3 * 14},//第二套第7日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0012b, 0x04020008,3 * 14},//第二套第8日时段表数据 起始时间及费率号 hhmmNN
        {0x18f0012d, 0x04000206,1},//谐波分析次数  NN
        {0x18f0012f, 0x04000207,1},//梯度数 NN
        {0x18f00201, 0x040005ff,2*7},//电表运行状态字 xxxx
        //{0x18100031, 0x0201ff00},//电压数据块
};
const size_t nnn = sizeof(crontab) / sizeof(crontab[0]);
struct rtc_context_t msta_context;

int fr_msta_entry(struct instance_t *instance, struct pipeline_t *pipeline, struct message_t *mesg)
{
    if(recv_mesg_header(instance, pipeline, mesg))
    {
        return mesg->ctrl;//返回收到的消息类型  以便跳转到对应的消息处理函数
    }

    return 0;
}

//系统消息
// 0 --- 上电
// 1 --- rtc更新
int fr_handle_kernel_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = pipeline->out_queue;
    struct rtc_context_t* ctx = (struct rtc_context_t*)pipeline->ctx;
    if (mesg->ctrl == 0x01)
    {
        //收到rtc更新信号  更新软时钟
        if (mesg->id == 0x00000001)
        {
            update_instance_rtc(instance);
            return 1;//跳转到qos判断 定时抄表  费率电能统计  超时和重发
        }
        //收到上电信号  抄读基表地址  
        if (mesg->id == 0x00000000)
        {
            ctx->meter_type = 0xff;
            mesg->ctrl = 0x13;
            mesg->id = 0x04000401;
            mesg->len = 4;
            mesg->to = msta_tx_to_meter;
            send_mesg(instance, pipeline, mesg, NULL);
            start_tasklet(instance,0);//启动超时重发机制
            return 0;
        }
    }

    return -1;
}

int make_mesg(struct message_t* mesg,uint8_t ctrl,uint32_t id,uint8_t len)
{
    mesg->ctrl =  ctrl;
    mesg->id = id;
    mesg->len = len;
    return 0;
}

int fr_handle_08_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = pipeline->out_queue;
    struct rtc_context_t* ctx = (struct rtc_context_t*)pipeline->ctx;
    uint8_t buf[200];
    //收到CCO的广播对时 
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, buf, mesg->len - 4);
    }
    if (mesg->id == 0x00000001)
    {
        if (!ctx->CCO)
        {
            ctx->CCO = 1;//已对时
            array_0_to_time(&instance->time, buf, mesg->len - 4);
        }
        //给基表对时  todo差值比较大时 循环对时
        time_to_array_0(&instance->time, buf, 6);
        make_mesg(mesg, 0x08, 0x00000000, 10);
        mesg->to = msta_tx_to_meter;
        send_mesg(instance, pipeline, mesg, buf);
        return 0;
    }
    //收到BT过来的时钟
    if (mesg->id == 0x00000002)
    {
        //要看是否cco对时过了
        return 0;
    }
}

int fr_handle_93_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = pipeline->out_queue;
    struct rtc_context_t* ctx = (struct rtc_context_t*)pipeline->ctx;
    uint8_t buf[200];

    if(mesg->id == 0x04000401)
    {
        if (mesg->len == 0x0a)
        {
            fifoBuf_getData(pipeline->in_queue, buf, mesg->len -4);
            //todo 识别基表类型 用来判断时钟阈值
            //todo 比对地址  保存地址
            __write(instance, mesg->id, buf, mesg->len-4);
            memcpy(ctx->addr,buf,6);
            ctx->meter_type = 1;//基表类型
            stop_tasklet(instance, 0);
            make_mesg(mesg, 0x11, 0x03110101, 4);//启动时钟合法性判断任务
            mesg->to = msta_tx_to_meter;
            send_mesg(instance, pipeline, mesg, NULL);
        }
    }
    return 0;
}

int fr_handle_91_mesg(struct instance_t *instance, struct pipeline_t *pipeline, struct message_t *mesg)
{
    struct fifo_t* fifo = pipeline->in_queue;
    struct rtc_context_t *ctx = (struct rtc_context_t *)pipeline->ctx;
    uint8_t buf[200];
    if (clock() > 1000)
    {
    }
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, buf, mesg->len - 4);
    }

    switch (mesg->id)
    {
    case 0x04000204://基表费率数
//        assert(mesg->len == 5);
        __write(instance, mesg->id, buf, mesg->len - 4);
        instance->meter_fee_n = buf[0];
        break;
    case 0x0400010c://收到了来自基表的时钟
        __write(instance, mesg->id, buf, mesg->len - 4);
        array_0_to_time(&instance->meter_rtc, buf, mesg->len);

        //判断时钟是否正常  
        // 正常  --> 写入rtc3d
        if (!ctx->CCO)
        {
            if (rtc_tm_valid(&instance->meter_rtc))
            {
                array_0_to_time(&instance->time, buf, mesg->len-4);
            }
            else
            {
                ctx->meter_rtc_is_fault = 1;
                memcpy(&instance->time, &instance->last_poweroff_tm, sizeof(struct tm));
            }
        }
        make_mesg(mesg, 0x11, 0x04000204, 4);//启动参数同步  读取费率数
        mesg->to = msta_tx_to_meter;
        send_mesg(instance, pipeline, mesg, NULL);
        mock_id_data_ok(0x04000204, 1);
        break;

    case 0x03110101://基表掉电记录发生时刻
        __write(instance, mesg->id, buf, mesg->len - 4);
        array_0_to_time(&ctx->meter_poweroff_tm, buf, mesg->len);
        if (rtc_tm_valid(&ctx->meter_poweroff_tm))
        {
        }
        else
        {
            ctx->meter_rtc_is_fault = 1;
            memcpy(&instance->time, &instance->last_poweroff_tm, sizeof(struct tm));
        }
        make_mesg(mesg, 0x11, 0x03111201, 4);
        mesg->to = msta_tx_to_meter;
        send_mesg(instance, pipeline, mesg, NULL);
        break;
    case 0x03111201://基表掉电记录结束时刻
        __write(instance, mesg->id, buf, mesg->len - 4);
        array_0_to_time(&ctx->meter_poweron_tm, buf, mesg->len);
        if (rtc_tm_valid(&ctx->meter_poweron_tm))
        {

        }
        else
        {
            ctx->meter_rtc_is_fault = 1;
        }
        make_mesg(mesg, 0x11, 0x22000001, 4);
        mesg->to = msta_tx_to_meter;
        send_mesg(instance, pipeline, mesg, NULL);
        break;
    case 0x22000001://基表电池欠压发生时刻
        __write(instance, mesg->id, buf, mesg->len - 4);
        make_mesg(mesg, 0x11, 0x0400010c, 4);
        mesg->to = msta_tx_to_meter;
        send_mesg(instance, pipeline, mesg, NULL);
        break;
    default:
        __write(instance, mesg->id, buf, mesg->len - 4);//todo 费率统计 负荷记录  日冻结 上报
        break;
    }

    return 0;
}

int fr_handle_11_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = pipeline->out_queue;
    struct rtc_context_t* ctx = (struct rtc_context_t*)pipeline->ctx;
    uint8_t buf[200];
    if (mesg->len > 4)
    {
        fifoBuf_getData(pipeline->in_queue, buf, mesg->len - 4);
    }

    //todo 高频数据的打包和压缩读取  0xec8XXXXX
    switch (mesg->id)
    {
    case 0x0400010c:
        break;
    default:
        __read(instance, mesg->id, buf, mesg->len - 4);
        make_mesg(mesg, 0x91, mesg->id, 4);
        mesg->to = msta_tx_to_sta;
        mesg->len = 4;
        send_mesg(instance, pipeline, mesg, buf);
        break;
    }
    return 1;
}

int fr_handle_periodic_task(struct instance_t *instance, struct pipeline_t *pipeline, struct message_t *mesg)
{
    
    //while(1)
    {
        const struct period_object_t *p;
        struct cron_t cron;
        uint32_t id;
        uint8_t mf = 0;
        uint8_t sf = 0;
        struct rtc_context_t *ctx = (struct rtc_context_t *)pipeline->ctx;
        if (ctx->meter_type == 0xff)
        {
            //没获取到基表通信地址
            return -1; 
        }
        else 
        {
            for (size_t i = 0; i < nnn; i++)
            {
                p = &crontab[i];
                cron.h = ((p->cron) >> 24);
                cron.f = ((p->cron) >> 16);
                cron.ms = ((p->cron) >> 8);
                cron.ss = ((p->cron)) & 0xff;
                id = p->id;
                mf = (cron.f >> 4) & 0x0f;
                sf = cron.f & 0x0f;

                if (cron.h != 24)
                {
                    if (instance->time.tm_hour != cron.h)
                    {
                        continue;
                    }
                }

                if (mf != 0)
                {
                    if ((instance->time.tm_min >= cron.ms) && (((instance->time.tm_min - cron.ms) % mf) != 0))
                    {
                        continue;
                    }
                }
                else
                {
                    if (instance->time.tm_min != cron.ms)
                    {
                        continue;
                    }
                }

                if (sf != 0)
                {
                    if ((instance->time.tm_sec >= cron.ss) && (((instance->time.tm_sec - cron.ss) % sf) != 0))
                    {
                        continue;
                    }
                }
                else
                {
                    if (instance->time.tm_sec != cron.ss)
                    {
                        continue;
                    }
                }

                ctx->current = i;
                mesg->ctrl = 0x11;
                mesg->id = p->id;
                mesg->len = 4 ;
                mesg->to = msta_tx_to_meter;
                mock_id_data_ok(p->id, p->len);
                send_mesg(instance, pipeline, mesg, NULL);
                return 0;
            }
        }
        return 0;
    }
}

int fr_run_tasklets(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct tasklet_t* tasklet;
    for (int i = 0; i < tasklet_setsize * 8; i++)
    {
        if (bit_isset(i, instance->tasklet_pool.bits))
        {
            tasklet = &instance->tasklet_pool.tasklets[i];
            if (tm_diff(&instance->time, &tasklet->start) >= tasklet->interval)
            {
                tasklet->deffered_work(instance,pipeline,mesg);
                tasklet->count++;
                memcpy(&tasklet->start, &instance->time, sizeof(struct tm));
                if ((tasklet->dcount != 0) && (tasklet->count >= tasklet->dcount))
                {
                    bit_clr(i, instance->tasklet_pool.bits);
                }
            }
        }
    }
    return 1;
}

int __fr_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint8_t buf[200];
    if (mesg->len > 4)
    {
        //fifoBuf_getData(pipeline->in_queue, buf, mesg->len - 4);
    }

    return 0;
}

const struct filter_t fr_msta[] =
{
    {0x00, fr_msta_entry, 0x01, 0x13},            
    {0x01, fr_handle_kernel_mesg, 0x02, 0x13},
    {0x91, fr_handle_91_mesg, 0x13, 0x13},
    {0x93, fr_handle_93_mesg, 0x13, 0x13},
    {0x11, fr_handle_11_mesg, 0x13, 0x13},
    {0x08, fr_handle_08_mesg, 0x13, 0x13},
    {0x02, fr_run_tasklets,   0x03, 0x13},
    {0x03, fr_handle_periodic_task, 0x13, 0x13},
    {0x13, __fr_end, 0x0, 0x00},//todo  未被匹配到的消息要清掉
}; 
struct fifo_t pipe_msta;

struct pipeline_t pl_msta =
{
    11,
    "msta",
    &msta_context,
    &pipe_msta,
    &pipe_msta_tx_to_meter_req,
    sizeof(fr_msta) / sizeof(fr_msta[0]),
    fr_msta,
};

