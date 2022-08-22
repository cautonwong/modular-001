
#include "nidavellir.h"
#include "applications.h"
#include <math.h>
#include "ndvh_rn8209.h"
#include "cpu_surport.h"
#include "pin_function_def.h"


extern const struct dev_info_t rn8209_dev;
    
extern void start_emu_signal_pro(void);
extern unsigned char WriteRn8209(unsigned char ChipAddr, unsigned char* Buf, unsigned char length , const struct dev_info_t* dev);
extern void  mcu_clear_watchdog(void);
extern uint8_t mcu_delay_us(volatile uint16_t count);
uint32_t over_Imax;
static uint8_t Buffer[50];
//static struct factor_adj_meter_t Factory3;
static uint16_t avg_activepower_cnt;
static int32_t avg_activepower_sum;

#define	lock_heat_compensate	0x5a
#define	lock_voltage_compensate	0xa5
#define	compensate_end			0x00
#define	compensate_start		0x01

struct adj_std_source_t
{
    int32_t volatge;      /**< 0.01V */
    int32_t current;      /**< 0.1ma */
    int32_t actPower;     /**< 0.01W */
    int32_t reactPower;   /**< 0.01W */
    int32_t countstart;
    int32_t countend;
    int8_t lock;
};

struct emu_voltage_adj_t
{
    uint16_t un80_offset;    
    uint16_t un90_offset;   
    uint16_t un110_offset;   
    uint16_t un115_offset;   
};

static struct adj_std_source_t	adj_std_source;
static struct emu_voltage_adj_t	emu_voltage_adj;

/*自热补偿函数*/
void heat_compensate(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint8_t i;
	uint16_t tmp[15],value=0,nowtime=0;
    uint32_t s_index=0;
    	
    if(abs(mbase_control_block->current_l) > 570000)
	{
		for(i=0;i<15;i++)
		{
			nowtime = (3600-(i*4)*60);
			if(adj_std_source.countstart >= nowtime)    //3600sec,60min，15个点，4分钟1个点
			{
                rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , GPQA , 2 , &value);   //读取计量芯片寄存器增益值
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);
                nd_get_index(instance, s_index, (uint8_t*)&tmp, 30); //读取EE中的补偿值

                if(tmp[14-i] != value)
                {
                    rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, GPQA, 2, &tmp[14-i]);
                }
                adj_std_source.countend = 0;
                adj_std_source.lock |= lock_heat_compensate;
                
                i=16;
			}
		}
		if(adj_std_source.countstart<3600)
            adj_std_source.countstart++;
	}
	else
	{
		if((adj_std_source.lock & lock_heat_compensate) == lock_heat_compensate)
		{
			adj_std_source.countend++;
			if(adj_std_source.countend>30)
			{
				s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
                nd_get_index(instance, s_index, (uint8_t*)&value, 2); //读取EE中的增益值
                
                rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, GPQA, 2, &value);
                adj_std_source.lock &= ~lock_heat_compensate;
                adj_std_source.countend = 0;
                adj_std_source.countstart = 0;
			}
		}
	}
}

/* 电压影响量补偿 */
void voltage_compensate(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t mode)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint16_t value=0,tmp=0;
    uint32_t s_index=0;
    uint8_t buf[8];
	
	if(mode == compensate_start)    //开始
	{
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
        nd_get_index(instance, s_index, (uint8_t *)&emu_voltage_adj.un80_offset, 8); //读取EE中的补偿值

		if(mbase_control_block->voltage_ac > 1750)
		{
			tmp = emu_voltage_adj.un80_offset;
		}
		if(mbase_control_block->voltage_ac > 1970)
		{
			tmp = emu_voltage_adj.un90_offset;
		}
		if(mbase_control_block->voltage_ac > 2410)
		{
			tmp = emu_voltage_adj.un110_offset;
		}
		if(mbase_control_block->voltage_ac > 2520)
		{
			tmp = emu_voltage_adj.un115_offset;
		}
        rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , APOSA , 2 , &value);   //读取计量芯片寄存器功率偏移
        
        if(tmp != value)
        {
            CmdRn8209(CMD_WR_EN , &rn8209_dev);
            rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, APOSA, 2, &tmp);
            adj_std_source.lock |= lock_voltage_compensate;
            CmdRn8209(CMD_WR_DIS , &rn8209_dev);
        }
	}
	else//结束
	{
		if((adj_std_source.lock & lock_voltage_compensate) == lock_voltage_compensate)
		{
            CmdRn8209(CMD_WR_EN , &rn8209_dev);
			s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            nd_get_index(instance, s_index, (uint8_t *)&buf[0], 8); //读取EE中的补偿值

            memcpy(&tmp, (uint8_t *)&buf[2], 2);//功率偏移
            rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, APOSA, 2, &tmp);
            adj_std_source.lock &= ~lock_voltage_compensate;
            CmdRn8209(CMD_WR_DIS , &rn8209_dev);
		}
	}
}

#define REAL_K_U_INIT     0.0137414116
#define REAL_K_I_INIT     0.016
#define REAL_K_P_INIT     0.0023294272


void emu_calibration_clear(struct instance_t* instance)
{

    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    signed short Pgain = 0;
    unsigned char Tofst = 0;
    
    Rn8209Adj_K.u = REAL_K_U_INIT;
    Rn8209Adj_K.il = REAL_K_I_INIT;
    Rn8209Adj_K.p = REAL_K_P_INIT;
    
    CmdRn8209(CMD_WR_EN , &rn8209_dev);
    rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, GPQA, 2, &Pgain);
    rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, PHSA, 1, &Tofst);
    CmdRn8209(CMD_WR_DIS , &rn8209_dev);
    
}

/***************************** 0.5L 100%Ib 增益，相位修正 *********************************/
void meterchip_adj_self(struct instance_t* instance , struct factor_adj_meter_t *adj_data)
{
    unsigned char i=0, Tofst , j=0, adj_buf[8];
    signed short Pgain;
    unsigned long TempUrms=0, TempIrms=0, TempPw=0, CalcPw=0, Corretion_Pw=0, U_value=0, I1_value=0, I2_value=0, P_value=0;
    unsigned long BaseErr,hfconst,addr=0;
    float err=0;
    struct factor_adj_meter_t Factory3;
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    memset((unsigned char*)Buffer,0x00,16);

    //E2P_Read((unsigned char*)&hfconst, Meter_VHFconst, 4 );	//获取脉冲速率
    //E2P_Read((unsigned char*)&BaseErr, MeterBaseErr, 4);	//获取基准偏差值
    
    //测试用，后期删除
    

    
    if(adj_data != 0)
    {
        Factory3.adjflag = adj_data->adjflag;
        Factory3.Ua = adj_data->Ua;   
        Factory3.Ia = adj_data->Ia;  
        Factory3.Pa = adj_data->Pa; 
        Factory3.Ian = adj_data->Ian;  
    }
    else
    {
        Factory3.adjflag = 0xff;
        Factory3.Ua = 0x55F0;    //220V 
        Factory3.Ia = 0xC34A;    //5A
        Factory3.Pa = 0xD6FF;    //550W
        Factory3.Ian = 0xC34A;   //5A
    }
    BaseErr = 0;
    hfconst = 0x064a;
    if (Factory3.adjflag & F_ALadj)
    {
        for (i = 0; i < 3; i++)
        {
            //获取电压
            rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , URMS , 3 , &U_value);
            if( U_value & 0x00800000 )//电压最高位为1时，数据无效
                U_value = 0;
            else
                U_value &= 0x00FFFFFF;
            TempUrms += U_value;

            //获取电流
            rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , IARMS , 3 , &I1_value);
            if( I1_value & 0x00800000 )//电流最高位为1时，数据无效
                I1_value = 0;
            else
                I1_value &= 0x00FFFFFF;
            TempIrms += I1_value;

            //获取功率
            rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , POWERPA , 4 , &P_value);
            if(P_value >= 0x80000000)
                P_value=~P_value;
            TempPw += P_value;
            for(j=0;j<10;j++)
            {
                mcu_delay_us(50000);	//等待500ms
                mcu_clear_watchdog();
            }
        }
        TempUrms = TempUrms/3;
        TempIrms = TempIrms/3;
        TempPw   = TempPw/3;

        if (mem_compare( &Factory3.Ua, Buffer, 12 ) != 0)
        {
            CmdRn8209(CMD_WR_EN , &rn8209_dev);
            
            Corretion_Pw = (unsigned long)(((float)Factory3.Ua*(float)Factory3.Ia/100000)/Rn8209Adj_K.p);//台体视在功率
            CalcPw = (unsigned long)(((float)TempUrms * (float)TempIrms)/32768); //表视在功率

            Rn8209Adj_K.u = (float)Factory3.Ua/TempUrms;
            Rn8209Adj_K.il = (float)Factory3.Ia/TempIrms;

            err = (CalcPw - (float)Corretion_Pw)/(float)Corretion_Pw;
            err = err + BaseErr*0.01;
            err = (-err/(1+err));
            if (err >= 0)
            {
                Pgain = (signed short)(err * 32768);
            }
            else
            {
                Pgain = (signed short)(65535 + err * 32768);
            }

            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            nd_get_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
            memcpy((uint8_t*)&adj_buf[0], (uint8_t*)&Pgain, 2);
            nd_set_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
            nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16);
            
            rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, GPQA, 2, &Pgain);
            CalcPw = (unsigned long)((float)TempPw*(1+err));
            Corretion_Pw = (unsigned long)(((float)Factory3.Pa/10)/Rn8209Adj_K.p);
            err = (float)(CalcPw - Corretion_Pw)/(Corretion_Pw);
            err = (float)(asin(-err/1.732))*100*57.29578;
            if (err >= 0)
        	{
        		Tofst = (unsigned char)((unsigned int)(err)>>1);
        	}
        	else
        	{
        		Tofst = (unsigned char)(((unsigned int)(512+err))>>1);
        	}
            rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, PHSA, 1, &Tofst);
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            adj_buf[6] = Tofst;
            nd_set_index(instance, addr, (uint8_t*)adj_buf, 8);
            
            CmdRn8209(CMD_WR_DIS , &rn8209_dev);
            
            rn8209_dev.dev_driver_ops->dev_initialize(instance, &rn8209_dev);
        }
    }
    if (Factory3.adjflag & F_ANadj)
    {
        if (mem_compare( &Factory3.Ian, Buffer, 4 ) != 0)
        {
            rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , IBRMS , 3 , &I2_value);
            Rn8209Adj_K.in = (float)Factory3.Ian/10/I2_value;
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
            nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16); 
        }
    }
}

//取6次平均值
unsigned long Get_Aver(struct instance_t* instance, unsigned char Reg_Addr)
{
    unsigned long Temp_B32, SPIOpData=0;
	unsigned char i, Cnt, Num = 6,j;
	
	Temp_B32 = 0;
	Cnt = 0;
	for(i=0; i<Num; i++)
	{
        rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , Reg_Addr , 3 , &SPIOpData);
        if(SPIOpData & 0x00800000) //最高位为1做零处理(电流非法)
        {
            SPIOpData = 0;
        }
        else
        {
            SPIOpData = SPIOpData & 0x00FFFFFF;
            Cnt++;
        }
            
        Temp_B32 += SPIOpData;
		
		for(j=0;j<10;j++)
        {
            mcu_delay_us(5000);	//等待500ms
            mcu_clear_watchdog();
        }
	}
	if( Cnt == 0 ) Cnt = 1;
	SPIOpData = Temp_B32/Cnt;
	
	return (SPIOpData);
}

//电流A/B有效值偏置修正(220V,I=0,1.0)
unsigned char IxRMSOS_Adjust(struct instance_t* instance, unsigned char CH)
{
	unsigned long SPIOpData,s_index=0,index=0;
    unsigned char temp_buffer[8]={0,};
    int32_t offset = 0;
    
    if(CH == 1)
    {
        index = ADJ_Pgain0;
    }
    else if(CH == 2)
    {
        index = ADJ_Pgain1;
    }
	
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + index);    
    nd_get_index(instance, s_index, temp_buffer, 8);
    
	if(CH == 1) SPIOpData = Get_Aver(instance, 0X22);
	else SPIOpData = Get_Aver(instance, 0X23);

	SPIOpData = SPIOpData * SPIOpData;		//平方

	SPIOpData = ~SPIOpData+1;
	SPIOpData = (SPIOpData & 0x00FFFF00);					//取中间4位

    temp_buffer[4] = (unsigned char)(SPIOpData>>8);
    temp_buffer[5] = (unsigned char)(SPIOpData>>16);
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + index);    
    nd_set_index(instance, s_index, temp_buffer, 8);

    memcpy((uint8_t*)&offset, (uint8_t*)&temp_buffer[4], 2);
    
    CmdRn8209(CMD_WR_EN , &rn8209_dev);
    if(CH == 1)
        rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, IARMSOS, 2, (uint8_t*)&offset);
    else if(CH == 2)
        rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, IBRMSOS, 2, (uint8_t*)&offset);
    CmdRn8209(CMD_WR_DIS , &rn8209_dev);
	return 0;
}

/************************************ 5%Ib 功率偏移修正 *******************************/
void Poffset_adj_oneself(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int32_t Gpqa, Aposa;
    uint32_t s_index = 0;
    float temp2=0;
    uint8_t buffer[8],i,tmp[8]={0,};
    
    if(adj_poffset.flag != 0xA55AA55A)  return;
    
    if(adj_poffset.tick < 10)
    {
        adj_poffset.tick ++;
        adj_poffset.power_avg += mbase_control_block->active_power;//获取功率
        if(adj_poffset.tick == 10)
        {
            adj_poffset.power_avg = adj_poffset.power_avg/10;//留1位小数
            adj_poffset.ps = adj_poffset.ps/10; //留1位小数
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    
            nd_get_index(instance, s_index, buffer, 8);
            memcpy(&Gpqa, &buffer[0], 2);   //取增益
            
            if(Gpqa&0x8000)
            {
                temp2 = (float)(Gpqa - 65536)/32768;
            }
            else
            {
                temp2 = (float)Gpqa / 32768;
            }
            Aposa = (int32_t)((adj_poffset.ps/Rn8209Adj_K.p - adj_poffset.power_avg/Rn8209Adj_K.p)/(1 + temp2));
                
            if(Aposa <0)
            {
                Aposa += 65535;
            }
            CmdRn8209(CMD_WR_EN , &rn8209_dev);
            rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, APOSA, 2, (uint8_t*)&Aposa);
            CmdRn8209(CMD_WR_DIS , &rn8209_dev);
             
            memcpy(&buffer[2] , &Aposa , 2);
            nd_set_index(instance, s_index, buffer, 8); //写功率偏移

            //写电压影响量补偿
            for(i=0;i<4;i++)  
            {
                memcpy((uint8_t *)&tmp[i*2], &Aposa , 2);
            }
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
            nd_set_index(instance, s_index, (uint8_t *)tmp, 8); 
        }
    }
}

typedef struct 
{
    uint8_t address;
    uint8_t len;
}meterchip;

const  meterchip  Mtable[9]=
{
    0x20,   2,  //快速有功脉冲计数器
    0x22,   3,  //通道A电流的有效值
    0x23,   3,  //通道B电流的有效值
    0x24,   3,  //电压有效值
    0x26,   4,  //有功功率A
    0x27,   4,  //有功功率B
    0x28,   4,  //无功功率
    0x2d,   3,  //计量状态及校验和寄存器
    0x50,   2,  //自热补偿寄存器
};


int rn8209c_get( struct instance_t* instance, uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t addr=0,lenth=0,i;
    int16_t tmp[15]={0,};
    uint32_t s_index=0,data=0;
    //uint8_t* buf = p_tx;
    
    addr = *(p_rx+4);
    *p_tx = addr;
    memset( p_tx+1,0x00,4);
    if(addr >= 0xA0)    //自热影响量 0.5L
    {
        if(addr >= 0xAF)	return false;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_05L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x90)//自热影响量 1.0L
    {
        if(addr >= 0x9F)	return false;

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x80)//电压影响量
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
        nd_get_index(instance, s_index, (uint8_t *)&tmp[0], 8); //读取EE中的补偿值
        switch(addr)
        {
            case 0x80://80%
                memcpy( p_tx+1, (uint8_t*)&tmp[0], 2);
                break;
            case 0x81://90%
                memcpy( p_tx+1, (uint8_t*)&tmp[1], 2);
                break;
            case 0x82://110%
                memcpy( p_tx+1, (uint8_t*)&tmp[2], 2);
                break;
            case 0x83://115%
                memcpy( p_tx+1, (uint8_t*)&tmp[3], 2);
                break;
            default:
                return false;
                break;
        }
        return 2;
    }
    else if(addr <= 0x17)
    {
        if((addr==0x07)||(addr == 0x08))
            lenth = 1;
        else
            lenth = 2;
        rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , addr , lenth , &data);
        
        if(addr == 0x17)
        {
            data = data>>8;
        }
        memcpy( p_tx+1, (uint8_t*)&data, lenth);
        
        return lenth;
    }
    else
    {
        for(i=0; i<9; i++)
        {
            if(addr == Mtable[i].address)
            {
                rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , addr , Mtable[i].len , &data);
                memcpy( p_tx+1, (uint8_t*)&data, Mtable[i].len);
                return Mtable[i].len;
            }
        }
    }
    return -1;
}

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
            rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , EMUCON2 , 2 , &tmp);
            if(data == 0x01)
            {
                data = tmp | 0x0100;
            }
            else
            {
                data = tmp & 0xFEFF;
            }
        }
        rn8209_dev.dev_driver_ops->dev_write(instance, &rn8209_dev, addr, lenth, (uint8_t*)&data);
        memcpy((uint8_t*)&tmp, (uint8_t*)&data, lenth);
        CmdRn8209(CMD_WR_DIS , &rn8209_dev);
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    //火线增益、相位
        nd_get_index(instance, s_index, temp_buffer, 8); 
        switch(addr)//存入EEPROM
        {
            case 0x05://增益
                temp_buffer[0] = (uint8_t)data;
                temp_buffer[1] = (uint8_t)(data>>8);
                break;
            case 0x0A://功率偏置
                temp_buffer[2] = (uint8_t)data;
                temp_buffer[3] = (uint8_t)(data>>8);
                break;
            case 0x0E://A相电流偏移
                temp_buffer[4] = (uint8_t)data;
                temp_buffer[5] = (uint8_t)(data>>8);
                break;    
            case 0x07://相位
                temp_buffer[6] = (uint8_t)data;
                break;
            case 0x17://相位微调
                temp_buffer[7] = (uint8_t)(data>>8);
                break;
        }
        nd_set_index(instance, s_index, temp_buffer, 8); 
        if(addr == 0x0A)    //功率偏置设置时刷新电压影响量参数
        {
            memset(temp_buffer, 0x00, 8);
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
            nd_get_index(instance, s_index, (uint8_t *)&temp_buffer, 8);
            temp_buffer[2] = (uint8_t)data;
            temp_buffer[3] = (uint8_t)(data>>8);
            nd_set_index(instance, s_index, (uint8_t *)&temp_buffer, 8); 
        }
          
        memset(temp_buffer, 0x00, 8);
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);    
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
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_05L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0xA0], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x90)//自热影响
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0x90], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x80)//电压影响量
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);    
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
    else if(addr == 0x51)   //自热补偿（硬件）
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Cmp_Value);
        nd_set_index(instance, s_index, (uint8_t*)&data, 2);//读取自热补偿值
        
        CmdRn8209(CMD_WR_EN, &rn8209_dev);
        CmdRn8209(0x02, &rn8209_dev);
        CmdRn8209(0xBA, &rn8209_dev);
        CmdRn8209(0x7F, &rn8209_dev);
        CmdRn8209(0x5C, &rn8209_dev);
        WriteRn8209( 0x51, (uint8_t*)&data, 2 , &rn8209_dev);
        CmdRn8209(CMD_WR_DIS, &rn8209_dev);
    }
    return true;
}

void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline)
{
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int32_t temp = 0;
    float pf , pa , u , i ;
    
    if(mbase_control_block->meter_start_cnt > 0)
    {
        mbase_control_block->meter_start_cnt--;
    }
    
    if(mbase_control_block->meter_start_cnt == 1)
    {
       start_emu_signal_pro();
    }
   
    //读计量芯片电压
    rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , URMS , 3 , &temp);
    
    mbase_control_block->voltage_ac = (uint32_t)(Rn8209Adj_K.u * temp)/10;
    if(mbase_control_block->voltage_ac > MAX_VOLTAGE)
    {
        mbase_control_block->voltage_ac = UN_VOLTAGE;
    }
    if(mbase_control_block->voltage_ac < MIN_VOLTAGE)
    {
        mbase_control_block->voltage_ac = 0;
    }
    
    //读计量芯片电流
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , IARMS , 3 , &temp);
    mbase_control_block->current_l = (int32_t)(Rn8209Adj_K.il * temp)/10;
    
    //视在功率
    mbase_control_block->apparent_power = mbase_control_block->current_l * mbase_control_block->voltage_ac/1000;
    
    //读计量芯片功率
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , POWERPA , 4 , &temp);
    mbase_control_block->active_power = (int32_t)(Rn8209Adj_K.p * temp);
    
    if( mbase_control_block->no_filt != 0xAA)   //0xAA关闭启动一下过滤功能，用于调试工频
    {
        if(abs(mbase_control_block->current_l) < mbase_control_block->curt_33)
        {
            mbase_control_block->current_l = 0;
            mbase_control_block->active_power = 0;
            mbase_control_block->apparent_power = 0;
        }
        
        if(abs(mbase_control_block->active_power) < mbase_control_block->power_33)
        {
            mbase_control_block->active_power = 0;
        }
    }
    
    if(mbase_control_block->active_avg_power == 0)
    {
        mbase_control_block->active_avg_power = mbase_control_block->active_power;
    }
    else
    {
        if(avg_activepower_cnt < 60)
        {
            avg_activepower_sum += mbase_control_block->active_power;
            avg_activepower_cnt++;
        }
        else
        {
            mbase_control_block->active_avg_power = avg_activepower_sum/avg_activepower_cnt;
            avg_activepower_cnt = 0;
            avg_activepower_sum = 0;
        }
    }

    if(mbase_control_block->active_power < 0)
    {
        mbase_control_block->power_dir = 1;
    }
    else
    {
        mbase_control_block->power_dir = 0;
    }
    
    if(abs(mbase_control_block->current_l) > 70000)
    {
        over_Imax = 0x55;
    }
    else
    {
        over_Imax = 0;
    }
    
    
    //读计量芯片频率 
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , UFREQ , 2 , &temp);
    mbase_control_block->frequency = (uint32_t)(44744312.5/temp);
    
    //读计量芯片零线电流
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(instance, &rn8209_dev , IBRMS , 3 , &temp);
    mbase_control_block->current_n = (int32_t)(Rn8209Adj_K.in * temp);
    if(abs(mbase_control_block->current_n) < MIN_CURRENT)
    {
        mbase_control_block->current_n = 0;
    }   
    //計算功率因數
    if((mbase_control_block->voltage_ac != 0) && (mbase_control_block->current_l != 0))
    {
        u = (float)mbase_control_block->voltage_ac/10;
        i = (float)mbase_control_block->current_l/1000;
        pa = (float)mbase_control_block->active_power/10;
        pf = pa / (u*i);
        if(pf>1) pf = 1;
        if(pf<-1) pf = -1;
        mbase_control_block->power_factor = (int32_t)(pf * 1000);
    }
    else
    {
        mbase_control_block->power_factor = 1000;
    }
    
    if(mbase_control_block->power_dir == 1) 
    {
        mbase_control_block->current_l = -mbase_control_block->current_l;
    }
    
    /****************** 补偿功能 ******************/
    if((mbase_control_block->metertype & 0x0800) == 0x0800)
    {
        heat_compensate(instance, pipeline);    //自热补偿
        
        if((mbase_control_block->voltage_ac < 19900) || (mbase_control_block->voltage_ac > 23900))
        {
            voltage_compensate(instance, pipeline, compensate_start);
        }
        else
        {
            voltage_compensate(instance, pipeline, compensate_end);
        }
    }
    
    /****************** 温度采样 ******************/
    if(true == mcu_adc_start_once(ADC_CHL_TEMP))
    {
        if(true == read_mcu_adc_measure_value(ADC_CHL_TEMP , (int32_t *)&temp))
        {
            mbase_control_block->temperature = temp/100;
        }
    }
    
    /****************** 电池采样 ******************/
    if(true == mcu_adc_start_once(ADC_CHL_BATTER))
    {
        if(true == read_mcu_adc_measure_value(ADC_CHL_BATTER , (int32_t *)&temp))
        {
            mbase_control_block->voltage_battery = temp/100;
            if(temp < 26000)
            {
                mbase_control_block->voltage_battery = 0;
            }
        }
    }    

}


void Electrical_measurement_poweron(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct Rn8209AdjSet_t adj_set;
    unsigned long addr=0;
    uint8_t adj_buf[8];
//    int32_t temp = 0;
    
    mbase_control_block->voltage_ac = 0;
    mbase_control_block->current_l = 0;
    mbase_control_block->active_power = 0;
    mbase_control_block->current_n = 0;
    mbase_control_block->frequency = 0;
    mbase_control_block->power_factor = 1000;
    
    mbase_control_block->p_pluse = 0;
    mbase_control_block->n_pluse = 0;
    
    
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
    nd_get_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16);

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
    nd_get_index(instance, addr, (uint8_t*)adj_buf, 8);
    memcpy((uint8_t*)&adj_set, (uint8_t*)adj_buf, 8);
    
    rn8209_dev.dev_driver_ops->dev_control(instance, &rn8209_dev , AC_POWER_ON , &adj_set);
    
    mbase_control_block->meter_start_cnt = 3;
    
    /********* 自热 *********/
    adj_std_source.countstart = 0;
    adj_std_source.countend = 0;
    adj_std_source.lock = 0;
    
    /********* 电压补偿 *********/
    memset((uint8_t*)&emu_voltage_adj.un80_offset, 0x00, 8);
    
    avg_activepower_cnt = 0;
    avg_activepower_sum = 0;
    mbase_control_block->active_avg_power = 0;
    
    /********* 各类门限 *********/
    mbase_control_block->volt_60 = AS_Un*0.6;   //60%电压
    mbase_control_block->volt_78 = AS_Un*0.78;   //78%电压
    mbase_control_block->volt_120 = AS_Un*1.20;  //120%电压
    
    mbase_control_block->curt_33 = (uint32_t)(AS_Ib*0.0033);  //0.3%电流
    mbase_control_block->power_33 = (uint32_t)(AS_P*0.0033);  //启动功率0.33%*P
}


void Electrical_measurement_poweroff(struct instance_t* instance, struct pipeline_t* pipeline)
{

    
    
}


