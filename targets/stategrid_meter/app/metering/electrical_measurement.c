
#include <stdbool.h>
#include "targets/stategrid_meter/app/meter_flags.h"
#include <stddef.h>
#include <stdint.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "cpu_surport.h"
#include <math.h>
#include "targets/stategrid_meter/board/board_rn8209.h"
#include "targets/stategrid_meter/board/meter_status.h"
#include "targets/stategrid_meter/app/metering/rn8209_target.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "components/core/types/utils.h"
#include "domain/metering/metering_input.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/app/resource_blocks.h"


uint32_t over_Imax;
struct Rn8209AdjK_t Rn8209Adj_K;
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

static int rn8209_read(uint8_t reg, void *buf, uint8_t len)
{
    return nd_rn8209_read(board_rn8209(), reg, buf, len);
}

static int rn8209_write(uint8_t reg, const void *buf, uint8_t len)
{
    return nd_rn8209_write(board_rn8209(), reg, buf, len);
}

static int rn8209_write_raw(uint8_t reg, const void *buf, uint8_t len)
{
    return nd_rn8209_write_raw(board_rn8209(), reg, buf, len);
}

static int rn8209_command(uint8_t command)
{
    return nd_rn8209_command(board_rn8209(), command);
}

static int rn8209_apply_adjustment(void *instance,
                                   const struct Rn8209AdjSet_t *set)
{
    static const uint16_t heat_defaults[9] = {
        0x3E40, 0x3F41, 0x4042, 0x4143, 0x4244,
        0x4345, 0x4446, 0x4547, 0x4648,
    };
    uint16_t emucon2 = 0;
    uint32_t data = 0;
    uint32_t addr;
    uint8_t i;

    rn8209_command(ND_RN8209_CMD_WR_EN);
    /* The old adjustment sequence waited an extra 10 ms before the first
     * write; the chip needs the write-enable command to settle. */
    nd_rn8209_delay_us(board_rn8209(), 10000);
    if (rn8209_write(ND_RN8209_GPQA, &set->Gpq, 2) != 0
        || rn8209_write(ND_RN8209_PHSA, &set->Phs, 1) != 0
        || rn8209_write(ND_RN8209_APOSA, &set->Apos, 2) != 0
        || rn8209_write(ND_RN8209_IARMSOS, &set->Irmsos, 2) != 0) {
        return -1;
    }
    if (rn8209_read(ND_RN8209_EMUCON2, &emucon2, 2) != 0) {
        return -1;
    }
    if ((set->Phs & 0x0100u) == 0x0100u) {
        emucon2 |= 0x0100u;
    } else {
        emucon2 &= (uint16_t)~0x0100u;
    }
    if (rn8209_write(ND_RN8209_EMUCON2, &emucon2, 2) != 0) {
        return -1;
    }

    if ((nd_meter_status_metertype(instance) & F_Compen_En) == F_Compen_En) {
        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16)
                + Heat_Cmp_Value);
        nd_get_index(instance, addr, (uint8_t *)&data, 2);
        for (i = 0; i < 9; i++) {
            if (data == heat_defaults[i]) {
                break;
            }
        }
        if (i >= 9) {
            data = 0x4244;
        }
        rn8209_command(0x02);
        rn8209_command(0xBA);
        rn8209_command(0x7F);
        rn8209_command(0x5C);
        rn8209_write_raw(ND_RN8209_HEAT_CMP, &data, 2);
    }

    rn8209_command(ND_RN8209_CMD_WR_DIS);
    return 0;
}

static int rn8209_chip_start(void *instance)
{
    nd_rn8209_control_param_t param;
    struct Rn8209AdjSet_t set;
    uint32_t temp = 0;
    uint8_t adj_buf[8];
    uint32_t addr;

    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);
    nd_get_index(instance, addr, (uint8_t *)&temp, 4);
    param.pstart = (uint16_t)temp;
    param.hfconst = (uint16_t)(temp >> 16);
    param.gain = 0;
    if (param.pstart == 0 || param.pstart == 0xFFFFu) {
        param.pstart = Pstart_default;
    }
    if (param.hfconst == 0 || param.hfconst == 0xFFFFu) {
        param.hfconst = Hfconst_default;
    }

    if (nd_rn8209_control(board_rn8209(), ND_RN8209_CONTROL_POWER_ON,
                          &param) != 0) {
        nd_meter_status_emu_error_set(instance, 1);
        return -1;
    }

    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
    nd_get_index(instance, addr, adj_buf, 8);
    memcpy(&set, adj_buf, sizeof(set));
    (void)rn8209_apply_adjustment(instance, &set);
    nd_meter_status_emu_error_set(instance, 0);
    return 0;
}

int rn8209_target_power(void *instance, int power_on)
{
    if (!power_on) {
        return nd_rn8209_control(board_rn8209(), ND_RN8209_CONTROL_POWER_OFF,
                                 NULL);
    }
    return rn8209_chip_start(instance);
}

int rn8209_target_check(void *instance)
{
    nd_rn8209_control_param_t param;
    struct Rn8209AdjSet_t set;
    uint32_t temp = 0;
    uint8_t adj_buf[8];
    uint32_t addr;

    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);
    nd_get_index(instance, addr, (uint8_t *)&temp, 4);
    param.hfconst = (uint16_t)(temp >> 16);
    param.pstart = 0;
    if (param.hfconst == 0 || param.hfconst == 0xFFFFu) {
        param.hfconst = Hfconst_default;
    }
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
    nd_get_index(instance, addr, adj_buf, 8);
    memcpy(&set, adj_buf, sizeof(set));
    param.gain = set.Gpq;

    return nd_rn8209_control(board_rn8209(), ND_RN8209_CONTROL_CHECK, &param);
}

unsigned int cal_pstart(float k)
{
    unsigned int p_start=0;

    //p_start = (unsigned int)(0.8 * 220 * 5 * 0.4 / k / 256);

    p_start = (unsigned int)(0.1375 / k);
    
    return p_start;
}

float cal_kp(unsigned int hfcst)
{
    float K=0;
    
    //kp = (float)(3221550000000/(4294967296*IMP*hfcst))*100;

    K = 7500.7556/((float)hfcst*IMP);
    
    return K;
}

/*自热补偿函数*/
void heat_compensate(void *instance, void *pipeline)
{
	uint8_t i;
	uint16_t tmp[15],value=0,nowtime=0;
    uint32_t s_index=0;
    	
    if(abs(nd_mb_current_l(instance)) > 570000)
	{
		for(i=0;i<15;i++)
		{
			nowtime = (3600-(i*4)*60);
			if(adj_std_source.countstart >= nowtime)    //3600sec,60min，15个点，4分钟1个点
			{
                rn8209_read(ND_RN8209_GPQA, &value, 2);   //读取计量芯片寄存器增益值
                s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);
                nd_get_index(instance, s_index, (uint8_t*)&tmp, 30); //读取EE中的补偿值

                if(tmp[14-i] != value)
                {
                    rn8209_write(ND_RN8209_GPQA, &tmp[14-i], 2);
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
				s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
                nd_get_index(instance, s_index, (uint8_t*)&value, 2); //读取EE中的增益值
                
                rn8209_write(ND_RN8209_GPQA, &value, 2);
                adj_std_source.lock &= ~lock_heat_compensate;
                adj_std_source.countend = 0;
                adj_std_source.countstart = 0;
			}
		}
	}
}

/* 电压影响量补偿 */
void voltage_compensate(void *instance, void *pipeline, uint8_t mode)
{
	uint16_t value=0,tmp=0;
    uint32_t s_index=0;
    uint8_t buf[8];
	
	if(mode == compensate_start)    //开始
	{
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
        nd_get_index(instance, s_index, (uint8_t *)&emu_voltage_adj.un80_offset, 8); //读取EE中的补偿值

		if(nd_mb_voltage_ac(instance) > 1750)
		{
			tmp = emu_voltage_adj.un80_offset;
		}
		if(nd_mb_voltage_ac(instance) > 1970)
		{
			tmp = emu_voltage_adj.un90_offset;
		}
		if(nd_mb_voltage_ac(instance) > 2410)
		{
			tmp = emu_voltage_adj.un110_offset;
		}
		if(nd_mb_voltage_ac(instance) > 2520)
		{
			tmp = emu_voltage_adj.un115_offset;
		}
        rn8209_read(ND_RN8209_APOSA, &value, 2);   //读取计量芯片寄存器功率偏移
        
        if(tmp != value)
        {
            rn8209_command(ND_RN8209_CMD_WR_EN);
            rn8209_write(ND_RN8209_APOSA, &tmp, 2);
            adj_std_source.lock |= lock_voltage_compensate;
            rn8209_command(ND_RN8209_CMD_WR_DIS);
        }
	}
	else//结束
	{
		if((adj_std_source.lock & lock_voltage_compensate) == lock_voltage_compensate)
		{
            rn8209_command(ND_RN8209_CMD_WR_EN);
			s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            nd_get_index(instance, s_index, (uint8_t *)&buf[0], 8); //读取EE中的补偿值

            memcpy(&tmp, (uint8_t *)&buf[2], 2);//功率偏移
            rn8209_write(ND_RN8209_APOSA, &tmp, 2);
            adj_std_source.lock &= ~lock_voltage_compensate;
            rn8209_command(ND_RN8209_CMD_WR_DIS);
		}
	}
}

void emu_calibration_clear(void *instance)
{
    signed short Pgain = 0;
    unsigned char Tofst = 0;
    
    Rn8209Adj_K.u = REAL_K_U_INIT;
    Rn8209Adj_K.il = REAL_K_I_INIT;
    Rn8209Adj_K.p = REAL_K_P_INIT;
    
    rn8209_command(ND_RN8209_CMD_WR_EN);
    rn8209_write(ND_RN8209_GPQA, &Pgain, 2);
    rn8209_write(ND_RN8209_PHSA, &Tofst, 1);
    rn8209_command(ND_RN8209_CMD_WR_DIS);
    
}

uint8_t get_uip(void *instance, uint8_t *Tu, uint8_t *Ti, uint8_t *Tp, uint8_t cnt )
{
    uint8_t i=0,j;
    uint32_t U_value=0, I1_value=0, I2_value=0, P_value=0;
    uint32_t TempUrms = 0, TempIrms = 0, TempPw = 0;
    
    
    for (i = 0; i < cnt; i++)
    {
        //获取电压
        rn8209_read(ND_RN8209_URMS, &U_value, 3);
        if( U_value & 0x00800000 )//电压最高位为1时，数据无效
            U_value = 0;
        else
            U_value &= 0x00FFFFFF;
        TempUrms += U_value;

        //获取电流
        rn8209_read(ND_RN8209_IARMS, &I1_value, 3);
        if( I1_value & 0x00800000 )//电流最高位为1时，数据无效
            I1_value = 0;
        else
            I1_value &= 0x00FFFFFF;
        TempIrms += I1_value;

        //获取功率
        rn8209_read(ND_RN8209_POWERPA, &P_value, 4);
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
    memcpy(Tu, (uint8_t*)&TempUrms, 4);
    memcpy(Ti, (uint8_t*)&TempIrms, 4);
    memcpy(Tp, (uint8_t*)&TempPw, 4);
    return 0;
}

/***************************** 0.5L 100%Ib 增益，相位修正 *********************************/
void meterchip_adj_self(void *instance , struct factor_adj_meter_t *adj_data)
{
    unsigned char i=0, Tofst , j=0, adj_buf[8];
    signed short Pgain;
    unsigned long TempUrms=0, TempIrms=0, TempPw=0, CalcPw=0, Corretion_Pw=0, U_value=0, I1_value=0, I2_value=0, P_value=0;
    unsigned long BaseErr=0,addr=0;
    float err=0;
    struct factor_adj_meter_t Factory3;
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
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
        BaseErr = 0;
    }
    else
    {
        return;
    }

    if (Factory3.adjflag & F_ALadj)
    {
        get_uip(instance, (uint8_t*)&TempUrms, (uint8_t*)&TempIrms, (uint8_t*)&TempPw, 3);

        if (mem_compare( &Factory3.Ua, Buffer, 12 ) != 0)
        {
            rn8209_command(ND_RN8209_CMD_WR_EN);
            
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

            addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            nd_get_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
            memcpy((uint8_t*)&adj_buf[0], (uint8_t*)&Pgain, 2);
            nd_set_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
            
            addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
            nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16);
            
            rn8209_write(ND_RN8209_GPQA, &Pgain, 2);
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
            rn8209_write(ND_RN8209_PHSA, &Tofst, 1);
            
            addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
            adj_buf[6] = Tofst;
            nd_set_index(instance, addr, (uint8_t*)adj_buf, 8);
            
            rn8209_command(ND_RN8209_CMD_WR_DIS);
            
        }
    }
    if (Factory3.adjflag & F_ANadj)
    {
        if (mem_compare( &Factory3.Ian, Buffer, 4 ) != 0)
        {
            rn8209_read(ND_RN8209_IBRMS, &I2_value, 3);
            Rn8209Adj_K.in = (float)Factory3.Ian/10/I2_value;
            
            addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
            nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16); 
        }
    }
}

//取6次平均值
unsigned long Get_Aver(void *instance, unsigned char Reg_Addr)
{
    unsigned long Temp_B32, SPIOpData=0;
	unsigned char i, Cnt, Num = 6,j;
	
	Temp_B32 = 0;
	Cnt = 0;
	for(i=0; i<Num; i++)
	{
        rn8209_read(Reg_Addr, &SPIOpData, 3);
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
unsigned char IxRMSOS_Adjust(void *instance, unsigned char CH)
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
	
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + index);    
    nd_get_index(instance, s_index, temp_buffer, 8);
    
	if(CH == 1) SPIOpData = Get_Aver(instance, 0X22);
	else SPIOpData = Get_Aver(instance, 0X23);

	SPIOpData = SPIOpData * SPIOpData;		//平方

	SPIOpData = ~SPIOpData+1;
	SPIOpData = (SPIOpData & 0x00FFFF00);					//取中间4位

    temp_buffer[4] = (unsigned char)(SPIOpData>>8);
    temp_buffer[5] = (unsigned char)(SPIOpData>>16);
    
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + index);    
    nd_set_index(instance, s_index, temp_buffer, 8);

    memcpy((uint8_t*)&offset, (uint8_t*)&temp_buffer[4], 2);
    
    rn8209_command(ND_RN8209_CMD_WR_EN);
    if(CH == 1)
        rn8209_write(ND_RN8209_IARMSOS, (uint8_t *)&offset, 2);
    else if(CH == 2)
        rn8209_write(ND_RN8209_IBRMSOS, (uint8_t *)&offset, 2);
    rn8209_command(ND_RN8209_CMD_WR_DIS);
	return 0;
}

/************************************ 5%Ib 功率偏移修正 *******************************/
void Poffset_adj_oneself(void *instance)
{
    int32_t Gpqa, Aposa;
    uint32_t s_index = 0;
    float temp2=0;
    uint8_t buffer[8],i,tmp[8]={0,};
    
    if(adj_poffset.flag != 0xA55AA55A)  return;
    
    if(adj_poffset.tick < 10)
    {
        adj_poffset.tick ++;
        adj_poffset.power_avg += nd_mb_active_power(instance);//获取功率
        if(adj_poffset.tick == 10)
        {
            if(adj_poffset.power_avg < 0)   adj_poffset.power_avg = ~adj_poffset.power_avg+1;
            adj_poffset.power_avg = adj_poffset.power_avg/10;//留1位小数
            adj_poffset.ps = adj_poffset.ps/10; //留1位小数
            s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    
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
            rn8209_command(ND_RN8209_CMD_WR_EN);
            rn8209_write(ND_RN8209_APOSA, (uint8_t *)&Aposa, 2);
            rn8209_command(ND_RN8209_CMD_WR_DIS);
             
            memcpy(&buffer[2] , &Aposa , 2);
            nd_set_index(instance, s_index, buffer, 8); //写功率偏移

            //写电压影响量补偿
            for(i=0;i<4;i++)  
            {
                memcpy((uint8_t *)&tmp[i*2], &Aposa , 2);
            }
            s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
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


uint8_t rn8209c_get( void *instance, uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t addr=0,lenth=0,i;
    int16_t tmp[15]={0,};
    uint32_t s_index=0,data=0;
    //uint8_t* buf = p_tx;
    
    addr = *p_rx;
    *p_tx = addr;
    memset( p_tx+1,0x00,4);
    if(addr >= 0xA0)    //自热影响量 0.5L
    {
        if(addr >= 0xAF)	return false;
        
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_05L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x90)//自热影响量 1.0L
    {
        if(addr >= 0x9F)	return false;

        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);
        nd_get_index(instance, s_index, (uint8_t*)&tmp[0], 30); //读取EE中的补偿值
        memcpy( (p_tx+1), (uint8_t*)&tmp[addr-0x90], 2);
        return 2;
    }
    else if(addr >= 0x80)//电压影响量
    {
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
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
        rn8209_read(addr, &data, lenth);
        
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
                rn8209_read(addr, &data, Mtable[i].len);
                memcpy( p_tx+1, (uint8_t*)&data, Mtable[i].len);
                return Mtable[i].len;
            }
        }
    }
    return false;
}

int rn8209c_set( void *instance, uint8_t *p_rx )
{
    uint8_t addr=0,lenth=0,temp_buffer[8]={0};
    int16_t tmp,temp[30]={0,};
    uint32_t s_index=0,data=0;
    uint8_t* buf = p_rx;
    
    addr = *(buf+4);//地址
    memcpy((uint8_t*)&data, buf, 2);//数据
    
    if((addr==0x07)||(addr==0x08))//相位寄存器
        lenth = 1;
    else
        lenth = 2;
    
    if(addr<=0x17)//00~17H,需要效验
    {
        rn8209_command(ND_RN8209_CMD_WR_EN);
        if(addr == 0x17)
        {
            rn8209_read(ND_RN8209_EMUCON2, &tmp, 2);
            if(data == 0x01)
            {
                data = tmp | 0x0100;
            }
            else
            {
                data = tmp & 0xFEFF;
            }
        }
        rn8209_write(addr, (uint8_t *)&data, lenth);
        memcpy((uint8_t*)&tmp, (uint8_t*)&data, lenth);
        rn8209_command(ND_RN8209_CMD_WR_DIS);
        
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    //火线增益、相位
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
            s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
            nd_get_index(instance, s_index, (uint8_t *)&temp_buffer, 8);
            temp_buffer[2] = (uint8_t)data;
            temp_buffer[3] = (uint8_t)(data>>8);
            nd_set_index(instance, s_index, (uint8_t *)&temp_buffer, 8); 
        }
          
        memset(temp_buffer, 0x00, 8);
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);    
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
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_05L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0xA0], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x90)//自热影响
    {
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Value_10L);    
        nd_get_index(instance, s_index, (uint8_t*)&temp[0], 30);
        memcpy((uint8_t*)&temp[addr-0x90], (uint8_t*)(buf), 2);
        nd_set_index(instance, s_index, (uint8_t*)&temp[0], 30);
    }
    else if(addr>=0x80)//电压影响量
    {
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);    
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
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Cmp_Value);
        nd_set_index(instance, s_index, (uint8_t*)&data, 2);//读取自热补偿值
        
        rn8209_command(ND_RN8209_CMD_WR_EN);
        rn8209_command(0x02);
        rn8209_command(0xBA);
        rn8209_command(0x7F);
        rn8209_command(0x5C);
        rn8209_write_raw(ND_RN8209_HEAT_CMP, (uint8_t*)&data, 2);
        rn8209_command(ND_RN8209_CMD_WR_DIS);
    }
    return true;
}


void meteric_para_clr(void *instance, uint8_t *p_rx)
{
    uint32_t s_index=0;
    uint16_t pstart=0,hfconst=0;
    uint8_t buf[40]={0,};
    float kp=0;
    
    memset(buf, 0x00, 8);
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);    //火线增益、相位
    nd_set_index(instance, s_index, buf, 8); 
    
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain1);    //零线增益、相位
    nd_set_index(instance, s_index, buf, 8); 
    
    pstart = Pstart_default;
    memcpy(buf, (uint8_t*)&pstart, 2);
    if(p_rx==0)
    {
        buf[2] = (uint8_t)Hfconst_default;
        buf[3] = (uint8_t)(Hfconst_default>>8);
    }
    else
    {
        memcpy((uint8_t*)&hfconst, p_rx+3, 2);
        memcpy((uint8_t*)&buf[2], p_rx+3, 2);//写入脉冲速率
        kp = cal_kp(hfconst);           //计算功率K系数
        pstart = cal_pstart(kp);        //计算启动阈值
        memcpy(buf, (uint8_t*)&pstart, 2);
    }
        
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Other);
    nd_set_index(instance, s_index, buf, 4); 
    
    memset((uint8_t*)&Rn8209Adj_K.u, 0x00, 16);
    Rn8209Adj_K.u = REAL_K_U_INIT;
    Rn8209Adj_K.il = REAL_K_I_INIT;
    if(p_rx == 0)
    {
        Rn8209Adj_K.p = REAL_K_P_INIT;
    }
    else
    {
        Rn8209Adj_K.p = kp;
    }
    
    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
    nd_set_index(instance, s_index, (uint8_t*)&Rn8209Adj_K.u, 16);//存储至EE
    
    if((nd_mb_metertype(instance) & F_Compen_En) == F_Compen_En)    //开启补偿功能
    {
        memset(buf, 0x00, 8);
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Un_Cmp_Value);
        nd_set_index(instance, s_index, (uint8_t*)&buf[0], 8);//清空电压影响量
        
        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + Heat_Cmp_Value);
        nd_set_index(instance, s_index, (uint8_t*)&buf[0], 2);   //清空自热补偿值
    }
}

void get_std_uip(uint8_t *Tu, uint8_t *Ti, uint8_t *Tp, uint8_t *p_rx)
{
    uint32_t temp = 0, std_u=0,std_i=0,std_p=0;
    
    memcpy((uint8_t*)&temp, p_rx+2, 3);
    temp = temp >> 4;
    std_u = long_bcd_tohex(temp);
    
    temp = 0;
    memcpy((uint8_t*)&temp, p_rx+6, 2);
    std_i = long_bcd_tohex(temp);
    std_i = std_i * 10;

    temp = 0;
    memcpy((uint8_t*)&temp, p_rx+9, 4);
    temp = temp >> 4;
    std_p = long_bcd_tohex(temp);
    
    memcpy(Tu, (uint8_t*)&std_u, 4);
    memcpy(Ti, (uint8_t*)&std_i, 4);
    memcpy(Tp, (uint8_t*)&std_p, 4);
}

uint8_t adj_10L(void *instance , uint8_t *p_rx)
{
    uint32_t TempUrms=0,TempIrms=0,TempPw=0,std_u=0,std_i=0,std_p=0,Corretion_Pw=0;
    uint64_t CalcPw=0;
    uint32_t addr=0,temp=0;
    uint8_t adj_buf[8];
    float a,b,err;
    signed short Pgain;
    
    get_uip(instance, (uint8_t*)&TempUrms, (uint8_t*)&TempIrms, (uint8_t*)&TempPw, 3);
    
    get_std_uip((uint8_t*)&std_u, (uint8_t*)&std_i, (uint8_t*)&std_p, p_rx);
    
    if((std_u == 0) || (std_i == 0) || (std_p == 0))    return 0;
    
    rn8209_command(ND_RN8209_CMD_WR_EN);
    
    Corretion_Pw = (unsigned long)(((float)std_u*(float)std_i/100000)/Rn8209Adj_K.p);//台体视在功率
    CalcPw = (uint64_t)TempUrms * TempIrms / 32768;//表视在功率

    Rn8209Adj_K.u = (float)std_u/TempUrms;
    Rn8209Adj_K.il = (float)std_i/TempIrms;
    
    if(CalcPw > Corretion_Pw)
    {
        err = (CalcPw - (float)Corretion_Pw)/(float)Corretion_Pw;
    }
    else 
    {
        err = (Corretion_Pw - (float)CalcPw)/(float)Corretion_Pw;
        err = -err;
    }
    
    err = (-err/(1+err));
    if (err >= 0)
    {
        Pgain = (signed short)(err * 32768);
    }
    else
    {
        Pgain = (signed short)(65535 + err * 32768);
    }
    
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
    nd_get_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
    memcpy((uint8_t*)&adj_buf[0], (uint8_t*)&Pgain, 2);
    nd_set_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
    
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
    nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16);
    
    rn8209_write(ND_RN8209_GPQA, &Pgain, 2);
    
    rn8209_command(ND_RN8209_CMD_WR_DIS);
    
    
    return 1;
}

uint8_t adj_05L(void *instance , uint8_t *p_rx)
{
    uint32_t TempUrms=0,TempIrms=0,TempPw=0,std_u=0,std_i=0,std_p=0,Corretion_Pw=0;
    uint64_t CalcPw=0;
    uint32_t addr=0,temp=0;
    uint8_t adj_buf[8];
    float a,b,err;
    signed short Ph;
    
    get_uip(instance, (uint8_t*)&TempUrms, (uint8_t*)&TempIrms, (uint8_t*)&TempPw, 3);
    
    get_std_uip((uint8_t*)&std_u, (uint8_t*)&std_i, (uint8_t*)&std_p, p_rx);
    
    if((std_u == 0) || (std_i == 0) || (std_p == 0))    return 0;
    
    rn8209_command(ND_RN8209_CMD_WR_EN);
    
    TempPw = Rn8209Adj_K.p * TempPw * 10;
    
    Corretion_Pw = std_p;//台体功率
    CalcPw = TempPw;//表功率
    
    if(CalcPw > Corretion_Pw)
    {
        err = (CalcPw - (float)Corretion_Pw)/(float)Corretion_Pw;
    }
    else 
    {
        err = (Corretion_Pw - (float)CalcPw)/(float)Corretion_Pw;
        err = -err;
    }
    
    err = (asin(-err/1.732))*100*57.29578;
    
	if(err <= 0)
	{
		Ph = (int16_t)err / 2;
	}
	else
	{
		Ph = (int16_t)(512+err) / 2;
	}
	Ph &= 0x00FF;
    
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_Pgain0);
    nd_get_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
    memcpy((uint8_t*)&adj_buf[6], (uint8_t*)&Ph, 2);
    nd_set_index(instance, addr, (uint8_t*)&adj_buf[0], 8);
    
    rn8209_write(ND_RN8209_PHSA, &Ph, 1);
    
    rn8209_command(ND_RN8209_CMD_WR_DIS);
    
    
    return 0;
}

unsigned char ft_adj_meter(void *instance, uint8_t *p_rx)
{
    uint32_t id=0,I2_value=0,in=0,addr=0;
    uint8_t cmd = 0;
    
    cmd = *p_rx;
    switch(*(p_rx-1))
    {
        case 0x07://校表初始化
            memcpy((uint8_t*)&id, p_rx, 3);
            if(id == 0x02FC01)
            {
                meteric_para_clr(instance, p_rx);
                rn8209_target_power(instance, 1);
            }
            break;
        case 0x11://校表
            switch(cmd)
            {
                case 0x11://1.0L增益，A相电流
                  adj_10L(instance , p_rx);
                  break;
                  
                case 0x15://0.5L相位
                  adj_05L(instance , p_rx);
                  break;
                  
                case 0x31://小信号修正
                  adj_poffset.tick = 0;
                  adj_poffset.flag = 0xA55AA55A;
                  adj_poffset.power_avg = 0;
                  memcpy((uint8_t*)&adj_poffset.power_avg, p_rx+9, 4);
                  adj_poffset.ps = long_bcd_tohex(adj_poffset.power_avg)/10;
                  adj_poffset.power_avg = 0;
                  break;
                  
                case 0xC0://有效值偏移修正
                  IxRMSOS_Adjust(instance, 1);
                  IxRMSOS_Adjust(instance, 2);
                  break;
                case 0x51://零线电流系数
                  memcpy((uint8_t*)&I2_value, p_rx+6, 4);
                  in = long_bcd_tohex(I2_value);
                  I2_value = 0;
                  rn8209_read(ND_RN8209_IBRMS, &I2_value, 3);
                  Rn8209Adj_K.in = (float)in/I2_value;
                  addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
                  nd_set_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16); 
                  break;
                  
                default:
                  return 1;
                  break;
            }
            break;
        default:
            break;
    }
    return 0;
    
}

void Electrical_measurement(void *instance, void *pipeline)
{
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    int32_t temp = 0;
    float pf , pa , u , i ;
    
    if(nd_mb_meter_start_cnt(instance) > 0)
    {
        nd_mb_meter_start_cnt(instance)--;
    }
    
    if(nd_mb_meter_start_cnt(instance) == 1)
    {
       start_emu_signal_pro();
    }
    
    rn8209_read(ND_RN8209_DEVICEID, &temp, 3);
    if(temp != 0x00820900)
    {
        nd_mb_emu_err_s_status(instance) = EVENT_SET;
        return;
    }
    nd_mb_emu_err_s_status(instance) = EVENT_RESET;
   
    
    //读计量芯片电压
    temp = 0;
    rn8209_read(ND_RN8209_URMS, &temp, 3);
    
    nd_mb_voltage_ac(instance) = (uint32_t)(Rn8209Adj_K.u * temp)/10;
    if(nd_mb_voltage_ac(instance) > MAX_VOLTAGE)
    {
        nd_mb_voltage_ac(instance) = UN_VOLTAGE;
    }
    if(nd_mb_voltage_ac(instance) < MIN_VOLTAGE)
    {
        nd_mb_voltage_ac(instance) = 0;
    }
    
    //读计量芯片电流
    temp = 0;
    rn8209_read(ND_RN8209_IARMS, &temp, 3);
    nd_mb_current_l(instance) = (int32_t)(Rn8209Adj_K.il * temp)/10;
    
    //视在功率
    nd_mb_apparent_power(instance) = nd_mb_current_l(instance) * nd_mb_voltage_ac(instance)/1000;
    
    //读计量芯片功率
    temp = 0;
    rn8209_read(ND_RN8209_POWERPA, &temp, 4);
    nd_mb_active_power(instance) = (int32_t)(Rn8209Adj_K.p * temp);
    
    if( nd_mb_no_filt(instance) != 0xAA)   //0xAA关闭启动一下过滤功能，用于调试工频
    {
        if(abs(nd_mb_current_l(instance)) < nd_mb_curt_33(instance))
        {
            nd_mb_current_l(instance) = 0;
            nd_mb_active_power(instance) = 0;
            nd_mb_apparent_power(instance) = 0;
        }
        
        if(abs(nd_mb_active_power(instance)) < nd_mb_power_33(instance))
        {
            nd_mb_active_power(instance) = 0;
        }
    }

    if (abs(nd_mb_active_power(instance)) > nd_mb_apparent_power(instance)) //有功大于视在时   视在等于有功 为何有功会大于视在？
    {
        nd_mb_apparent_power(instance) = abs(nd_mb_active_power(instance));
    }

    if(nd_mb_avg_cal_start_flag(instance) < 60)  //前60s 赋值当前瞬时值
    {
        nd_mb_avg_cal_start_flag(instance)++;
        nd_mb_active_avg_power(instance) = nd_mb_active_power(instance);
    } 

    if(avg_activepower_cnt < 60)
    {
        avg_activepower_sum += nd_mb_active_power(instance);
        avg_activepower_cnt++;
    }
    else
    {
        nd_mb_active_avg_power(instance) = avg_activepower_sum/avg_activepower_cnt;
        avg_activepower_cnt = 0;
        avg_activepower_sum = 0;
    }

    
    if(nd_mb_apparent_power(instance) < abs(nd_mb_active_power(instance)))    //视在功率小于有功功率，视在=有功
    {
       nd_mb_apparent_power(instance) = abs(nd_mb_active_power(instance));
    }

    if(nd_mb_active_power(instance) < 0)
    {
        nd_mb_power_dir(instance) = 1;
    }
    else
    {
        nd_mb_power_dir(instance) = 0;
    }
    
    if(abs(nd_mb_current_l(instance)) > 70000)
    {
        over_Imax = 0x55;
    }
    else
    {
        over_Imax = 0;
    }
    
    
    //读计量芯片频率 
    temp = 0;
    rn8209_read(ND_RN8209_UFREQ, &temp, 2);
    nd_mb_frequency(instance) = (uint32_t)((44744312.5/temp)*Fr_K);
    
    //读计量芯片零线电流
    temp = 0;
    rn8209_read(ND_RN8209_IBRMS, &temp, 3);
    nd_mb_current_n(instance) = (int32_t)(Rn8209Adj_K.in * temp);
    //读计量芯片零线电流功率
    temp = 0;
    rn8209_read(ND_RN8209_POWERPB, &temp, 4);
    if(temp < 0)
    {
        nd_mb_current_n(instance) = -nd_mb_current_n(instance);
    }
    if(abs(nd_mb_current_n(instance)) < MIN_CURRENT)
    {
        nd_mb_current_n(instance) = 0;
    }   
    //計算功率因數
    if((nd_mb_voltage_ac(instance) != 0) && (nd_mb_current_l(instance) != 0))
    {
        u = (float)nd_mb_voltage_ac(instance)/10;
        i = (float)nd_mb_current_l(instance)/1000;
        pa = (float)nd_mb_active_power(instance)/10;
        pf = pa / (u*i);
        if(pf>1) pf = 1;
        if(pf<-1) pf = -1;
        nd_mb_power_factor(instance) = (int32_t)(pf * 1000);
    }
    else
    {
        nd_mb_power_factor(instance) = 1000;
    }
    
    if(nd_mb_power_dir(instance) == 1) 
    {
        nd_mb_current_l(instance) = -nd_mb_current_l(instance);
    }
    
    /****************** 补偿功能 ******************/
    if((nd_mb_metertype(instance) & F_Compen_En) == F_Compen_En)
    {
        //heat_compensate(instance, pipeline);    //自热补偿
        
        if((nd_mb_voltage_ac(instance) < 19900) || (nd_mb_voltage_ac(instance) > 23900))
        {
            voltage_compensate(instance, pipeline, compensate_start);
        }
        else
        {
            voltage_compensate(instance, pipeline, compensate_end);
        }
    }
    
    /****************** 温度采样 ******************/
    temp = 0;
    if (nd_metering_input_read(ND_METERING_INPUT_TEMPERATURE, &temp))
    {
        nd_mb_temperature(instance) = temp/100;
    }
    
    /****************** 电池采样 ******************/
    temp = 0;
    if (nd_metering_input_read(ND_METERING_INPUT_BATTERY, &temp))
    {
        nd_mb_voltage_battery(instance) = temp/100;
        if(temp < 26000)
        {
            nd_mb_voltage_battery(instance) = 0;
        }
    }    

}


void Electrical_measurement_poweron(void *instance, void *pipeline)
{
    unsigned long addr=0;
//    int32_t temp = 0;
    
    nd_mb_voltage_ac(instance) = 0;
    nd_mb_current_l(instance) = 0;
    nd_mb_active_power(instance) = 0;
    nd_mb_current_n(instance) = 0;
    nd_mb_frequency(instance) = 0;
    nd_mb_power_factor(instance) = 1000;
    
    nd_mb_p_pluse(instance) = 0;
    nd_mb_n_pluse(instance) = 0;
    
    
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ADJ_K);
    nd_get_index(instance, addr, (uint8_t*)&Rn8209Adj_K.u, 16);

    rn8209_target_power(instance, 1);
    
    nd_mb_meter_start_cnt(instance) = 3;
    
    /********* 自热 *********/
    adj_std_source.countstart = 0;
    adj_std_source.countend = 0;
    adj_std_source.lock = 0;
    
    /********* 电压补偿 *********/
    memset((uint8_t*)&emu_voltage_adj.un80_offset, 0x00, 8);
    
    avg_activepower_cnt = 0;
    avg_activepower_sum = 0;
    nd_mb_active_avg_power(instance) = 0;
    nd_mb_avg_cal_start_flag(instance) = 0;
    
    /********* 各类门限 *********/
    nd_mb_volt_60(instance) = AS_Un*0.6;   //60%电压
    nd_mb_volt_78(instance) = AS_Un*0.78;   //78%电压
    nd_mb_volt_120(instance) = AS_Un*1.20;  //120%电压
    
    nd_mb_curt_33(instance) = (uint32_t)(AS_Ib*0.0033);  //0.3%电流
    nd_mb_power_33(instance) = (uint32_t)(AS_P*0.0033);  //启动功率0.33%*P
}


void Electrical_measurement_poweroff(void *instance, void *pipeline)
{

    
    
}


