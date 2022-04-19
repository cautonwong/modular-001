
#include "645id_table.h"

const COMMTAB  CommTab[] =
{
	/********************************** 生产命令 ************************************/
    0xA8000200, 0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 8,
	0xA8000201,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 10,//电表计量参数
	0xA8000202, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY,                        //初始化校表参数
    0xA8000203,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 20,//A相校表参数
	0xA8000204,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 20,//B相校表参数
    0xA800020A,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 20,//K系数
	0xA8000100,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + 1,//工厂模式
	0xA8000101,	0xFFFF,											CommData_WR + CommData_HEX + CommData_RAM + CommData_FACTORY + 1,//生产测试模式
	0xA8000102,	0xFFFF,											CommData_W + CommData_HEX + CommData_RAM + CommData_FACTORY + 1,//测试命令
	0xA8000103,	0xFFFF,											CommData_R + CommData_RAM + 6,//自检状态字
	0xA8000105, 0xFFFF,											CommData_W + CommData_FACTORY + 0,//默认参数初始化 
	0xA8000106,	0xFFFF,											CommData_W + CommData_FACTORY + 0,//初始化电表
	0xA8000107,	0xFFFF,											CommData_R + CommData_RAM + 20,//厂家内部软件版本号
	0x04F0FFFE, 0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 17,//计量终端校表参数
	0x02FFFF00, 0xFFFF,											CommData_W + CommData_HEX + CommData_FACTORY + 52,                   //台体瞬时数据
    0xEE00030B, 0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_FACTORY + 44,//时钟微调参数
	0xEE001006,	0xFFFF,											CommData_WR + CommData_E2PROM + CommData_HEX + CommData_SECONDLY + 1,//安徽用户手动合闸配置
	0xEE000308,	0xFFFF,											CommData_R + CommData_RAM + 4,//V9381错误状态字
	0xEE001005,	0xFFFF,											CommData_WR + CommData_E2PROM + CommData_HEX + CommData_SECONDLY + 1,//表功能配置
	0xA8881077,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_PROGRAM + 10,//模板号
	0xEEEE1054,	0xFFFF,											CommData_WR + CommData_HEX + CommData_E2PROM + CommData_PROGRAM + 10,//生产记录
	
    /********************************** 测试命令 ************************************/
	0xEE001001,	0xFFFF,											CommData_R + CommData_RAM + 11,//通讯抄读状态字数据集合
	0xEE001002,	0xFFFF,											CommData_R + CommData_RAM + 1,//继电器操作原因
};