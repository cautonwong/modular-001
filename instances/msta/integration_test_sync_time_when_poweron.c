#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

/*
电能表时钟故障（超差大于5或10分钟）时，运维模块启动本地费率电量统计功能（基础数据每分钟从基表定时获取一次）。运维模块每15分钟（起始时间以模块中的RTC3为准）读取正向有功总电量和反向有功总电量，通过计算15分钟电量增量来计算费率电量。按照15分钟间隔，应冻结正向有功总及费率电能、反向有功总及费率电能，且存储不少于3天的数据量。
电能表时钟正常（超差小于等于5或10分钟）时，运维模块可不启动本地费率电量统计功能。CCO抄读日冻结时，直接以基表日冻结数据为准。
电能表时钟故障（超差30分钟）或RTC3与基表不一致（偏差＞5秒）时，运维模块应启动以RTC3为时钟基准的负荷记录功能。
运维模块基于RTC3每15分钟读取电能表当前电压、电流、频率；有、无功功率；功率因数；有功总和分相电能；无功总电能；四象限无功总电能；当前需量；零线电流。采用运维模块RTC3的时间作为负荷记录的时标，且存储不少于3天的数据量。
*/
void integration_test_sync_time_when_poweron(void** state)
{
    extern struct rtc_context_t msta_context;

    struct message_t mesg;
    struct instance_t* instance;
    instance = (struct instance_t*)*state;
    struct tm  meter_rtc;//基表时钟
    struct tm  meter_poweroff_time;// 基表掉电记录发生时刻 YYMMDDhhmmss
    struct tm  meter_poweron_time;// 基表掉电记录结束时刻 YYMMDDhhmmss
    struct tm  meter_battery_off_time;//基表电池欠压发生时刻
    struct tm  cco_tm =
    {
        .tm_year = 2021 - 1900,   /* years are counted from 1900 */
        .tm_mon = 3,                      /* 0 = January, 11 = December */
        .tm_mday = 30,
        .tm_hour = 23,
        .tm_min = 59,
        .tm_sec = 57
    };

    memcpy(&instance->cco_tm, &cco_tm,sizeof(struct tm));
    struct test_case_t 
    {
        //条件
        int meter_type;//用于对时阈值的选择 5分钟  还是 10分钟
        int msta_last_poweroff_delta;//msta上次的掉电时刻  相对cco的时钟偏差  单位秒
        int msta_delta;//msta的初始时钟相对cco的时钟偏差  单位秒
        int meter_delta;//meter的初始时钟相对cco的时钟偏差 单位秒
        int meter_rtc_fault_type;//meter的时钟故障类型  0--未故障 1-- 时钟发生倒退 2--格式错乱  3--上电时刻小于掉电时刻 4--掉电时间1000天
        int meter_poweroff_tm_delta;//meter的掉电发生时刻  相对cco的时钟偏差  单位秒
        int meter_poweron_tm_delta;//meter的掉电结束时刻 相对cco的时钟偏差  单位秒
        //期望结果
        int use_meter_rtc;//1表示采用基表时钟作为msta的时钟  0采用上次掉电时钟作为msta的时钟
        int is_fault;//是否故障  
        int is_msta_started;//运维功能是否已启动
        int cco_broadcasted;//cco给msta对时
        int sync_time_ok;//对时后  三者时钟是否一致
    };

//        int bt_delta;//附近的电表时钟
//        int days_needed;//需要几天才能对回来
//        int cco_broadcasted;//cco给msta对时
//        int bt_broadcasted;//bt给msta对时
//        int is_bt_called;//是否需要调用蓝牙广播给附近的表
//        int read_meters_nearby_via_bt;//是否需要调用蓝牙进行时钟表决
//    };

    struct test_case_t cases[] =
    {
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、时钟未失效(时钟偏差≤5分钟)；期望：基表时钟作为MSTA时钟，且MSTA时钟=基表时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO已对时、时钟未失效(时钟偏差≤5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=基表时钟=CCO时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时成功、时钟未失效(时钟偏差≤5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=基表时钟=CCO时钟

        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、时钟未失效(5分钟＜时钟偏差≤30分钟)；期望：基表时钟作为MSTA时钟，且MSTA时钟=基表时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO已对时、时钟未失效(5分钟＜时钟偏差≤30分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=CCO时钟，基表时钟=基表时钟-5分钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时成功、时钟未失效(5分钟＜时钟偏差≤30分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=CCO时钟，基表时钟=基表时钟-5分钟

        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、时钟未失效(时钟偏差≤-5分钟)；期望：基表时钟作为MSTA时钟，且MSTA时钟=基表时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO已对时、时钟未失效(时钟偏差≤-5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=基表时钟=CCO时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时成功、时钟未失效(时钟偏差≤-5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=基表时钟=CCO时钟

        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、时钟未失效(-30分钟＜时钟偏差＜-5分钟)；期望：基表时钟作为MSTA时钟，且MSTA时钟=基表时钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO已对时、时钟未失效(-30分钟＜时钟偏差＜-5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=CCO时钟，基表时钟=基表时钟+5分钟
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时成功、时钟未失效(-30分钟＜时钟偏差＜-5分钟)；期望：CCO时钟作为MSTA时钟，且MSTA时钟=CCO时钟，基表时钟=基表时钟+5分钟

        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、蓝牙未对时，时钟故障(时钟偏差＞30分钟，或＜-30分钟、或非法), 掉电时间合法(有CCO对时记录)；期望：掉电时间作为MSTA时钟，基表时钟维持
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO未对时、蓝牙已对时，时钟故障(时钟偏差＞30分钟，或＜-30分钟、或非法), 掉电时间合法(有CCO对时记录)；期望：蓝牙时钟作为MSTA时钟，基表时钟维持
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA上电或者复位，阀值5分钟、CCO已对时、时钟故障(时钟偏差＞30分钟，或＜-30分钟、或非法); 期望：CCO时钟作为MSTA时钟，基表时钟维持
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时成功、时钟故障(时钟偏差＞30分钟，或＜-30分钟、或非法); 期望：CCO时钟作为MSTA时钟，基表时钟维持
        {5, -300, 180,-180, 0, -300,-200, 0, 0},//MSTA正常运行，阀值5分钟、每日CCO广播校时失败、蓝牙对时成功，时钟故障(时钟偏差＞30分钟，或＜-30分钟、或非法); 期望：蓝牙时钟作为MSTA时钟，基表时钟维持
    };
    uint8_t buf[256];
    uint32_t ts;
    struct test_case_t *this_case;

    ts = rtc_mktime(&cco_tm);

    generate_data(buf, 200, 6, 0);

    will_return(__return_error,-4);
    will_return(__return_error,-4);
    will_return(__return_error, -4);
    will_return(__return_error, -4);
    will_return(__return_error, -4);
    will_return(__return_error, -4);
    will_return(__return_error, -4);
    will_return(__return_error, -4);

    mesg.from = kernel;
    mesg.to = pid_msta;
    mesg.ctrl = 0x01;//上电
    mesg.id = 0x0000000;
    mesg.len = 4;
    send_mesg(instance, &pl_kernel, &mesg, NULL);
    nidavellir_loop_n_with_rtc_running(instance, 20);
    assert_true(msta_context.meter_type == 0xff);//适应失败

    for (int i = 0; i < sizeof(cases) / sizeof(struct test_case_t); i++)
    {
        init_test_environment();
        will_return(__return_error, 6);
        mock_id_data_ok(0x04000401, 6); //让基表回复抄读表地址成功  6字节
        this_case = &cases[i];

        rtc_localtime(ts + this_case->msta_last_poweroff_delta, &instance->last_poweroff_tm);
        rtc_localtime(ts + this_case->msta_delta, &instance->time);
        if (this_case->meter_rtc_fault_type == 2)
        {
            //时钟格式错乱
            mock_id_data_ok(0x03110101, 6); 
            mock_id_data_ok(0x03111201, 6);
            mock_id_data_ok(0x22000001, 6);
            mock_id_data_ok(0x0400010c, 6);
        }
        else
        {
            rtc_localtime(ts + this_case->meter_delta, &instance->meter_rtc);
            rtc_localtime(ts + this_case->meter_poweroff_tm_delta, &meter_poweroff_time);
            rtc_localtime(ts + this_case->meter_poweron_tm_delta, &meter_poweron_time);
            rtc_localtime(ts, &meter_battery_off_time);
            mock_time(&meter_poweroff_time, 0x03110101);// 基表掉电记录发生时刻 YYMMDDhhmmss
            mock_time(&meter_poweron_time, 0x03111201);//基表掉电记录结束时刻 YYMMDDhhmmss
            mock_time(&meter_battery_off_time, 0x22000001);//基表电池欠压发生时刻
            mock_time(&instance->meter_rtc, 0x0400010c);//YYMMDDWWhhmmss
        }

        mesg.from = kernel;
        mesg.to = pid_msta;
        mesg.ctrl = 0x01;//上电
        mesg.id = 0x0000000;
        mesg.len = 4;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        nidavellir_loop_n_with_rtc_running(instance, 10);
        assert_true(msta_context.meter_type == 0x01);

        if (this_case->meter_rtc_fault_type == 0)
        {
            //未故障
            //则将基表时钟作为msta的时钟
            assert_true(rtc_tm_compare(&instance->meter_rtc, &instance->time) == 0);
        }
        else
        {
            //故障
            // 电表运行状态字1（从电表抄读）的时钟故障位置1
            // 上报时钟故障
            //先将最近一次掉电时间（应判断有效性）重写入时钟芯片,再等待CCO广播校时或MSTA通过通信（如：BLE）获取邻近电表的时钟
            //则将最近一次掉电时间重写入模块的RTC时钟， 并等待入网成功后重新获得CCO时钟
            assert_true(rtc_tm_compare(&instance->last_poweroff_tm, &instance->time) == 0);
        }

        stop_tasklet(instance, 0);

        //CCO 对msta广播对时
        mesg.from = kernel;
        mesg.to = sta_tx_to_msta;
        mesg.ctrl = 0x08;
        mesg.id = 0x00000001;
        mesg.len = 10;
        time_to_array_0(&instance->cco_tm, buf, 6);
        send_mesg(instance, &pl_kernel, &mesg, buf);

        msta_context.CCO = 0;
        //MSTA应判断广播校时命令，对30分钟内的多个相同数值的广播对时
        //nidavellir_loop_n_with_rtc_running(instance, 40);
        nidavellir_loop_n(instance, 80);
        //确保三者时钟已一致
        assert_true(rtc_tm_compare(&instance->time, &instance->cco_tm) == 0);
        assert_true(rtc_tm_compare(&instance->meter_rtc, &instance->cco_tm) == 0);
    }
}
