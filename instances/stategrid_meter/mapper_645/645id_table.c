#include "nidavellir.h"
#include "645id_table.h"
#include "port.h"
#include "utils.h"
#include "gs_record.h"
#include "fr_645_report.h"

typedef uint8_t (*data_check_fun)(struct instance_t* instance, uint32_t addr, uint8_t *, uint8_t *);
extern uint8_t write_mcu_rtc_time(uint8_t addr, uint8_t *buf, uint16_t len);
extern uint8_t write_mcu_rtc_date(uint8_t addr, uint8_t *buf, uint16_t len);
extern unsigned char GetWeek645( unsigned char* Addr );
extern const  unsigned char FirmWareVersion[20];
extern unsigned long SoftwareCHK(void);
extern int rn8209c_set( struct instance_t* instance, uint8_t *p_rx );
extern uint8_t rn8209c_get( struct instance_t* instance, uint8_t *p_rx, uint8_t* p_tx);
extern uint8_t get_current_plc_baud(struct instance_t* instance);

const COMMTAB  CommTab[] =
{
    0x04000101,	0xFFFF,				            FORMAT_01,      CommData_WR + CommData_RAM + CommData_SECONDLY + 4,//日期及星期
    0x04000102,	0xFFFF,                         FORMAT_02,      CommData_WR + CommData_RAM + CommData_SECONDLY + 3,//时间
    0x0400010C,	0xFFFF,                         FORMAT_03,      CommData_WR + CommData_RAM + CommData_SECONDLY + 7,//日期、星期、时间
    0x04000106,	SeasonTable_Changetime,         FORMAT_04,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 5,//两套时区表切换时间
    0x04000107,	DayTable_Changetime,            FORMAT_04,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 5,//两套时段表切换时间
//    0x04000108, ToupriceTable_Changetime,       FORMAT_04,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 5,	//两套分时费率切换时间
//    0x04000109, StepZone_Changetime,            FORMAT_04,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 5,	//两套阶梯切换时间
    
    0x04000201, SeasonDay_Para,                 FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,   //年时区数(<=14)
    0x04000202, SeasonDay_Para+1,               FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,   //日时段表数(<=8)
    0x04000203, SeasonDay_Para+2,               FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,   //日时段数(<=14)
    0x04000204, SeasonDay_Para+3,               FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,   //费率数(<=12)
    0x04000205, SeasonDay_Para+4,               FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,   //公共假日数
    0x04000207, Step_Num,                       FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,   //阶梯数
    
    0x04000301, 0xFFFF,	 						FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//自动循环显示屏数
	0x04000302, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//自动循环显示每屏显示时间
    0x04000303, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//显示电能小数位数
	0x04000304, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//显示功率小数位数
	0x04000305, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//按键循环显示屏数
	0x04000306, Meter_CT,                       FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 3,	//CT
    0x04000307, Meter_PT,                       FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 3,	//PT
    0x04000308, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//全屏显示时间
    
    0x04000401, 0xFFFF,                         FORMAT_05,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 6,	//表地址
    0x04000402, 0xFFFF,                         FORMAT_07,      CommData_WR + CommData_E2PROM + CommData_PROGRAM + 6,	//表号
    0x04000403, ASCII_AssetsMan,				FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 32,//资产管理编码
    0x04000404, ASCII_Un,						FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 6,//额定电压
	0x04000405, ASCII_Ib,						FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 6,//额定电流
	0x04000406, ASCII_Imax,						FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 6,//最大电流
	0x04000407, ASCII_AccuracyClass1,			FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 4,//计量精度
	0x04000409, Meter_Const1,					FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 3,//脉冲常数
	0x0400040B, ASCII_MeterType,                FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 10,//电表型号
	0x0400040C, Produced_Date645,				FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 10,//生产日期
	0x0400040D, ASCII_ptl645_Version,           FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 16,//协议版本号
	0x0400040E, User_Number,                    FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 6,	//客户编号
    0x0400040F, 0xFFFF,					        FORMAT_09,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 11,//电能表位置信息
    
    0x04000501,	0xFFFF,                  	    FORMAT_08,      CommData_R + CommData_RAM + 2,//电表运行状态字1
	0x04000502,	0xFFFF,                  	    FORMAT_08,      CommData_R + CommData_RAM + 2,//电表运行状态字1
	0x04000503,	0xFFFF,                  	    FORMAT_08,      CommData_R + CommData_RAM + 2,//电表运行状态字3
	0x04000504,	0xFFFF,                  	    FORMAT_08,      CommData_R + CommData_RAM + 2,//电表运行状态字4	
	0x04000507,	0xFFFF,						    FORMAT_08,      CommData_R + CommData_RAM + 2,//电表运行状态字7
    0x040005FF,	0xFFFF,						    FORMAT_08,      CommData_R + CommData_RAM + 14,//电表运行状态字1~7
    
    0x04000601, Measure_Mode,					FORMAT_0C,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//有功组合方式字
    
    0x04000705, 0xFFFF,							FORMAT_08,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//PLC波特率
	0x04000703, 0xFFFF,							FORMAT_08,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//485通信波特率特征字
	0x04000701, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//红外通信速率
    
    0x04000801, Weekend,                        FORMAT_0C,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//周休日特征字
    0x04000802, WeekendFee,                     FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//周休日采用的日时段表号
    
    0x04000901, LoadRec_Mode,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//负荷记录模式字
    0x04000902, TimeFroze_Mode,                 FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//定时冻结模式字
	0x04000903, InsFroze_Mode,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//瞬时冻结模式字
	0x04000904, AppointFroze_Mode,              FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//约定冻结模式字
	0x04000905, HourFroze_Mode,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//整点冻结模式字
	0x04000906, DayFroze_Mode,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//日冻结模式字
    
    0x04000A01, LoadRec_Time,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 4,//负荷记录起始时间
	0x04000A02, LoadRec_Inv1,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//负荷记录间隔时间
	0x04000A03, LoadRec_Inv2,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//负荷记录间隔时间
	0x04000A04, LoadRec_Inv3,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//负荷记录间隔时间
	0x04000A05, LoadRec_Inv4,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//负荷记录间隔时间
    
    0x04000B01, Balance_Time,                   FORMAT_0A,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//结算日1
	0x04000B02, Balance_Time+1,                 FORMAT_0A,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//结算日2
	0x04000B03, Balance_Time+2,                 FORMAT_0A,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//结算日3
    
    0x04000C03,	Meter_02Password,				FORMAT_00,      CommData_W + CommData_E2PROM + CommData_SECONDLY + 4,//02级密码
	0x04000C05,	Meter_04Password,				FORMAT_00,      CommData_W + CommData_E2PROM + CommData_SECONDLY + 4,//04级密码
	
//    0x04001001, Alarm_Credit1,                  FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 4,    //报警金额1
//    0x04001002, Alarm_Credit2,                  FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 4,    //报警金额2
    0x04001101, MeterRunWord,					FORMAT_0B,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,  //电表运行特征字1
    0x04001104, 0xFFFF,                         FORMAT_08,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 8,  //主动上报模式字

    0x04001201,	HourFroze_StartTime,            FORMAT_00,	    CommData_WR + CommData_E2PROM + CommData_SECONDLY + 5,  //整点冻结
    0x04001202,	HourFroze_Inv,                  FORMAT_00,	    CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,  //整点冻结间隔时间
	0x04001203,	DayFroze_Time,                  FORMAT_00,	    CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,  //日冻结时间
	0x04001401,	Remote_RelayOffTime,            FORMAT_06,	    CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,  //拉闸延时时间
    0x04001402,	ImaxProtectValue,				FORMAT_06,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 3,  //过流拉闸门限值
    0x04001403,	ImaxProtectTime,				FORMAT_06,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,  //过流拉闸延时时间
    0x04001404, IrAuthenTime,                   FORMAT_06,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,  //红外认证时间
    0x04001405, ReportRstTime,					FORMAT_00,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,  //主动上报复位延时时间

    0x04001501, 0xFFFF,					        FORMAT_10,		CommData_R  + 12,                                       //主动上报状态字

    0x04001503, 0xFFFF,					        FORMAT_0F,		CommData_W  + 12,                                       //复位主动上报状态字
    
    0x04100901, 0xFFFF,							FORMAT_08,		CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//按键循环显示每屏显示时间
    
    0x04090801, 0xFFFF,							FORMAT_08,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 2,//过流事件电流阈值
    0x04090802, 0xFFFF,							FORMAT_08,      CommData_WR + CommData_E2PROM + CommData_SECONDLY + 1,//过流事件延时时间
    
    0x04800001, ASCII_SoftWareNo,				FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 32,//软件版本号
	0x04800002, ASCII_HardWareNo,				FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 32,//硬件版本号
	0x04800003, ASCII_Factor,					FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 32,//工厂编号
	0x04800004, SoftWareNo_645,                 FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 8,//软件备案号
    
    0xA6000300,	ADJ_Pgain0,                     FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 8,	//火线计量参数
	0xA6000301,	ADJ_Pgain1,						FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 8,	//零线计量参数
	0xA6000303,	ADJ_K,							FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 24,	//k系数
	0xA6000305,	ADJ_Other,						FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 4,	//脉冲速率和启动阈值
	0xA6000306,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 4,	//表内计量常数
	0xA6000307,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 18,	//交采规格配置
	0xA6000309,	Un_Cmp_Value,                   FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 8,	//电压影响量补偿
	0xA6011054, ResetCount,                     FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 4,//复位次数
	0xA601100F, 0xFFFF,							FORMAT_08,      CommData_WR + CommData_RAM + 4, //校验码
    
	0xA8000100,	0xFFFF,							FORMAT_08,      CommData_WR + CommData_RAM + 1,	    //工厂模式
	0xA8000105, 0xFFFF,							FORMAT_00,      CommData_W + CommData_FACTORY + 0,  //默认参数初始化
	
	0xA8001001,	(unsigned int)FirmWareVersion,  FORMAT_00,      CommData_R + CommData_RAM + 20,//厂家内部软件版本号    
	0xA8F00002,	0xFFFF,					        FORMAT_08,      CommData_WR + CommData_E2PROM + 4,	//背光报警控制字
	0xA8F00003,	0xFFFF,						    FORMAT_08,      CommData_WR + CommData_RAM + 2,	//电能表类型、功能配置字
	0xA8F00051,	0xFFFF,							FORMAT_08,      CommData_WR + 2,	//掉电主动上报的阈值时间
	0xA8F00052,	0xFFFF,						    FORMAT_08,      CommData_WR + CommData_E2PROM + 1,	//小电流不滤除 AA开启
    0xA8F00053, 0xFFFF,                         FORMAT_00,      CommData_WR + 6, //停电时间（测试补冻结用）
	0xA8F000FF,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_E2PROM + 0xFF,//万能读写

    0xA8000200, 0xFFFF,							FORMAT_00,      CommData_WR + CommData_E2PROM + CommData_FACTORY + 8,
	0xA8000202, 0xFFFF,							FORMAT_00,      CommData_W + CommData_FACTORY+ 0,	//初始化校表参数
    0xA8000203,	ADJ_Pgain0,                     FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 8,//A相校表参数
	0xA8000204,	ADJ_Pgain1,                     FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 8,//B相校表参数
    0xA8000208, 0xFFFF,							FORMAT_00,      CommData_W + CommData_FACTORY + 52,	//台体瞬时数据 交采包
    0xA800020A,	ADJ_K,							FORMAT_00,      CommData_WR + CommData_E2PROM_BP + CommData_FACTORY + 20,//K系数
    0xA800020B, 0xFFFF,							FORMAT_00,      CommData_W + CommData_FACTORY + 8,	//小信号交采包
	0xA800020C, 0xFFFF,							FORMAT_0E,      CommData_WR + CommData_FACTORY + 5,	//计量芯片万能读写
	0xA8000101,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_RAM + CommData_FACTORY + 1,//生产测试模式
	0xA8000102,	0xFFFF,							FORMAT_00,      CommData_W + CommData_RAM + CommData_FACTORY + 1,//测试命令
	0xA8000103,	0xFFFF,							FORMAT_00,      CommData_R + CommData_RAM + 6,//自检状态字
    
    0xA8FFFFF0,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_FACTORY + 4,//正向电能设置底度
    0xA8FFFFF1,	0xFFFF,							FORMAT_00,      CommData_WR + CommData_FACTORY + 4,//反向电能设置底度
};

static uint8_t dl645rd_date(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint8_t temp=0;
    
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_wday);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_mday);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_mon + 1);
    temp = instance->time.tm_year + 1900 - 2000;
    *databuf++ = BYTE_BCD((uint8_t)temp);
    
    return 0;
}

static uint8_t dl645rd_time(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_sec);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_min);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_hour);
    return 0;
}

static uint8_t dl645rd_datetime(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint8_t temp=0;
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_sec);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_min);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_hour);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_wday);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_mday);
    *databuf++ = BYTE_BCD((uint8_t)instance->time.tm_mon + 1);
    temp = instance->time.tm_year + 1900 - 2000;
    *databuf++ = BYTE_BCD((uint8_t)temp);
    return 0;
}

static uint8_t dl645rd_chgtime(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t addr1=0;
    uint16_t temp=0;
    uint8_t buf[7] = {0,};
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_get_index(instance, addr1, buf, 7);
    
    *databuf++ = BYTE_BCD(buf[5]);  
    *databuf++ = BYTE_BCD(buf[4]);
    *databuf++ = BYTE_BCD(buf[3]);
    *databuf++ = BYTE_BCD(buf[2]);
    mem_swap_cpy((uint8_t *)&temp, buf, 2);
    if(temp >= 2000)
        temp = temp - 2000;
    *databuf++ = BYTE_BCD((uint8_t)temp);
    
    return 0;
}

static uint8_t dl645rd_meteraddr(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    memcpy(databuf, mbase_cb->tsa, 6);
    
    return 0;
}

static uint8_t dl645rd_hex_bcd(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t addr1=0;
    uint32_t temp=0;
    uint8_t buf[4] = {0,},len=0, storage_len = 0;
    
    if((addr == ImaxProtectValue) || (addr == Meter_CT) || (addr == Meter_PT) 
       || (addr == IrAuthenTime) || (addr == Meter_Const1))    //拉闸电流门限需要从4字节转成3字节
    {
        len = 4;
        storage_len = 4;
    }
    else if((addr == (SeasonDay_Para+4)))    //协议2字节 存储1字节
    {
        len = 2;
        storage_len = 1;
    }
    else
    {
        len = *length;
        storage_len = len;
    }
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_get_index(instance, addr1, buf, storage_len);
    mem_swap_cpy((uint8_t*)&temp, buf, storage_len);
    
    if(addr == ImaxProtectValue)
    {
        temp = temp/10;
    }
    HEX_BCD(databuf,temp);
    
    return 0;
}

static uint8_t dl645rd_meterno(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mem_swap_cpy(databuf, mbase_cb->meterNo, 6);
    return 0;
}

const uint8_t baud645[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const uint8_t baud698[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x00};
static uint8_t dl645rd_ram(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t id=0,StAddr=0, s_index = 0;
    uint8_t i=0,j,index=0, Baut = 0;
    uint16_t sate=0,sate1=0;
    
    memcpy((uint8_t *)&id, databuf-4, 4);
    switch(id)
    {
        case 0x04000301:    //轮显屏数
            *databuf = BYTE_BCD(mbase_cb->lcd_autodis_list_num);
            break;
        case 0x04000302:    //轮显每屏时间
            WORD_BCD2(databuf, mbase_cb->lcd_autodis_keep_sec);
            break;
        case 0x04000303:    //电能小数位数
            *databuf = BYTE_BCD(mbase_cb->energy_point);
            break;
        case 0x04000304:    //功率小数尾数
            *databuf = BYTE_BCD(mbase_cb->power_point);
            break;
        case 0x04000305:    //按显屏数
            *databuf = BYTE_BCD(mbase_cb->lcd_keydis_list_num);
            break;
        case 0x04000308:    //全显时间
            *databuf = BYTE_BCD(mbase_cb->lcd_poweron_all_dis_time);
            break;
        case 0x04000501:
        case 0x04000502:
        case 0x04000503:
        case 0x04000504:
        case 0x04000505:
        case 0x04000506:
        case 0x04000507:
        case 0x040005FF:
            index = id & 0x000000FF;
            if(index != 0xFF)
            {
                mem_swap_cpy((uint8_t*)&sate, (uint8_t*)&mbase_cb->run_state[index], 2);
                for(i=0; i<16; i++)
                {
                    sate1 = sate1 >> 1;
                    if(sate & 0x8000)
                    {
                        sate1 |= 0x8000;
                    }
                    sate = sate << 1;
                    
                }
                memcpy(databuf, (uint8_t*)&sate1, 2);
            }
            else
            {
                for(j=1;j<8;j++)
                {
                    mem_swap_cpy((uint8_t*)&sate, (uint8_t*)&mbase_cb->run_state[j], 2);
                    sate1 = 0;
                    for(i=0; i<16; i++)
                    {
                        sate1 = sate1 >> 1;
                        if(sate & 0x8000)
                        {
                            sate1 |= 0x8000;
                        }
                        sate = sate << 1;
                        
                    }
                    memcpy(databuf+2*(j-1), (uint8_t*)&sate1, 2);
                }
            }
            break;

        case 0x04001104:  //主动上报模式字
            ndv_memcopy(databuf, mbase_cb->report_mode_645, sizeof(mbase_cb->report_mode_645));
            break;
        case 0x04100901:    //按键显示每屏时间
            WORD_BCD2(databuf, mbase_cb->lcd_keydis_keep_sec);
            break;
        case 0x04000705:    //载波波特率 
            Baut = get_current_plc_baud(instance);
            for(i=0;i<8;i++)
            {
                if(Baut == baud698[i])
                {
                    *databuf = baud645[i];
                    return 0;
                }
            }
            *databuf = 0x80;
            break;
        case 0x04000703:    //485波特率
            for(i=0;i<8;i++)
            {
                if(mbase_cb->Baut_rs485 == baud698[i])
                {
                    *databuf = baud645[i];
                    return 0;
                }
            }
            *databuf = 0x80;
            break;
        case 0x04000701:    //红外波特率
            *databuf = 0x04;
            break;
        case 0x04090801:    //过流事件阈值
            sate = mbase_cb->OverCurrent_threshold/1000;
            WORD_BCD2(databuf, sate);
            break;
        case 0x04090802:    //过流事件阈值
            *databuf = BYTE_BCD(mbase_cb->OverCurrent_duration_para);
            break;
        case 0xA8000100:    //工厂模式
            *databuf = mbase_cb->factor_mode;
            if(mbase_cb->factor_mode == true)
            {
                *databuf = 0xAA;
            }
            else 
            {
                *databuf = 0x00;
            }
            break;
        case 0xA8F00003:    //多功能配置字+
            memset(databuf, 0, 2);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Meter_Type);
            nd_get_index(instance, addr, (uint8_t*)databuf, 2); //区分判断 是否已经设置过表型
            //memcpy(databuf, (uint8_t *)&mbase_cb->metertype, 2);
            break;
        case 0xA8F00002:    //背光配置字
            memset(databuf, 0, 4);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Blt_Alarm_Byte);
            nd_get_index(instance, addr, (uint8_t*)databuf, 4); 
            //memcpy(databuf, (uint8_t *)&mbase_cb->blt_alarm, 2);
            break;
        case 0xA8F00051://掉电主动上报的阈值时间(秒)hex  0x0E10
            memcpy(databuf, (uint8_t*)&mbase_cb->active_report_sec_limit, 2 );        
            break; 
        case 0xA8F00052://工频防潜开关
            *databuf = mbase_cb->no_filt;
            break;
        case 0xA601100F:
			StAddr = SoftwareCHK();
			memcpy( databuf, (unsigned char*)&StAddr, 4 );
			break;
    }
    return 0;
}

static uint8_t dl645rd_loction(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t addr1=0;
    uint8_t buf[10] = {0,};
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location1);
    nd_get_index(instance, addr1, buf, 10);
    *databuf++ = buf[9];
    *databuf++ = buf[7];
    *databuf++ = buf[5];
    *databuf++ = buf[3];
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location2);
    nd_get_index(instance, addr1, buf, 10);
    *databuf++ = buf[9];
    *databuf++ = buf[7];
    *databuf++ = buf[5];
    *databuf++ = buf[3];
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location3);
    nd_get_index(instance, addr1, buf, 5);
    *databuf++ = buf[4];
    *databuf++ = buf[3];
    *databuf++ = buf[2];
    
    return 0;
}

static uint8_t dl645rd_blanceday(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t addr1=0;
    uint8_t buf[2] = {0,};
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_get_index(instance, addr1, buf, 2);
    *databuf++ = BYTE_BCD(buf[1]);
    *databuf++ = BYTE_BCD(buf[0]);
    return 0;
}

static uint8_t dl645rd_runbyte(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index = 0;
 
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_get_index(instance, s_index, databuf, 1);
    
    if(mbase_cb->relay_ctl_mode == 0)   //（0：脉冲式，1：保持式）
        *databuf |= 0x01;
    else if(mbase_cb->relay_ctl_mode == 1)
        *databuf &= ~0x01;

    if((mbase_cb->metertype & F_ExternRelay_En) == 0) //内置 固定为0
    {
        *databuf &= ~0x01;
    }

    
    if(mbase_cb->lcd_12_mean == 1)  // 0显示当前套、备用套时段; 1显示当前套、备用套费率
        *databuf |= 0x02;
    else
        *databuf &= ~0x02;
    
    return 0;
}

static uint8_t dl645rd_hex_string(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i=0,word=0;
    uint32_t addr1 = 0;
    
    switch(addr)
    {
        case Measure_Mode:  //组合特征字
            word = mbase_cb->active_energy_cal_word;
            break;
        case Weekend:  //周休日特征字
            addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
            nd_get_index(instance, addr1, (uint8_t*)&word, 1);
            break;
    }
    *databuf = 0;
    for(i=0; i<8; i++)
    {
        if(word & 0x80)
        {
            *databuf |= 0x01<<i;
        }
        word = word << 1;
    }
    return 0;
}

static uint8_t dl645rd_meterchip(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint8_t len=0;
    
    len = rn8209c_get(instance, (uint8_t*)&addr, databuf);
    *length = len;
    return 0;
}

static uint8_t dl645rd_report_stat(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{

    report_state_read_pro(instance, databuf, length);
    
    return 0;
}



static uint8_t dl645wt_date(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t day = 0, settimebuf[4]={0,};
    uint8_t buffer[10]={0},temp[10]={0};
    uint32_t s_index = 0;

    //保存校时前时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);

    day = instance->time.tm_mday;
    prx++;
    settimebuf[0] = *prx++;//日
    settimebuf[1] = *prx++;//月
    settimebuf[2] = *prx++;//年
    settimebuf[3] = GetWeek645(&settimebuf[0]);
    write_mcu_rtc_date(0, settimebuf, 4);
    read_mcu_rtc(0x00, instance);

    //保存校时后时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);
    
    if(day != instance->time.tm_mday)  //跨日，清除广播校时标志位
    {
        mbase_cb->boardadjtime = 0;
    }

    record_event_no_mesg_pro(instance, NULL, SetTimeS, NULL, 1);// 校时记录
    after_Time_set_hook(instance);
    
    return 0;
}

static uint8_t dl645wt_time(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t settimebuf[3]={0,};
    uint8_t buffer[10]={0},temp[10]={0};
    uint32_t s_index = 0;

    //保存校时前时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);
    
    settimebuf[0] = *prx++;//日
    settimebuf[1] = *prx++;//月
    settimebuf[2] = *prx++;//年
    write_mcu_rtc_time(0, settimebuf, 3);
    read_mcu_rtc(0x00, instance);

    //保存校时后时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);


    record_event_no_mesg_pro(instance, NULL, SetTimeS, NULL, 1);// 校时记录
    after_Time_set_hook(instance);

    return 0;
}

static uint8_t dl645wt_datetime(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t day = 0, settimebuf[7]={0,};
    uint8_t buffer[10]={0},temp[10]={0};
    uint32_t s_index = 0;

    //保存校时前时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);
    
    day = instance->time.tm_mday;
    settimebuf[0] = *prx++;//秒
    settimebuf[1] = *prx++;//分
    settimebuf[2] = *prx++;//时
    prx++;//跳过周
    settimebuf[3] = *prx++;//日
    settimebuf[4] = *prx++;//月
    settimebuf[5] = *prx++;//年
    settimebuf[6] = GetWeek645(&settimebuf[3]);
    write_mcu_rtc(0, settimebuf, 7);
    read_mcu_rtc(0x00, instance);
    if(day != instance->time.tm_mday)  //跨日，清除广播校时标志位
    {
        mbase_cb->boardadjtime = 0;
    }

    //保存校时后时间 
    rtc_get_from_tm(instance,temp);
    mem_swap_cpy( &buffer[0], &temp[0], 7);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
    nd_set_index(instance, s_index, buffer, 7);


    record_event_no_mesg_pro(instance, NULL, SetTimeS, NULL, 1);// 校时记录
    after_Time_set_hook(instance);

    
    return 0;
}

static uint8_t dl645wt_chgtime(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    uint32_t addr1=0;
    uint16_t year=0;
    uint8_t buf[7] = {0,};
    
    buf[6] = 0xFF;  
    buf[5] = BCD_BYTE(*prx++);  //分
    buf[4] = BCD_BYTE(*prx++);  //时
    buf[3] = BCD_BYTE(*prx++);  //日
    buf[2] = BCD_BYTE(*prx++);  //月
    year = BCD_BYTE(*prx++) + 2000;  //年
    mem_swap_cpy((uint8_t*)&buf[0], (uint8_t*)&year, 2);//

    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_set_index(instance, addr1, buf, 7);
    
    return 0;
}

static uint8_t dl645wt_meteraddr(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index=0;
    
    memcpy(mbase_cb->tsa, prx, 6);
    
    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
            offsetof(struct mbase_control_block_t, tsa));
    nd_set_offset(instance, s_index, mbase_cb->tsa, 6); 
    
    return 0;
}

static uint8_t dl645wt_hex_bcd(struct instance_t* instance, uint32_t addr, uint8_t *prx, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint32_t addr1=0;
    uint32_t temp=0,data=0;
    uint8_t buf[4] = {0,},len=0, storage_len = 0;
    uint16_t temp_u16 = 0;
    
    memcpy((uint8_t*)&temp, prx, *length);
    
    if((addr == ImaxProtectValue) || (addr == Meter_CT) || (addr == Meter_PT) 
       || (addr == IrAuthenTime) || (addr == Meter_Const1))    //拉闸电流门限需要从4字节转成3字节
    {
        len = 4;
        storage_len = 4;
    }
    else if((addr == (SeasonDay_Para+4)))    //协议2字节 存储1字节
    {
        len = 2;
        storage_len = 1;
    }

    else
    {
        len = *length;
        storage_len = len;
    }

    switch(len)
    {
        case 0x01:
            data = BCD_BYTE(temp);
            break;
        case 0x02:
            data = word_bcd_to_hex(temp);
            break;
        case 0x03:
        case 0x04:
            data = long_bcd_tohex(temp);
            break;
    }
    if(addr == ImaxProtectValue)
    {
        data = data*10;
    }
    mem_swap_cpy((uint8_t*)&temp, (uint8_t*)&data, storage_len);
    
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_set_index(instance, addr1, (uint8_t*)&temp, storage_len);



    //同步更新ram

    switch(addr)
    {
        case ImaxProtectTime://大电流延时时间赋值
            mem_swap_cpy((uint8_t *)&mbase_cb->Prot_time, (uint8_t*)&temp, 2);
            if(mbase_cb->imaxmin > 0)   //过流中修改时间，刷新
                mbase_cb->imaxmin = mbase_cb->Prot_time*60;

            break;
            
        case ImaxProtectValue: 
            mem_swap_cpy((uint8_t*)&mbase_cb->I_Prot, (uint8_t*)&temp, 4);
            break;
        
        case Remote_RelayOffTime: 
            mem_swap_cpy((uint8_t *)&temp_u16, (uint8_t*)&temp, 2);
            if(mbase_cb->alarmmin > 0)   //延时中刷新
            {
                mbase_cb->alarmmin = temp_u16*60;   
            }
            break;
            
        default:
            break;
    }

    
    return 0;
}

static uint8_t dl645wt_ram(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    uint32_t id=0,s_index=0,dt=0;
    uint16_t temp=0;
    
    memcpy((uint8_t *)&id, databuf-12, 4);
    
    switch(id)
    {
        case 0x04000301:    //轮显屏数
            mbase_cb->lcd_autodis_list_num = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, lcd_autodis_list_num));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->lcd_autodis_list_num, 1);
            break;
        case 0x04000302:    //轮显每屏时间
            memcpy((uint8_t*)&temp, databuf, 2);
            mbase_cb->lcd_autodis_keep_sec = word_bcd_to_hex(temp);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, lcd_autodis_keep_sec));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->lcd_autodis_keep_sec, 2);
            break;
        case 0x04000303:    //电能小数位数
            mbase_cb->energy_point = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, energy_point));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->energy_point, 1);
            break;
        case 0x04000304:    //功率小数尾数
            mbase_cb->power_point = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, power_point));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->power_point, 1);
            break;
        case 0x04000305:    //按显屏数
            mbase_cb->lcd_keydis_list_num = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, lcd_keydis_list_num));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->lcd_keydis_list_num, 1);
            break;
        case 0x04000308:    //全显时间
            mbase_cb->lcd_poweron_all_dis_time = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, lcd_poweron_all_dis_time));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->lcd_poweron_all_dis_time, 1);
            break;
        case 0x04000701:    //紅外波特率設置無效
            break;
        case 0x04000705:    //plc波特率
            switch( *databuf )
            {
                case 0x02:  //600
                    mbase_cb->Baut_plc = 1;
                    break;
                case 0x04:  //1200
                    mbase_cb->Baut_plc = 2;
                    break;
                case 0x08:  //2400
                    mbase_cb->Baut_plc = 3;
                    break;
                case 0x10:  //4800
                    mbase_cb->Baut_plc = 4;
                    break;
                case 0x20:  //9600
                    mbase_cb->Baut_plc = 6;
                    break;
                case 0x40:  //19200
                    mbase_cb->Baut_plc = 7;
                    break;
            }
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, Baut_plc));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->Baut_plc, 1);
            mbase_cb->baud_chg = 0x02;  //更新串口波特率  
            break;
        case 0x04000703:    //485波特率
            switch( *databuf )
            {
                case 0x02:  //600
                    mbase_cb->Baut_rs485 = 1;
                    break;
                case 0x04:  //1200
                    mbase_cb->Baut_rs485 = 2;
                    break;
                case 0x08:  //2400
                    mbase_cb->Baut_rs485 = 3;
                    break;
                case 0x10:  //4800
                    mbase_cb->Baut_rs485 = 4;
                    break;
                case 0x20:  //9600
                    mbase_cb->Baut_rs485 = 6;
                    break;
                case 0x40:  //19200
                    mbase_cb->Baut_rs485 = 7;
                    break;
            }
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, Baut_rs485));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->Baut_rs485, 1);
            mbase_cb->baud_chg = 0x01;  //更新串口波特率
            break;
        case 0x04001104:  //主动上报模式字
            ndv_memcopy(mbase_cb->report_mode_645, databuf, sizeof(mbase_cb->report_mode_645));
            //保存 645上报模式字
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
            offsetof(struct mbase_control_block_t, report_mode_645));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->report_mode_645, sizeof(mbase_cb->report_mode_645));

            report_mode_set_from_645(instance, databuf);
            break;
        case 0x04090801:    //过流事件阈值
            memcpy((uint8_t*)&dt, databuf, 2);
            temp = word_bcd_to_hex(dt);
            mbase_cb->OverCurrent_threshold = temp*1000;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, OverCurrent_threshold));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->OverCurrent_threshold, 4);
            break;
        case 0x04090802:    //过流事件阈值
            mbase_cb->OverCurrent_duration_para = BCD_BYTE(*databuf);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, OverCurrent_duration_para));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->OverCurrent_duration_para, 1);
            break;
            
        case 0x04100901:    //按键显示每屏时间
            memcpy((uint8_t*)&temp, databuf, 2);
            mbase_cb->lcd_keydis_keep_sec = word_bcd_to_hex(temp);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, lcd_keydis_keep_sec));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->lcd_keydis_keep_sec, 2);
            break;
        case 0xA8F00002:    //背光报警配置字
            memcpy((uint8_t*)&mbase_cb->blt_alarm, databuf, 4);

            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Blt_Alarm_Byte);
            nd_set_index(instance, s_index, (uint8_t*)&mbase_cb->blt_alarm, 4);
            break;
        case 0xA8F00003:    //多功能配置字
            memcpy((uint8_t*)&mbase_cb->metertype, databuf, 2);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Meter_Type);
            nd_set_index(instance, s_index, (uint8_t*)&mbase_cb->metertype, 2);
            break;
        case 0xA8F00051://掉电主动上报的阈值时间(秒)hex  0x0E10
            memcpy(&mbase_cb->active_report_sec_limit, databuf, 2);                
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, active_report_sec_limit));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->active_report_sec_limit, 2);//存储至EE  
            break;  
        case 0xA8F00052://工频防潜开关
            mbase_cb->no_filt = *databuf;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, no_filt));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->no_filt, 1);//存储至EE  
            break;
    }
    return 0;
}

static uint8_t dl645wt_loction(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t s_index = 0;
  
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location1);
    nd_set_index(instance, s_index, databuf, 10); 
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location2);
    nd_set_index(instance, s_index, databuf, 10);   
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location3);
    nd_set_index(instance, s_index, databuf, 5);

    return 0;
}

static uint8_t dl645wt_blanceday(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint32_t addr1=0;
    uint8_t buf[2] = {0};
    uint8_t buftmp[30] = {0};
    uint32_t s_index = 0;

    //-保存编程前
    nd_get(instance, 0x41160200, buftmp, 20, NULL);
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_Balance);
    nd_set_index(instance, s_index, buftmp, 20);
    buftmp[0] = buftmp[5]; // 第一结算日 
    buftmp[1] = buftmp[7];

    buf[1] = BCD_BYTE(*databuf++);
    buf[0] = BCD_BYTE(*databuf++);
    addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_set_index(instance, addr1, buf, 2);


    if( addr == Balance_Time )
    {
        if ( (buf[0] != buftmp[0]) || (buf[1] != buftmp[1]) ) //第一结算日改变
        {
            mbase_cb->first_MbillDay_flag = 0x5A;
            record_event_no_mesg_pro(instance, NULL, BillFreeze, NULL, 1);//
        }
    }

    record_event_no_mesg_pro(instance, NULL, BillDayWrS, NULL, 1);//

    return 0;
}

static uint8_t dl645wt_runbyte(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index =0;
    
    if(*databuf & 0x01)
        mbase_cb->relay_ctl_mode = 0;   //（0：脉冲式，1：保持式）
    else
        mbase_cb->relay_ctl_mode = 1;

    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, relay_ctl_mode));
    nd_set_offset(instance, s_index, &mbase_cb->relay_ctl_mode, sizeof(mbase_cb->relay_ctl_mode));  

    
    if(*databuf & 0x02) 
        mbase_cb->lcd_12_mean = 1;  // 0显示当前套、备用套时段; 1显示当前套、备用套费率
    else
        mbase_cb->lcd_12_mean = 0;

    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
    offsetof(struct mbase_control_block_t, lcd_12_mean));
    nd_set_offset(instance, s_index, &mbase_cb->lcd_12_mean, sizeof(mbase_cb->lcd_12_mean));  

   
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
    nd_set_index(instance, s_index, databuf, 1);
    
    return 0;
}

static uint8_t dl645wt_hex_string(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i=0,word=0;
    uint32_t s_index = 0;
    uint8_t tmpbuf[30] = {0};
    
    for(i=0; i<8; i++)
    {
        if(*databuf & 0x80)
        {
            word |= 0x01<<i;
        }
        *databuf = *databuf << 1;
    }
    
    switch(addr)
    {
        case Measure_Mode:  //组合特征字
            //保存编程前
            nd_get(instance, 0x41120200, tmpbuf, 3, NULL);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_ActCom);
            nd_set_index(instance, addr, tmpbuf, 3);

            mbase_cb->active_energy_cal_word = word;
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, active_energy_cal_word));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->active_energy_cal_word, 1);

            record_event_no_mesg_pro(instance, NULL, ActCombWrS, NULL, 1);// 
            break;
        case Weekend:  //周休日特征字
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
            nd_set_index(instance, s_index, (uint8_t*)&word, 1);
            break;
    }
    return 0;
}

static uint8_t dl645wt_daytable(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    uint32_t s_index;
    uint8_t i,temp[42]={0,};
    
    if((addr == Season_Table0) || (addr == Season_Table1))
    {
        for(i=0; i<14; i++)
        {
            temp[2] = BCD_BYTE(*databuf++);
            temp[1] = BCD_BYTE(*databuf++);
            temp[0] = BCD_BYTE(*databuf++);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
            nd_set_index(instance, s_index, temp, 3);
        }
    }
    else
    {
        for(i=0; i<14; i++)
        {
            temp[i*3+2] = BCD_BYTE(*databuf++);
            temp[i*3+1] = BCD_BYTE(*databuf++);
            temp[i*3+0] = BCD_BYTE(*databuf++);
        }
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr);
        nd_set_index(instance, s_index, temp, 3*MaxTouNum);
    }
    return 0;
}
 
static uint8_t dl645wt_meterchip(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    int len;
    len = rn8209c_set(instance, databuf);
     *length = (uint8_t)len;
    return 0;
}


static uint8_t dl645wt_reset_report_state(struct instance_t* instance, uint32_t addr, uint8_t *databuf, uint8_t *length)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t cnt, index;
        
    if (0 == mbase_cb->report_stat_already_read645)
    {
        return RS_ERR_OTHERS;
    }

    for (cnt = 0; cnt < 12; cnt++)
    {      
        for (index = 0; index < 8; index++) /*若复位状态字中包含已发生未读事件，返回异常应答，状态位保留*/
        {   
            if ((0 == (databuf[cnt] & (0x01 << index))) && (0 == (mbase_cb->report_stat_read_cache645[cnt] & (0x01 << index)))
                && (mbase_cb->report_stat_645[cnt] & (0x01 << index)))
            {
                return RS_ERR_OTHERS;
            }  

            if ((0 == (databuf[cnt] & (0x01 << index))) && (mbase_cb->report_stat_read_cache645[cnt] & (0x01 << index)))
            {
                mbase_cb->report_stat_645[12 + 8*cnt + index] = 0; 
                report_645stat_reset_from_645(instance, 8*cnt + index);
            }
        }

        mbase_cb->report_stat_645[cnt] &= databuf[cnt];
        mbase_cb->report_stat_read_cache645[cnt] &= databuf[cnt];
    }

    report_645stat_save(instance);        

    mbase_cb->report_stat_already_read645 = 0; 

    event_out_check(instance);

    return 0;	
}


static const CODE data_check_fun data_645read_table[] =
{
    0x00,                   //FORMAT_00
    dl645rd_date,           //FORMAT_01
    dl645rd_time,           //FORMAT_02
    dl645rd_datetime,       //FORMAT_03
    dl645rd_chgtime,        //FORMAT_04
    dl645rd_meteraddr,      //FORMAT_05
    dl645rd_hex_bcd,        //FORMAT_06
    dl645rd_meterno,        //FORMAT_07
    dl645rd_ram,            //FORMAT_08
    dl645rd_loction,        //FORMAT_09
    dl645rd_blanceday,      //FORMAT_0A
    dl645rd_runbyte,        //FORMAT_0B
    dl645rd_hex_string,     //FORMAT_0C
    0x00,                   //FORMAT_0D 
    dl645rd_meterchip,      //FORMAT_0E
    0x00,                   //FORMAT_0F
    dl645rd_report_stat,    //FORMAT_10

};

static const CODE data_check_fun data_645write_table[] =
{
    0x00,                   //FORMAT_00
    dl645wt_date,           //FORMAT_01
    dl645wt_time,           //FORMAT_02
    dl645wt_datetime,       //FORMAT_03
    dl645wt_chgtime,        //FORMAT_04
    dl645wt_meteraddr,      //FORMAT_05
    dl645wt_hex_bcd,        //FORMAT_06
    0x00,                   //FORMAT_07
    dl645wt_ram,            //FORMAT_08
    dl645wt_loction,        //FORMAT_09
    dl645wt_blanceday,      //FORMAT_0A
    dl645wt_runbyte,        //FORMAT_0B
    dl645wt_hex_string,     //FORMAT_0C
    dl645wt_daytable,       //FORMAT_0D 
    dl645wt_meterchip,      //FORMAT_0E
    dl645wt_reset_report_state,      //FORMAT_0F
    0x00,                   //FORMAT_10

};


static const uint8_t CODE data_check_fun_num = sizeof(data_645read_table) / sizeof(data_645read_table[0]);

uint8_t CODE(*find_check_rd_fun(uint8_t index))(struct instance_t* instance, uint32_t addr, uint8_t *, uint8_t *)
{
    return data_645read_table[index];
}

uint8_t CODE(*find_check_wt_fun(uint8_t index))(struct instance_t* instance, uint32_t addr, uint8_t *, uint8_t *)
{
    return data_645write_table[index];
}

uint8_t dl645rd_chg_data(struct instance_t* instance, uint32_t addr, uint8_t format, uint8_t *pbuf, uint8_t *len)
{
    uint8_t      result;
    uint8_t CODE (*format_fun)(struct instance_t* instance, uint32_t addr, uint8_t *, uint8_t *);

    format_fun = find_check_rd_fun(format);
    if (0 != format_fun)
    {
        result = format_fun(instance, addr, pbuf, len);
        return result;
    }

    return 1;
}

uint8_t dl645wt_chg_data(struct instance_t* instance, uint32_t addr, uint8_t format, uint8_t *pbuf, uint8_t *len)
{
    uint8_t      result;
    uint8_t CODE (*format_fun)(struct instance_t* instance, uint32_t addr, uint8_t *, uint8_t *);

    format_fun = find_check_wt_fun(format);
    if (0 != format_fun)
    {
        result = format_fun(instance, addr, pbuf, len);
        return result;
    }

    return 1;
}

