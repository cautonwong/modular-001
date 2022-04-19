#include "nidavellir.h"
#include "645id_table.h"

//错误信息字定义
#define 	RS_ERR_LEN		        	0x01//数据域长度错误,其它错误
#define 	RS_ERR_OTHERS		       	0x01//其它错误
#define 	RS_ERR_ID		        	0x02//ID错误
#define 	RS_ERR_PASS		        	0x04//密码错/未授权
#define 	RS_ERR_WR		        	0x04//读写权限
#define 	RS_ERR_BAUT		        	0x08//波特率不能更改
#define     RS_ERR_SEAOV                0x10//年时区数超
#define     RS_ERR_TOUOV                0x20//日时段数超
#define     RS_ERR_FEEOV                0x40//费率数超

extern unsigned char GetCommTable( unsigned long ID );

/***********************************************************************/
/*  函数名称：CommWrite_SpecProc                                       */
/*  函数功能：关于通讯写的特殊处理                                     */
/*  输入参数：ID：通讯ID                                               */
/*  输出参数：0为写EEPROM完成,其余为未写                              */
/***********************************************************************/
unsigned char CommWrite_SpecProc( unsigned long ID )
{
       
	switch( ID )
	{
		case 0xA8000100://工厂模式
                break;
		case 0xA8000202:
                break;
        case 0x04F0FFFE:
                break;
		case 0x02FFFF00:
                break;
		case 0xA8000101://生产测试模式
				break;
		case 0xA8000106://初始化
				break;
		case 0xA8000201://电表计量参数(Un、Ib、Imax、CP)
				break;
		case 0xA8000105://默认参数初始化命令
				break;
		case 0xA8000203://校表参数
		case 0xA8000204://校表参数
				break;
		case 0xA800020A://K系数
				break;
		case 0xA8000200://V9381能量桶和启动阀值
				break;
		case 0xEE000309://校表参数
				break;		
		case 0xEE001005:
				return 1;
	}
	return 0;
}

/***********************************************************************/
/*  函数名称：CommWrite_DataCheck                                      */
/*  函数功能：通讯中数据合法性判断                                     */
/*  输入参数：ID：通讯ID                                               */
/*            Len：数据长度                                            */
/*  输出参数：0为合法,其余为错误                                      */
/***********************************************************************/
unsigned char CommWrite_DataCheck( unsigned long ID, unsigned char* Len )
{
	unsigned char Err_Byte=0;

	return 	Err_Byte;
}
/***********************************************************************/
/*  函数名称：Cmp_CommDataLen                                          */
/*  函数功能：通讯中非定长数据长度合法性判断                           */
/*  输入参数：ID：通讯ID                                               */
/*            DataLen：实际数据长度                                    */
/*            TableLenn：表格数据长度                                  */
/*  输出参数：0为正确,其余为错误                                      */
/***********************************************************************/
unsigned char Cmp_CommDataLen( unsigned long ID, unsigned char DataLen, unsigned char TableLen )
{
	unsigned char Err_Byte=0;

	if( DataLen != TableLen )
		Err_Byte = RS_ERR_LEN;

	return Err_Byte;
}

/***********************************************************************/
/*  函数名称：Comm_Write                                               */
/*  函数功能：红外通讯写                                               */
/*  输出参数：读结果：0为正确,其余为错误                              */
/***********************************************************************/
unsigned char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t len)
{
	unsigned char Err_Byte=0,Ptr;
	unsigned char Len;
	unsigned long Addr,Info,addr1;
	unsigned long ID;
	struct mbase_control_block_t* mbase_cb;
	
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return 1;
    }

	memcpy((unsigned char*)&ID, p_rx, 4);

	//根据ID,获取相应的数据地址和信息
	Ptr = GetCommTable( ID );//查表
	if( Ptr == 0xFF )
		return 1;
	Addr = CommTab[Ptr].Addr;
	Info = CommTab[Ptr].Info;

	if( (Info & CommData_W) == 0 )//不可写
		return 1;

	Len = (unsigned char)(Info & CommDataLen);

	if(mbase_cb->factor_mode == true) 
	{
		//数据长度判断
		Err_Byte = Cmp_CommDataLen( ID, *p_rx-12, Len );
		if( Err_Byte != 0 )
			return Err_Byte;
		Len = *p_rx-12;

		Err_Byte = CommWrite_DataCheck( ID, (unsigned char*)&Len );//特殊判断
		if( Err_Byte != 0 )
			return 1;

		if( CommWrite_SpecProc( ID ) == 1 )//特殊写操作
		{
			addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Addr);
			nd_set_index(instance, addr1, p_rx, Len); 
		}
        return 0;
	}
	else
		return 1;
}