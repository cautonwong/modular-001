
#include "645id_table.h"
#include "port.h"

extern const  unsigned char FirmWareVersion[20];

const COMMTAB  CommTab[] =
{
    0x04000401, Comm_Number,                                    CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 6,	//表地址
	0xA6000300,	ADJ_Pgain0,                                     CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,	//火线计量参数
	0xA6000301,	ADJ_Pgain1,										CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,	//零线计量参数
	0xA6000303,	ADJ_K,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 24,	//k系数
	0xA6000305,	ADJ_Other,										CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 4,	//脉冲速率和启动阈值
	0xA6000306,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 4,	//表内计量常数
	0xA6000307,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 18,	//交采规格配置
	0xA6000309,	Un_Cmp_Value,                                   CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,	//电压影响量补偿
	0xA6011054, 0xFFFF,											CommData_R + CommData_HEX + CommData_E2PROM + 10,//复位次数
	
	0xA8000100,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 1,	//工厂模式
	0xA8000105, 0xFFFF,											CommData_W + CommData_FACTORY + 0,					//默认参数初始化
	
	0xA8001001,	(unsigned int)FirmWareVersion,                  CommData_R + CommData_RAM + 20,//厂家内部软件版本号    
	0xA8F00001,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 4,	//继电器报警控制字
	0xA8F00002,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 4,	//背光报警控制字
	0xA8F00003,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 2,	//电能表类型、功能配置字
	0xA8F00050,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 4,	//上电持续时间
	0xA8F00051,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 2,	//掉电主动上报的阈值时间
	0xA8F00052,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 1,	//小电流不滤除 AA开启
	0xA8F000FF,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 0xFF,//万能读写

    0xA8000200, 0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,
	0xA8000202, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY+ 0,	//初始化校表参数
    0xA8000203,	ADJ_Pgain0,                                     CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,//A相校表参数
	0xA8000204,	ADJ_Pgain1,                                     CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,//B相校表参数
    0xA8000208, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY + 52,	//台体瞬时数据 交采包
    0xA800020A,	ADJ_K,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 20,//K系数
    0xA800020B, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY + 8,	//小信号交采包
	0xA800020C, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY + 5,	//计量芯片万能读写
	0xA8000101,	0xFFFF,											CommData_WR + CommData_HEX + CommData_RAM + CommData_FACTORY + 1,//生产测试模式
	0xA8000102,	0xFFFF,											CommData_W + CommData_HEX + CommData_RAM + CommData_FACTORY + 1,//测试命令
	0xA8000103,	0xFFFF,											CommData_R + CommData_RAM + 6,//自检状态字
	0xA8000106,	0xFFFF,											CommData_W + CommData_FACTORY + 0,//初始化电表
	0xA8000107,	0xFFFF,											CommData_R + CommData_RAM + 20,//厂家内部软件版本号
};