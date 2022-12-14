#include "nidavellir.h"
#include "645id_table.h"
#include "ndvh_rn8209.h"

extern unsigned char GetCommTable( unsigned long ID );
struct factor_adj_meter_t factory3;
extern void emu_calibration_clear(struct instance_t* instance);
extern void meterchip_adj_self(struct instance_t* instance , struct factor_adj_meter_t *adj_data);
extern const struct dev_info_t rn8209_dev;
extern int16_t Check_para_Pro(struct instance_t* instance);
extern void Energy_Set(struct instance_t* instance, uint32_t basedata, uint8_t dir);
extern int rn8209c_set( struct instance_t* instance, uint8_t *p_rx );
extern unsigned char Cmp_Password(struct instance_t* instance, unsigned char* Addr );
extern unsigned char Date_645Format_Check( unsigned char* Addr );
extern unsigned char Time_645Format_Check( unsigned char* Addr );
extern uint8_t dl645wt_chg_data(struct instance_t* instance, uint32_t addr, uint8_t format, uint8_t *pbuf, uint8_t *len);
extern unsigned char Comm485Baut_Check( unsigned char Baut );
extern unsigned char ChangeTime_Format_Check( unsigned char* Addr );
extern unsigned char SeasonTab_Format_Check( struct instance_t* instance, unsigned char* Addr, unsigned char* Len );
extern unsigned char GetMonMaxDay( unsigned char* Addr );
extern unsigned char TouTab_Format_Check( struct instance_t* instance, unsigned char* Addr, unsigned char* Len );
struct ADJ_POFFSET_T adj_poffset;




/**第一类参数（明文＋MAC）**/
const  uint32_t DL645_Prepay_ParaSetTab[] = 
{
	0x04000108,/**两套分时费率切换时间**/
	0x04000109,/**阶梯表切换时间**/
	0x0400010a,/**阶梯时区切换时间**/
	0x04001001,/**报警金额1**/
	0x04001002,/**报警金额2**/
	0x04001003,
	0x04001004,
	0x04001005,
	0x04001006,
	/** 0x040501ff,当前套费率电价**/
	0x0405ffff,/**备用套费率电价**/
	/** 0x040606ff, 当前套阶梯**/
	0x0406ffff,/** 备用套阶梯**/
	0x040700ff,/** 备用套阶梯时区**/
	0x04000306,/** 电流互感起变比**/
	0x04000307,/** 电压互感起变比**/
	/** 0x0400040E,客户编号**/
	0x04000207,
};

const  uint32_t DL645_Prepay_ParaRdTab[] = 
{
	0x000d00ff,  /** 年结算日结算时间 **/
	0x00900200,  /** 剩余金额                       **/
	0x00900201,  /** 透支金额                       **/
    0x0280000B,
	0x02800020,	
	0x02800021,
	0x03300fff,
	0x033010ff,
	0x0333ffff,
	0x033400ff,
	
	0x0505ffff,
	0x0507ffff,
	0x050802ff,
	0x050affff,	
};

uint8_t Search_DL645_Prepay_IDTab(struct instance_t* instance, uint32_t id,uint8_t cmd)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i = 0,num;
    union dlt645id
    {
        uint32_t idl; 
        uint8_t  ide[4];
    };

    union dlt645id   dlt645id1,dlt645id2;

    if(((cmd != 0x11)&&(cmd != 0x14)) || (mbase_cb->factor_mode == true))
    {
        return true;
    }



    if(mbase_cb->metertype & F_Crad_En)
    {
        return true;
    }	

    dlt645id1.idl = id;	
    num = eleof(DL645_Prepay_ParaSetTab);
    for(i =0 ; i <num ; i++)
    {    
        dlt645id2.idl = DL645_Prepay_ParaSetTab[i];		
        if((dlt645id1.ide[3] == dlt645id2.ide[3])&&(dlt645id1.ide[2] == dlt645id2.ide[2])
	     &&((dlt645id1.ide[1] == dlt645id2.ide[1])||(dlt645id2.ide[1] == 0xff))
	     &&((dlt645id1.ide[0] == dlt645id2.ide[0])||(dlt645id2.ide[0] == 0xff)))
        {
            return false;
        }
    }

    if(cmd == 0x11)
    {
        num = eleof(DL645_Prepay_ParaRdTab);
        for (i =0 ; i <num ; i++)
        {    
          dlt645id2.idl = DL645_Prepay_ParaRdTab[i];

          if((dlt645id1.ide[3] == dlt645id2.ide[3])&&(dlt645id1.ide[2] == dlt645id2.ide[2])
  	     &&((dlt645id1.ide[1] == dlt645id2.ide[1])||(dlt645id2.ide[1] == 0xff))
  	     &&((dlt645id1.ide[0] == dlt645id2.ide[0])||(dlt645id2.ide[0] == 0xff)))
          {
              return false;
          }
       }    
    }   
    
    return true;
}


typedef struct
{
    unsigned short  Id; //显示项ID
    unsigned long   Oad; //oad
    unsigned char   Sequ;//序号
} DISPLAYTAB645;

//显示表格定义
const  DISPLAYTAB645 DisplayTab_645[] =
{
    //当前时间
    0xA001,     0x04000101,			0x00,   //日期
    0xA002,     0x04000102,			0x00,   //时间
    0xA003,     0x02010100,			0x00,   //电压
    0xA006,     0x02020100,			0x00,   //火线电流
    0xA009,     0x02800001,			0x00,   //零线电流
    0xA00A,     0x02030000,			0x00,   //A相功率
    0xA00B,     0x02030100,			0x00,   //A相功率
    0xA012,     0x02060000,			0x00,   //A相功率因数
    0xA013,     0x02060100,			0x00,   //A相功率因数
    
    0xB001,		0x04000401,			0x00,   //表地址高4位
    0xB002,		0x04000401,			0x01,   //表地址低8位
    0xB003,		0x04000402,			0x00,   //表号高4位
    0xB004,		0x04000402,			0x01,   //表号低8位
    0xB005,		0x0400040E,			0x00,   //客户编号高4位
    0xB006,		0x0400040E,			0x01,   //客户编号低8位
    0xB007,		0x04000409,			0x00,   //电表有功常数
    
    #if 0    
    //正向电能量
    //0xC0FF,     0xC000~0xC00C:总，费率1~12;   0xC000~0xC180:当前~上24月正向总电能
    //反向电能量
    //0xD0FF,     0xD000~0xD00C:总，费率1~12;   0xD000~0xD180:当前~上24月反向总电能  
    //组合电能量
    //0xE0FF,     0xE000~0xE00C:总，费率1~12;   0xE000~0xE180:当前~上24月组合总电能  
    #endif
};

#define 	Display645TabNum		sizeof(DisplayTab_645)/sizeof(DISPLAYTAB645)

uint8_t get_645displayid_info( struct instance_t* instance,  uint8_t* prx, uint32_t ID, uint8_t* Addr, uint8_t* Format, uint8_t* Info)
{
    uint8_t IDL1,IDL0,sque,i,fl;
    uint16_t displayid = 0xA002,IDH=0,last_n;
    uint32_t id_645=0,s_index;
    
    memcpy((uint8_t*)&id_645, prx+12, 4);
    sque = *(prx+16);
    
    for(i=0; i<Display645TabNum; i++)
    {
        if((id_645 == DisplayTab_645[i].Oad) && (sque == DisplayTab_645[i].Sequ))
        {
            displayid = DisplayTab_645[i].Id;
            break;
        }
    }
    if(i >= Display645TabNum)
    {
        IDH = (id_645 & 0xFFFF0000) >> 16;
        fl = (uint8_t)((id_645 & 0x0000FF00) >> 8);
        last_n = (uint16_t)((id_645 & 0x000000FF)<<4);
        if(last_n > 12) return RS_ERR_OTHERS;
        if(fl > 12) return RS_ERR_OTHERS;
        switch(IDH)
        {
            case 0x0000:
              displayid = 0xE000 | fl | last_n;
              break;
            case 0x0001:
              displayid = 0xC000 | fl | last_n;
              break;
            case 0x0002:
              displayid = 0xD000 | fl | last_n;
              break;
            default:
              return RS_ERR_OTHERS;
        }
    }
    
    IDL1 = (uint8_t)((ID & 0x0000FF00) >> 8);
    IDL0 = (uint8_t)(ID & 0x000000FF);
    if(IDL0 == 0)   return RS_ERR_OTHERS;
    if(IDL1 == 0x01)    //循环显示
        s_index = AutoDisplayList + IDL0 - 1;
    else if(IDL1 == 0x02) //按键显示
        s_index = KeyDisplayList + IDL0 - 1;
    memcpy(Addr, (uint8_t*)&s_index, 4);
    *Format = 0;
    s_index = CommData_WR + CommData_HEX + CommData_E2PROM + CommData_SECONDLY + 2;
    memcpy(Info, (uint8_t*)&s_index, 2);
    mem_swap_cpy(prx+12, (uint8_t*)&displayid,2);
    return 0;
}

uint8_t get_645daytable_info( struct instance_t* instance,  uint8_t* prx, uint32_t ID, uint8_t* Addr, uint8_t* Format, uint8_t* Info)
{
    uint8_t IDL0=0;
    uint16_t IDH = 0;
    uint32_t addr1 = 0;
    
    IDH = (uint16_t)((ID & 0xFFFF0000) >> 16);
    IDL0 = (uint8_t)(ID & 0x000000FF);
    if(IDL0 > MaxTouTabNum) return RS_ERR_OTHERS;
    
    switch(IDH)
    {
        case 0x0401:
            if(IDL0 == 0)   //时区
                addr1 = Season_Table0;
            else            //时段
                addr1 = Day_Table00;
            break;
        case 0x0402:
            if(IDL0 == 0)   //时区
                addr1 = Season_Table1;
            else            //时段
                addr1 = Day_Table10;
            break;
        default:
            return RS_ERR_OTHERS;
    }
    memcpy(Addr, (uint8_t*)&addr1, 4);
    *Format = FORMAT_0D;
    addr1 = CommData_WR + CommData_E2PROM + CommData_SECONDLY + 3*MaxSeasonNum;
    memcpy(Info, (uint8_t*)&addr1, 2);
    return 0;
}

uint8_t get_645holiday_info( struct instance_t* instance,  uint8_t* prx, uint32_t ID, uint8_t* Addr, uint8_t* Format, uint8_t* Info)
{
    uint8_t IDL0=0,temp[6]={0,};
    uint32_t addr1 = 0;
    uint16_t year=0;
    
    IDL0 = ID & 0x000000FF;
    if((IDL0 > MaxHolidayNum) || (IDL0 == 0))    return RS_ERR_OTHERS;
    
    temp[5] = BCD_BYTE(*(prx+12));
    temp[3] = BCD_BYTE(*(prx+13));     //日
    temp[2] = BCD_BYTE(*(prx+14));     //月
    temp[1] = BCD_BYTE(*(prx+15));     //年
    temp[4] = GetWeek(&temp[3]);      //周
    year = temp[1]+2000;
    temp[1] = (uint8_t)year;
    temp[0] = (uint8_t)(year>>8);

    mem_swap_cpy(prx+12, temp, 6);
    
    addr1 = HoliDay_Table + IDL0 - 1;
    memcpy(Addr, (uint8_t*)&addr1, 4);
    *Format = FORMAT_00;
    addr1 = CommData_WR + CommData_E2PROM + CommData_SECONDLY + 6;
    memcpy(Info, (uint8_t*)&addr1, 2);
    
    return 0;
}

int8_t GetCommAddrInfo(struct instance_t* instance, uint8_t *p_rx, uint32_t ID, uint8_t* Addr, uint8_t* Format, uint8_t* Info)
{
    uint8_t err = 0;
    uint16_t IDH = 0;
    
    IDH = (uint16_t)((ID & 0xFFFF0000) >> 16);
    
    switch(IDH)    
    {
        case 0x0404:    //显示设置
            err = get_645displayid_info(instance, p_rx, ID, Addr, Format, Info);
            return err;
        case 0x0401:    //第一套时区、时段表数据
#if(DEF_FIRST_ZONE_SET_EN == 0) //不可设时
            return RS_ERR_OTHERS;
            break;
#endif
        case 0x0402:    //第二套时区、时段表数据
            err = get_645daytable_info(instance, p_rx, ID, Addr, Format, Info);
            return err;
        case 0x0403:    //公共假日
            err = get_645holiday_info(instance, p_rx, ID, Addr, Format, Info);
            return err;
        default:
            return RS_ERR_OTHERS; 
    }
    //return 0;
}
/***********************************************************************/
/*  函数名称：CommWrite_SpecProc                                       */
/*  函数功能：关于通讯写的特殊处理                                     */
/*  输入参数：ID：通讯ID                                               */
/*  输出参数：0为写EEPROM完成,其余为未写                              */
/***********************************************************************/
unsigned char CommWrite_SpecProc( struct instance_t* instance,  unsigned char* prx, unsigned long ID )
{
    unsigned char buf[10] = {0,},settimebuf[7]={0,},day=0;
    unsigned long s_index = 0,temp=0;
    signed char ret = 0,err=0;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t IDH = 0;
    
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    IDH = (uint16_t)((ID & 0xFFFF0000) >> 16);
    
	switch( ID )
	{
		case 0xA8000202:    //计量参数初始化
                meteric_para_clr(instance, 0);
                emu_calibration_clear(instance);
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
                //rn8209c_set(instance, prx);
                break;
		case 0xA6000305://启动阈值和脉冲速率
                memcpy(buf, prx, 4);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);//写入脉冲速率
                nd_set_index(instance, s_index, buf, 4);
				break;
        case 0xA8000208://单点校表
                //0000C350 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 0000D742 0000C350 000055F0
                memcpy((unsigned char*)&factory3.Ua, prx, 4); //电压220.00V，2位小数
                memcpy((unsigned char*)&factory3.Ia, prx+4, 4); //电流5.0000A，4位小数
                memcpy((unsigned char*)&factory3.Pa, prx+8, 4); //有功功率550.00W，2位小数
                memcpy((unsigned char*)&factory3.Ian, prx+48, 4);//零线电流5.0000A，4位小数
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
        case 0xA800020B:    //小信号修正
                adj_poffset.tick = 0;
                adj_poffset.flag = 0xA55AA55A;
                adj_poffset.power_avg = 0;
                memcpy((uint8_t*)&adj_poffset.ps, prx, 4);
                break;
        case 0xA8F00053://模拟停电时间（补冻结用）
                mbase_cb->v_powerdown_tm.tm_year = BCD_BYTE((*(prx+5))) + 2000 - 1900;
                mbase_cb->v_powerdown_tm.tm_mon = BCD_BYTE((*(prx+4))) - 1;
                mbase_cb->v_powerdown_tm.tm_mday = BCD_BYTE((*(prx+3)));
                mbase_cb->v_powerdown_tm.tm_hour = BCD_BYTE((*(prx+2)));
                mbase_cb->v_powerdown_tm.tm_min = BCD_BYTE((*(prx+1)));
                mbase_cb->v_powerdown_tm.tm_sec = BCD_BYTE((*prx));
                return 0;
                break;                
        case 0xA8FFFFF0:    //设置正向电能底度
                memcpy((uint8_t*)&temp, prx, 4);
                Energy_Set(instance, temp, 0);
                break;
        case 0xA8FFFFF1:    //设置反向电能底度
                memcpy((uint8_t*)&temp, prx, 4);
                Energy_Set(instance, temp, 1);
                break;
        default:
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
uint8_t CommWrite_DataCheck( struct instance_t* instance, uint32_t ID, uint8_t *p_rx, uint8_t * Len )
{
	unsigned char Err_Byte=0;
	unsigned char IDL0,IDL1,IDH0,IDH1,Num = 0;
	unsigned int Tmp = 0;
    uint16_t u16_tmp = 0;
    uint32_t s_index = 0;
    struct mbase_control_block_t* mbase_cb;
	uint8_t buftmp[50] = {0};
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	IDH1 = (unsigned char)((ID & 0xFF000000)>>24);
	IDH0 = (unsigned char)((ID & 0x00FF0000)>>16);
	IDL1 = (unsigned char)((ID & 0x0000FF00)>>8);
	IDL0 = (unsigned char)((ID & 0x000000FF));
	switch( ID )
	{
		case 0x04000101://日期及星期
                Err_Byte = Date_645Format_Check( p_rx );
                mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x04000102://
                Err_Byte = Time_645Format_Check( p_rx );
                mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x0400010C://日期及星期时间
				if( Time_645Format_Check( p_rx ) == 0 )
					Err_Byte = Date_645Format_Check( p_rx+3);
				else
					Err_Byte = 1;

				mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x04000601://组合有功特征字
//				HisRecord_Write(MeasModeProRec,START);
				break;
		case 0x04000201://年时区数
		        u16_tmp = BCD_BYTE(*p_rx);
				if(( u16_tmp > MaxSeasonNum ) || ( u16_tmp == 0))
					Err_Byte = RS_ERR_SEAOV;

			    mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x04000202://日时段表数
		        u16_tmp = BCD_BYTE(*p_rx);
				if(( u16_tmp > MaxTouTabNum ) || ( u16_tmp == 0))
					Err_Byte = RS_ERR_OTHERS;
				mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;	
				break;
		case 0x04000203://日时段数
				u16_tmp = BCD_BYTE(*p_rx);
				if(( u16_tmp > MaxTouNum ) || ( u16_tmp == 0))
					Err_Byte = RS_ERR_TOUOV;
                mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x04000204://费率数
				if( BCD_BYTE(*p_rx) > 12 )
					Err_Byte = RS_ERR_FEEOV;
                mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
				break;
		case 0x04000A02:
		case 0x04000A03:
		case 0x04000A04:
		case 0x04000A05:
				if((*p_rx == 0x00) && (*(p_rx+1) == 0x00))
					Err_Byte = RS_ERR_OTHERS;
				break;
		#if FEIKONG_MODE == 0
		case 0x04000207://梯度数
				if( BCD_BYTE(*p_rx) > MaxStepNum )
					Err_Byte = RS_ERR_OTHERS;
				break;
		#endif
		case 0x04000205://公共假日数
                u16_tmp = buf_to_uint16_lsb(p_rx);
				Tmp = word_bcd_to_hex( u16_tmp );
				if( Tmp > MaxHolidayNum )
					Err_Byte = RS_ERR_OTHERS;
				mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;	
				break;
		case 0x04000802://周休日时段表号
				//E2P_Read( (unsigned char*)&Num, Day_Table_Num, 1 );
				Num = MaxTouTabNum; 
				if( ((*p_rx) == 0) || ((*p_rx) > MaxTouTabNum ) )
					Err_Byte = RS_ERR_OTHERS;
                //else
					//HisRecord_Write(WeekendProRec,START);
					
                nd_get(instance, 0x40130200, buftmp, 2, NULL);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_WeekTab);
                nd_set_index(instance, s_index, buftmp, 2);
                //不一定设置成功
                record_event_no_mesg_pro(instance, NULL, WeekTabWrS, NULL, 1);// 周休日编程事件 

				mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;	
				break;
		case 0x04000106://切换时间
		case 0x04000107:
		#if FEIKONG_MODE == 0
		case 0x04000108:
		case 0x04000109:
		#endif
				mbase_cb->Fee |= F_ChangeTmOn;
				break;
		case 0x04001203://日冻结时间
				if( (*p_rx > 0x59) || (*(p_rx+1) > 0x23) )
					Err_Byte = RS_ERR_OTHERS;
				break;
		case 0x04000B01://结算日1
		case 0x04000B02://结算日2
		case 0x04000B03://结算日3
				if(( ((*p_rx > 0x23) || (*(p_rx+1) < 0x01) || (*(p_rx+1) > 0x28)) && (*p_rx != 0x99) && (*(p_rx+1)!= 0x99))
                   || (((p_rx[0] == 0x99) || (p_rx[1] == 0x99)) && ((p_rx[0] != 0x99) || (p_rx[1] != 0x99))))
                {
					Err_Byte = RS_ERR_OTHERS;
                }
                else
				{

				    //HisRecord_Write(BalanceProRec,START);
					if( ID == 0x04000B01)//设置第一结算日时，清当前月电量
					{
						//E2P_Read( BalancDayBuffer, Balance_Time1, 2 );
						//if((BalancDayBuffer[0] == *p_rx) && (BalancDayBuffer[1] == *(p_rx+1)))
						//	break;
						//memcpy((unsigned char*)&BalanceRtc.Hour,(unsigned char*)&Clock_Block.Clock.Hour,4);
						//HisRecord_Write( Rec_MonthBalance, START );
                        //HisRecord_Write(Rec_MonthCycleBalance,START);
						//memset( (unsigned char*)&Energy_Pz.MonthPz0, 0x00, 8 );
						//E2P_Write( CurrentMonth_Pz0, (unsigned char*)&Energy_Pz.MonthPz0, 4 );
						//E2P_Write( MonthBalance_Pz0, (unsigned char*)&Energy_Pz.BalancePz0, 4 );
						//Flag.Fee |= F_STEPChange;
					}

					
				}
				break;
		case 0x04000302://自动循环显示时间
				if( (*p_rx < 0x05) || (*p_rx > 0x20) )
					Err_Byte = RS_ERR_OTHERS;
				break;
		case 0x04000308:
				if( (*p_rx < 0x05) || (*p_rx > 0x30) )
					Err_Byte = RS_ERR_OTHERS;
				break;
		case 0x04000703://485通信波特率特征字
				Err_Byte = Comm485Baut_Check( *p_rx );
				break;
		case 0x04001404://红外认证时效
				if(*p_rx == 0x00)// && (*(&Rs_DataW+1) == 0x00))
					Err_Byte = RS_ERR_OTHERS;
				break;
		case 0x04000208:
				if( *p_rx > 0x20)
					Err_Byte = RS_ERR_OTHERS;
				break;
/*		case 0x04000C03://通讯密码
		case 0x04000C05:
				if( Rs_Pass > *p_rx )
					Err_Byte = RS_ERR_PASS;
				else if( *p_rx != (IDL0-1) )
					Err_Byte = RS_ERR_OTHERS;
				break;*/
		case 0x04001201://整点冻结起始时间
				if( (*p_rx != 0x00) && (*p_rx != 0x30) )//设置值应为：整点值或半点值
					Err_Byte = RS_ERR_OTHERS;
				else
					Err_Byte = ChangeTime_Format_Check( p_rx );
				break;
		case 0x04001202://整点冻结时间间隔
				if( (*p_rx != 0x15) && (*p_rx != 0x30) && (*p_rx != 0x60) )//设置值应为：15、30、60
					Err_Byte = RS_ERR_OTHERS;
				break;
		case 0x04000303://显示电能小数位数
		case 0x04000304://显示功率小数位数
				if( *p_rx > 4 )//范围0～4
					Err_Byte = RS_ERR_OTHERS;
				break;
		default:
				if( (IDH1 == 0x04) && ((IDH0 == 0x01) || (IDH0 == 0x02)) && (IDL1 == 0x00) )
				{
					if( IDL0 == 0x00 )//时区表
					{
						//if(mbase_cb->factor_mode == true)
						//	Err_Byte = 0;
						//else
							Err_Byte = SeasonTab_Format_Check( instance, p_rx, Len );
						//if(Err_Byte == 0)
							//HisRecord_Write(SeasonTabProRec,START);
					}
					else if( IDL0 <= MaxTouTabNum )//日时段表
					{
						//if(mbase_cb->factor_mode == true)
						//	Err_Byte = 0;
						//else
							Err_Byte = TouTab_Format_Check( instance, p_rx, Len );
						//if(Err_Byte == 0)
							//HisRecord_Write(DayTabProRec,START);

					}
				}
				else if( (ID >= 0x04030001) && (ID <= 0x040300FE) )//公共假日
				{
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 4);    //读取日时段个数
                    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
   					if( Num > MaxTouTabNum ) Num = MaxTouTabNum;
					if((*p_rx == 0) && (*(p_rx+1) == 0) && (*(p_rx+2) == 0) && (*(p_rx+3) == 0)
                       && (*(p_rx+4) == 0)&& (*(p_rx+5) == 0))
					{
						//HisRecord_Write(HoliDayProRec,START);
						mbase_cb->Fee |= F_TOUChange + F_DAYTABChange;
						//Flag.ProgmReport |= F_Holiday;
					}
					else if( (*p_rx == 0) || (*p_rx > Num) )
						Err_Byte = RS_ERR_OTHERS;
					else if( ((*(p_rx+2) < 1) || (*(p_rx+2) > 31)) || ((*(p_rx+3) < 1) || (*(p_rx+3) > 12)) )//日月
					    Err_Byte = RS_ERR_OTHERS;
                    else if( *(p_rx+2) > GetMonMaxDay( p_rx+3 ) )//大小月判断
				  	    Err_Byte = RS_ERR_OTHERS;
					else
					{
						//HisRecord_Write(HoliDayProRec,START);
						//Flag.Fee |= (F_TOUChange+F_STEPChange+F_DAYTABChange);
                        mbase_cb->Fee |= F_TOUChange + F_DAYTABChange;
						//Flag.ProgmReport |= F_Holiday;
					}
				}
				break;
	}

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
    unsigned char Num,IDL0,IDL1,IDH0,IDH1,Err_Byte=0;

	IDH1 = (unsigned char)((ID & 0xFF000000)>>24);
	IDH0 = (unsigned char)((ID & 0x00FF0000)>>16);
	IDL1 = (unsigned char)((ID & 0x0000FF00)>>8);
	IDL0 = (unsigned char)(ID & 0x000000FF);

	if( (IDH1 == 0x04) && ((IDH0 == 0x01) || (IDH0 == 0x02)) && (IDL1 == 0x00) )
	{
		if( IDL0 == 0x00 )//时区表
		{
			if( (DataLen%3) != 0 )//数据长度不是3的倍数
				Err_Byte = RS_ERR_LEN;
			else if( DataLen > 3*MaxSeasonNum )
				Err_Byte = RS_ERR_SEAOV;//年时区数超
		}
		else if( IDL0 <= MaxTouTabNum )//日时段表
		{
			if( (DataLen%3) != 0 )//数据长度不是3的倍数
				Err_Byte = RS_ERR_LEN;
			else if( DataLen > 3*MaxTouNum )
				Err_Byte = RS_ERR_TOUOV;//日时段数超
		}
	}
    else if((IDH1 == 0x04) && (IDH0 == 0x04) && ((IDL1 == 0x01) || (IDL1 == 0x02)))
    {
        if(TableLen != 2)   return RS_ERR_LEN;
    }
    else if((IDH1 == 0x04) && (IDH0 == 0x03))
    {
        if(TableLen != 6)   return RS_ERR_LEN;
    }
    else if((IDH1 == 0x04) && (IDH0 == 0x00) && (IDL1 == 0x02) && (IDL0 == 0x05))
    {
        if(DataLen != 2)   return RS_ERR_LEN;
    }
	else
	{
		if( DataLen != TableLen )
			Err_Byte = RS_ERR_LEN;
	}

	return Err_Byte;
}

/***********************************************************************/
/*  函数名称：Comm_Write                                               */
/*  函数功能：红外通讯写                                               */
/*  输出参数：读结果：0为正确,其余为错误                              */
/***********************************************************************/
signed char Comm_Write(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len, struct context_645_t* ctx)
{
    signed char ret=0;
	unsigned char Err_Byte=0,Ptr,Format=0,rx_len=0,buf[32]={0,};
	uint32_t s_index=0, Addr,Info,addr1,block=0,ID;
	struct mbase_control_block_t* mbase_cb;
    int16_t err = 0;
	
	mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        Err_Byte = RS_ERR_OTHERS;
        return Err_Byte;
    }
    
    err = Check_para_Pro(instance);

    rx_len = *(p_rx-1);
	memcpy((unsigned char*)&ID, p_rx, 4);


    if(false == Search_DL645_Prepay_IDTab(instance, ID, 0x14)) //表型相关
    {
        Err_Byte = RS_ERR_ID;
        return Err_Byte;
    }


	//根据ID,获取相应的数据地址和信息
	Ptr = GetCommTable( ID );//查表
	if( Ptr != 0xFF )
    {
		Addr = CommTab[Ptr].Addr;
        Format = CommTab[Ptr].format;
        Info = CommTab[Ptr].Info;
    }
    else
    {
        if(GetCommAddrInfo(instance, p_rx, ID, (uint8_t*)&Addr, (uint8_t*)&Format, (uint8_t*)&Info) != 0)
        {
            Err_Byte = RS_ERR_OTHERS;
            return Err_Byte;
        }
    }

	if( (Info & CommData_W) == 0 )//不可写
    {
        Err_Byte = RS_ERR_WR;
        return Err_Byte;
    }

	*len = (unsigned char)(Info & CommDataLen);
    
    if(ID == 0xA8000100)
    {
        if((mbase_cb->CoverOpenS_status != EVENT_SET) && (*(p_rx+12) == 0xAA) && (mbase_cb->key_state == false))  return RS_ERR_PASS;
        
        if((*(p_rx+12) == 0xAA) || (*(p_rx+12) == 0xFB))
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

    //获取密码
    memcpy((unsigned char*)&ctx->password[0], p_rx+4, 4);
    
    //安全验证
    ret = security_judge_645(instance, p_rx, p_tx, len, ctx);
    if(ret < 0)
    {
        return (uint8_t)-ret;
    }
    rx_len = *(p_rx-1); //重新获取长度
	if(ret > 0)//工厂模式或明文模式下 
	{
		//数据长度判断
		Err_Byte = Cmp_CommDataLen( ID, rx_len-12, *len );
		if( Err_Byte != 0 )
        {
            return Err_Byte;
        }
		//*len = rx_len-12;

		Err_Byte = CommWrite_DataCheck( instance, ID, p_rx+12, len );//特殊判断
		if( Err_Byte != 0 )
        {
			return Err_Byte;
        }
        
        if(Format != FORMAT_00)
        {
            if(dl645wt_chg_data(instance, Addr, Format, p_rx+12, len) == 0)     return 0;
        }

        ret = CommWrite_SpecProc( instance, p_rx+12, ID );
		if( ret == 1 )//特殊写操作
		{
            if((Info & CommData_E2PROM_BP) == CommData_E2PROM_BP)
            {
                block = mbase_block_parameter_1;
            }
            else if((Info & CommData_E2PROM) == CommData_E2PROM)
            {
                block = mbase_block_parameter;
            }
            else
            {
                return RS_ERR_OTHERS;
            }
            if(*len > 32)   
                return RS_ERR_OTHERS;
            else
                mem_swap_cpy(buf, p_rx+12, *len);
			addr1 = ((RES_DEV_EEPROM << 24) + (block << 16) + Addr);
			nd_set_index(instance, addr1, buf, *len); 
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
