#include "nidavellir.h"
#include "645id_table.h"
#include "ndvh_rn8209.h"

extern unsigned char GetCommTable( unsigned long ID );
struct factor_adj_meter_t factory3;
extern void emu_calibration_clear(struct instance_t* instance);
extern void meterchip_adj_self(struct instance_t* instance , struct factor_adj_meter_t *adj_data);
extern const struct dev_info_t rn8209_dev;

int rn8209c_set( struct instance_t* instance, uint8_t *p_rx )
{
    uint8_t addr=0,lenth=0,temp_buffer[8]={0};
    int16_t tmp,temp[30]={0,};
    uint32_t s_index=0,data=0;
    uint8_t* buf = p_rx;
    
    addr = *(buf+16);//地址
    memcpy((uint8_t*)&data, (uint8_t*)(buf+12), 2);//数据
    
    if((addr==0x07)||(addr==0x08))//相位寄存器
        lenth = 1;
    else
        lenth = 2;
    
    if(addr<=0x17)//00~17H,需要效验
    {
        CmdRn8209(CMD_WR_EN , &rn8209_dev);
        if(addr == 0x17)
        {
            rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , EMUCON2 , 2 , &tmp);
            if(data == 0x01)
            {
                data = tmp | 0x0100;
            }
            else
            {
                data = tmp & 0xFEFF;
            }
        }
        rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, addr, lenth, (uint8_t*)&data);
        memcpy((uint8_t*)&tmp, (uint8_t*)&data, lenth);
        CmdRn8209(CMD_WR_DIS , &rn8209_dev);
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain0);    //火线增益、相位
        nd_get_index(instance, s_index, temp_buffer, 8); 
        switch(addr)//存入EEPROM
        {
            case 0x05://增益
                temp_buffer[0] = (uint8_t)data;
                temp_buffer[1] = (uint8_t)(data>>8);
                break;
            case 0x07://相位
                temp_buffer[2] = (uint8_t)data;
                break;
            case 0x0A://功率偏置
                temp_buffer[4] = (uint8_t)data;
                temp_buffer[5] = (uint8_t)(data>>8);
                break;
            case 0x17://相位微调
                temp_buffer[3] = (uint8_t)(data>>8);
                break;
        }
        nd_set_index(instance, s_index, temp_buffer, 8); 
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Other);    
        nd_get_index(instance, s_index, temp_buffer, 4);
        switch(addr)
        {
            case 0x03:  //启动阀值
                temp_buffer[0] = (uint8_t)data;
                temp_buffer[1] = (uint8_t)(data>>8);
                break;
            case 0x02:  //hfconst
                temp_buffer[2] = (uint8_t)data;
                temp_buffer[3] = (uint8_t)(data>>8);
                break;
        }
        nd_set_index(instance, s_index, temp_buffer, 4);
    }
    else if(addr >= 0xA0)//0.5L 自热影响量
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Heat_Value_05L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0xA0], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x90)//自热影响
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Heat_Value_10L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0x90], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x80)//电压影响量
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Un_Cmp_Value);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 8);
        switch(addr)
        {
            case 0x80:
                memcpy((uint8_t*)&temp[0], (uint8_t*)&data, 2);
                break;
            case 0x81:
                memcpy((uint8_t*)&temp[1], (uint8_t*)&data, 2);
                break;
            case 0x82:
                memcpy((uint8_t*)&temp[2], (uint8_t*)&data, 2);
                break;
            case 0x83:
                memcpy((uint8_t*)&temp[3], (uint8_t*)&data, 2);
                break;
            default:
                return false;
                break;
        }
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 8);
    }
    return true;
}

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
        case 0xA800020C://微调与补偿,计量相关万能写
                rn8209c_set(instance, prx);
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
