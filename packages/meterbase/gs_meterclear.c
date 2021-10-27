#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"

unsigned char factorymode = 0;	//工内模式=0xAA，厂外模式=0x00

struct meter_clear_ram
{
	unsigned char bootfalg;		/**< 清除升级标志*/
	unsigned char bootrstflag;	/**< 清除升级复位标志*/
	unsigned long bateusetime;	/**< 清除电池使用时间*/
	unsigned char macerrtime;	/**< 清除上一次MAC校验错误总次数超限日期 */
	unsigned char macerrcnt;	/**< 清除当日MAC校验错误总次数*/
	unsigned char keysta;		/**< 清除密钥状态0:测试密钥;1:正式密钥*/
	unsigned char freezecover;	/**< 清除冻结覆盖标志*/
	unsigned char poweroff;		/**< 清除停电模拟时间*/
	unsigned char pass02cnt;	/**< 02级密码错误次数*/
};

void event_report_oad_clear(void)
{
    unsigned char  buf[5];

    memset(buf, 0, 5);

    /**< 清除事件上报OAD列表1 */
    /**< 清除事件上报OAD列表2 */
    /**< 清除事件上报OAD列表3 */
    /**< 清除事件上报OAD列表4 */

    /*关闭上报*/
    close_event_pin(0);
}

void reactive_comb_mode_info_clear(void)
{
    unsigned char buf[7];

    memset(buf, 0, 5);
	//保留
}

/* 分项事件清零，协议调用 */
unsigned char event_clear_uint(unsigned short oad)
{
	unsigned long i;

	for (i = array_count(event_list) - 0x01; i >= 0; i--)
	{
	    if (event_list[i].sd == oad)
	    {
            //清除目标事件指针和次数
            return true;
        }
	}
    return false;
}

//事件总清
void event_clear_all(void)
{
	unsigned long i;

	for (i = array_count(event_list) - 0x01; i >= 0; i--)
	{
        //清除目标事件指针和次数
	}
}

unsigned char event_clear(unsigned short oad)
{
    /*清除跟随上报oad*/
    event_report_oad_clear();
    reactive_comb_mode_info_clear();

    if (oad)
    {
        event_clear_uint(oad);//分项清零
    }
    else
    {
        event_clear_all();	//事件总清
    }

    return true;
}

/*清除上一次结算,冻结时间*/
void clear_last_account_time(void)
{
	unsigned char time[5];
	memset((unsigned char *)&time, 0x05);
	time[3]   = 0x01;
	time[4] = 0x01;
	//清除上一日冻结时间
	//清除上一月结算时间
	//清除上一年结算时间
}


/*模板初始化*/
unsigned char board_clear(void)
{
    clear_last_account_time();
    memset((unsigned char *)&meter_clear_ram, 0x00, sizeof(meter_clear_ram));
    return true;
}


void ec_clear(void)
{
    //电量清零，清除EE数据或者效验
}

unsigned char meter_clear(void)
{
    ec_clear();
    event_clear(0);
    profile_clear();
    billing_clear();

    if (factorymode == 0x00)	//厂外模式
    {
        /*记录电能表清零事件*/
    }
    else
    {
        event_clear_uint(1);/**< 清除事件清零记录 */
        event_clear_uint(2);/**< 清除电表清零记录 */
    }
    board_clear();
    return true;
}


  

