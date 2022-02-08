
#include "nidavellir.h"
#include "applications.h"
#include <math.h>
#include "ndvh_rn8209.h"
#include "cpu_surport.h"
#include "pin_function_def.h"

#define MAX_VOLTAGE  4000 //400v
#define MIN_VOLTAGE  500 //50v
#define UN_VOLTAGE   220000 //220v
#define MAX_CURRENT  120000 //120A
#define MIN_CURRENT  10     //10mA

extern const struct dev_info_t rn8209_dev;
    
//void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline)
struct mbase_control_block_t ctx;

extern void  mcu_clear_watchdog(void);
extern uint8 mcu_delay_us(volatile uint16 count);
uint32 over_Imax;
static uint8 Buffer[50];
static struct factor_adj_meter_t Factory3;
static uint8 key_cnt;

#define	lock_heat_compensate	0x5a
#define	lock_voltage_compensate	0xa5
#define	compensate_end			0x00
#define	compensate_start		0x01

struct adj_std_source_t
{
    int32 volatge;      /**< 0.01V */
    int32 current;      /**< 0.1ma */
    int32 actPower;     /**< 0.01W */
    int32 reactPower;   /**< 0.01W */
    int32 countstart;
    int32 countend;
    int8 lock;
};

struct emu_voltage_adj_t
{
    uint16 un80_offset;    
    uint16 un90_offset;   
    uint16 un110_offset;   
    uint16 un115_offset;   
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
                rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , GPQA , 2 , &value);   //读取计量芯片寄存器增益值
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Heat_Value_10L);
                nd_get_index(instance, s_index, (uint8_t*)&tmp, 30); //读取EE中的补偿值

                if(tmp[14-i] != value)
                {
                    rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, GPQA, 2, &tmp[14-i]);
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
				s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain0);
                nd_get_index(instance, s_index, (uint8_t*)&value, 2); //读取EE中的增益值
                
                rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, GPQA, 2, &value);
                adj_std_source.lock &= ~lock_heat_compensate;
                adj_std_source.countend = 0;
                adj_std_source.countstart = 0;
			}
		}
	}
}

/* 电压影响量补偿 */
void voltage_compensate(struct instance_t* instance, struct pipeline_t* pipeline, uint8 mode)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint16_t value=0,tmp=0;
    uint32_t s_index=0;
	
	if(mode == compensate_start)    //开始
	{
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + U80_Value);
        nd_get_index(instance, s_index, (uint8 *)&emu_voltage_adj.un80_offset, 8); //读取EE中的补偿值

		if(mbase_control_block->voltage_ac > 17500)
		{
			tmp = emu_voltage_adj.un80_offset;
		}
		if(mbase_control_block->voltage_ac > 19700)
		{
			tmp = emu_voltage_adj.un90_offset;
		}
		if(mbase_control_block->voltage_ac > 24100)
		{
			tmp = emu_voltage_adj.un110_offset;
		}
		if(mbase_control_block->voltage_ac > 25200)
		{
			tmp = emu_voltage_adj.un115_offset;
		}
        rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , APOSA , 2 , &value);   //读取计量芯片寄存器功率偏移
        
        if(tmp != value)
        {
            rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, APOSA, 2, &tmp);
            adj_std_source.lock |= lock_voltage_compensate;
        }
	}
	else//结束
	{
		if((adj_std_source.lock & lock_voltage_compensate) == lock_voltage_compensate)
		{
			s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pofst0);
            nd_get_index(instance, s_index, (uint8 *)&tmp, 2); //读取EE中的补偿值

            rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, APOSA, 2, &tmp);
            adj_std_source.lock &= ~lock_voltage_compensate;
		}
	}
}

void meterchip_adj_self(struct instance_t* instance, struct pipeline_t* pipeline)
{
    unsigned char i=0, Tofst , j=0;
    signed short Pgain;
    unsigned long TempUrms=0, TempIrms=0, TempPw=0, CalcPw=0, Corretion_Pw=0, U_value=0, I1_value=0, I2_value=0, P_value=0;
    unsigned long BaseErr,hfconst,addr=0;
    float err=0;
    
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    memset((unsigned char*)Buffer,0x00,16);

    //E2P_Read((unsigned char*)&hfconst, Meter_VHFconst, 4 );	//获取脉冲速率
    //E2P_Read((unsigned char*)&BaseErr, MeterBaseErr, 4);	//获取基准偏差值
    
    //测试用，后期删除
    
    Factory3.adjflag = 0xff;
    hfconst = 0x096F;
    BaseErr = 0;
    Factory3.Ua = 0x55F0;
    Factory3.Ia = 0xC34A;
    Factory3.Pa = 0xD6FF;
    Factory3.Ian = 0xC34A;
    
    if (Factory3.adjflag & F_ALadj)
    {
        for (i = 0; i < 3; i++)
        {
            //获取电压
            rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , URMS , 3 , &U_value);
            if( U_value & 0x00800000 )//电压最高位为1时，数据无效
                U_value = 0;
            else
                U_value &= 0x00FFFFFF;
            TempUrms += U_value;

            //获取电流
            rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IARMS , 3 , &I1_value);
            if( I1_value & 0x00800000 )//电流最高位为1时，数据无效
                I1_value = 0;
            else
                I1_value &= 0x00FFFFFF;
            TempIrms += I1_value;

            //获取功率
            rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , POWERPA , 4 , &P_value);
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
            Corretion_Pw = (unsigned long)(((float)Factory3.Ua*(float)Factory3.Ia/10000)/mbase_control_block->k_p);//台体视在功率
            CalcPw = (unsigned long)(((float)TempUrms * (float)TempIrms)/32768); //表视在功率

            mbase_control_block->k_u = (float)Factory3.Ua/TempUrms/10;
            mbase_control_block->k_il = (float)Factory3.Ia/10/TempIrms;

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

            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Pgain0);
            nd_set_index(instance, addr, (uint8_t*)&Pgain, 2);
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Ku);
            nd_set_index(instance, addr, (uint8_t*)&mbase_control_block->k_u, 8);
            
            rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, GPQA, 2, &Pgain);
            CalcPw = (unsigned long)((float)TempPw*(1+err));
            Corretion_Pw = (unsigned long)(((float)Factory3.Pa/100)/mbase_control_block->k_p);
            err = ((float)CalcPw - Corretion_Pw)/(Corretion_Pw);
            err = (asin(-err/1.732))*100*57.29578;
            if (err >= 0)
        	{
        		Tofst = (unsigned char)((unsigned int)(err)>>1);
        	}
        	else
        	{
        		Tofst = (unsigned char)(((unsigned int)(512+err))>>1);
        	}
            rn8209_dev.dev_driver_ops->dev_write(&rn8209_dev, PHSA, 1, &Tofst);
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Ph0);
            nd_set_index(instance, addr, (uint8_t*)&Tofst, 2);
            
            rn8209_dev.dev_driver_ops->dev_initialize(&rn8209_dev);
        }
    }
    if (Factory3.adjflag & F_ANadj)
    {
        if (mem_compare( &Factory3.Ian, Buffer, 4 ) != 0)
        {
            rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IBRMS , 3 , &I2_value);
            mbase_control_block->k_in = (float)Factory3.Ian/10/I2_value;
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ADJ_Kin);
            nd_set_index(instance, addr, (uint8_t*)&mbase_control_block->k_in, 4); 
        }
    }
}


void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline)
{
    
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int32_t temp = 0;
   
    //读计量芯片电压
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , URMS , 3 , &temp);
    
    mbase_control_block->voltage_ac = (uint32_t)(mbase_control_block->k_u * temp)/10;
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
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IARMS , 3 , &temp);
    mbase_control_block->current_l = (int32_t)(mbase_control_block->k_il * temp)/10;
    
    //视在功率
    mbase_control_block->apparent_power = mbase_control_block->current_l * mbase_control_block->voltage_ac/1000;
    
    //读计量芯片功率
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , POWERPA , 4 , &temp);
    mbase_control_block->active_power = (int32_t)(mbase_control_block->k_p * temp);
    
    if(mbase_control_block->active_power < 0)
    {
        mbase_control_block->power_dir = 0;
    }
    else
    {
        mbase_control_block->power_dir = 1;
    }
    
    if(abs(mbase_control_block->current_l) < MIN_CURRENT)
    {
        mbase_control_block->current_l = 0;
        mbase_control_block->active_power = 0;
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
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , UFREQ , 2 , &temp);
    mbase_control_block->frequency = (uint32_t)(44744312.5/temp);
    
    //读计量芯片零线电流
    temp = 0;
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IBRMS , 3 , &temp);
    mbase_control_block->current_n = (int32_t)(mbase_control_block->k_in * temp);
    if(abs(mbase_control_block->current_n) < MIN_CURRENT)
    {
        mbase_control_block->current_n = 0;
    }   
    
    if(DISPLAY_KEY_GET_DIN() == 0)
    {
        key_cnt++;
    }
    if(key_cnt == 3)
    {
        meterchip_adj_self(instance, pipeline);
    }
    
    //計算功率因數
    if((mbase_control_block->voltage_ac != 0) && (mbase_control_block->current_l != 0))
    {
        mbase_control_block->power_factor = mbase_control_block->active_power / (mbase_control_block->voltage_ac * mbase_control_block->current_l);
    }
    else
    {
        mbase_control_block->power_factor = 1000;
    }
    
    /****************** 补偿功能 ******************/
    heat_compensate(instance, pipeline);    //自热补偿
    
    if((mbase_control_block->voltage_ac < 19900) || (mbase_control_block->voltage_ac > 23900))
    {
        voltage_compensate(instance, pipeline, compensate_start);
    }
    else
    {
        voltage_compensate(instance, pipeline, compensate_end);
    }
    
    /****************** 温度采样 ******************/
    if(true == mcu_adc_start_once(ADC_CHL_TEMP))
    {
        if(true == read_mcu_adc_measure_value(ADC_CHL_TEMP , (int32 *)&temp))
        {
            mbase_control_block->temperature = temp/100;
        }
    }
    
    /****************** 电池采样 ******************/
    if(true == mcu_adc_start_once(ADC_CHL_BATTER))
    {
        if(true == read_mcu_adc_measure_value(ADC_CHL_BATTER , (int32 *)&temp))
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
  
//    int32_t temp = 0;
    
    mbase_control_block->voltage_ac = 0;
    mbase_control_block->current_l = 0;
    mbase_control_block->active_power = 0;
    mbase_control_block->current_n = 0;
    mbase_control_block->frequency = 0;
    mbase_control_block->power_factor = 1000;
    
    mbase_control_block->p_pluse = 0;
    mbase_control_block->n_pluse = 0;
    
    mbase_control_block->k_u = 1.413436490e-2;
    mbase_control_block->k_il = 0.166826248;//火线
    mbase_control_block->k_in = 0.166826248;
    mbase_control_block->k_p = 0.0155295;
    
    /********* 自热 *********/
    adj_std_source.countstart = 0;
    adj_std_source.countend = 0;
    adj_std_source.lock = 0;
    
    /********* 电压补偿 *********/
    memset((uint8_t*)&emu_voltage_adj.un80_offset, 0x00, 8);
}


void Electrical_measurement_poweroff(struct instance_t* instance, struct pipeline_t* pipeline)
{

    
    
}


