#include "nidavellir.h"
#include "645id_table.h"
#include "ndvh_rn8209.h"

/*
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
*/
extern unsigned char GetCommTable( unsigned long ID );
struct factor_adj_meter_t factory3;
extern void emu_calibration_clear(struct instance_t* instance);
extern void meterchip_adj_self(struct instance_t* instance , struct factor_adj_meter_t *adj_data);
/***********************************************************************/
/*  函数名称：CommWrite_SpecProc                                       */
/*  函数功能：关于通讯写的特殊处理                                     */
/*  输入参数：ID：通讯ID                                               */
/*  输出参数：0为写EEPROM完成,其余为未写                              */
/***********************************************************************/
unsigned char CommWrite_SpecProc( struct instance_t* instance,  unsigned char* prx, unsigned long ID )
{
    unsigned char buf[10] = {0,};
    unsigned long s_index = 0;
    signed char ret = 0;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
	switch( ID )
	{
		case 0xA8000202:    //计量参数初始化
                meteric_para_clr(instance, 0);
                emu_calibration_clear(instance);
                break;
        case 0x04F0FFFE:
                break;
		case 0x02FFFF00:
                break;
		case 0xA8000101://生产测试模式
				break;

		case 0xA8000201://电表计量参数(Un、Ib、Imax、CP)
				break;
		case 0xA8000105://默认参数初始化命令
                ret = para_init( instance );
                meter_clr(instance);
                if(ret < 0) return -1;
				break;
		case 0xA8000203://校表参数
		case 0xA8000204://校表参数
				break;
		case 0xA800020A://K系数
				break;
		case 0xA6000305://启动阈值和脉冲速率
                memcpy(buf, prx+12, 4);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Other);//写入脉冲速率
                nd_set_index(instance, s_index, buf, 4);
				break;
        case 0xA8000208://单点校表
                //0000C350 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 0000D742 0000C350 000055F0
                memcpy((unsigned char*)&factory3.Ua, prx+12, 4); //电压220.00V，2位小数
                memcpy((unsigned char*)&factory3.Ia, prx+16, 4); //电流5.0000A，4位小数
                memcpy((unsigned char*)&factory3.Pa, prx+20, 4); //有功功率550.00W，2位小数
                memcpy((unsigned char*)&factory3.Ian, prx+60, 4);//零线电流5.0000A，4位小数
                if(factory3.Pa != 0)    //火线修正
                  factory3.adjflag |= F_ALadj;
                else
                  factory3.adjflag &= ~F_ALadj;
                
                if(factory3.Ian != 0)   //零线修正
                    factory3.adjflag |= F_ANadj;
                else
                    factory3.adjflag &= ~F_ANadj;
                meterchip_adj_self(instance, &factory3);
                break;
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
signed char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len)
{
    signed char ret=0;
	unsigned char Err_Byte=0,Ptr;
	unsigned char rx_len=0;
	unsigned long Addr,Info,addr1;
	unsigned long ID;
    uint32_t s_index=0;
	struct mbase_control_block_t* mbase_cb;
	
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        Err_Byte = RS_ERR_OTHERS;
        return Err_Byte;
    }

    rx_len = *(p_rx-1);
	memcpy((unsigned char*)&ID, p_rx, 4);

	//根据ID,获取相应的数据地址和信息
	Ptr = GetCommTable( ID );//查表
	if( Ptr == 0xFF )
    {
		Err_Byte = RS_ERR_OTHERS;
        return Err_Byte;
    }
	Addr = CommTab[Ptr].Addr;
	Info = CommTab[Ptr].Info;

	if( (Info & CommData_W) == 0 )//不可写
    {
        Err_Byte = RS_ERR_WR;
        return Err_Byte;
    }

	*len = (unsigned char)(Info & CommDataLen);
    
    if(ID == 0xA8000100)
    {
        if(*(p_rx+12) == 0xAA)
            mbase_cb->factor_mode = true;
        else if(*(p_rx+12) == 0x00)
            mbase_cb->factor_mode = false;
        else
            Err_Byte = RS_ERR_OTHERS;
        if(Err_Byte == 0)
        {
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, factor_mode));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->factor_mode, 1);//存储至EE
            return 0;
        }
        else
        {
            return Err_Byte;
        }
    }

	if(mbase_cb->factor_mode == true) 
	{
		//数据长度判断
		Err_Byte = Cmp_CommDataLen( ID, rx_len-12, *len );
		if( Err_Byte != 0 )
        {
            return Err_Byte;
        }
		*len = rx_len;

		Err_Byte = CommWrite_DataCheck( ID, len );//特殊判断
		if( Err_Byte != 0 )
        {
			return Err_Byte;
        }

        ret = CommWrite_SpecProc( instance, p_rx, ID );
		if( ret == 1 )//特殊写操作
		{
			addr1 = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Addr);
			nd_set_index(instance, addr1, p_rx, *len); 
            return 0;
		}
        else if(ret == 0)
        {
            return 0;
        }
        else 
        {
            Err_Byte = RS_ERR_OTHERS;
            return Err_Byte;
        }
	}
	else
    {
        Err_Byte = RS_ERR_PASS;
        return Err_Byte;
    }
}
