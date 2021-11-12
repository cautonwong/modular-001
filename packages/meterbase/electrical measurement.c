
#include "nidavellir.h"
#include "applications.h"
#include <math.h>
#include "ndvh_rn8209.h"
#include "cpu_surport.h"
#include "pin_function_def.h"

#define MAX_VOLTAGE  40000 //400v
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
void meterchip_adj_self(void)
{
    unsigned char i=0, Tofst , j=0;
    signed short Pgain;
    unsigned long TempUrms=0, TempIrms=0, TempPw=0, CalcPw=0, Corretion_Pw=0, U_value=0, I1_value=0, I2_value=0, P_value=0;
    unsigned long BaseErr,hfconst,RN8209Kpq;
    float err=0, Ku, Ka, Kb;
    
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

            //MEM_DBWrite( ADJ_Pgain0, (unsigned char*)&Adj_Block[0].Adj.Pgain, 2, Mem_E2PROM );
            //MEM_DBWrite( Meter_Ku, (unsigned char*)&Fact.Ku, 8, Mem_E2PROM );
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
            //MEM_DBWrite( ADJ_Tofst0, (unsigned char*)&Adj_Block[0].Adj.Tofst, 1, Mem_E2PROM );
            rn8209_dev.dev_driver_ops->dev_initialize(&rn8209_dev);
        }
    }
    if (Factory3.adjflag & F_ANadj)
    {
        if (mem_compare( &Factory3.Ian, Buffer, 4 ) != 0)
        {
                rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IBRMS , 3 , &I2_value);
                mbase_control_block->k_in = (float)Factory3.Ian/10/I2_value;
                //MEM_DBWrite( Meter_Kb, (unsigned char*)&Fact.Kb, 4, Mem_E2PROM );
        }
    }
}


void Electrical_measurement(struct instance_t* instance, struct pipeline_t* pipeline)
{
    
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    int32_t temp = 0;
   
    //读计量芯片电压
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , URMS , 3 , &temp);
    
    mbase_control_block->voltage_ac = temp * mbase_control_block->k_u;
    if(mbase_control_block->voltage_ac > MAX_VOLTAGE)
    {
        mbase_control_block->voltage_ac = UN_VOLTAGE;
    }
    if(mbase_control_block->voltage_ac < MIN_VOLTAGE)
    {
        mbase_control_block->voltage_ac = 0;
    }
    //读计量芯片电流
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IARMS , 3 , &temp);
    mbase_control_block->current_l = temp * mbase_control_block->k_il;
    
    //读计量芯片功率
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , POWERPA , 4 , &temp);
    mbase_control_block->active_power = temp * mbase_control_block->k_p;
    
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
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , UFREQ , 2 , &temp);
    mbase_control_block->frequency = 44744312.5/temp;
    
    //读计量芯片零线电流
    rn8209_dev.dev_driver_ops->dev_read(&rn8209_dev , IBRMS , 3 , &temp);
    mbase_control_block->current_n = temp * mbase_control_block->k_in;
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
        meterchip_adj_self();
    }
    
    
}


//void Electrical_measurement_reset(struct instance_t* instance, struct pipeline_t* pipeline)
void Electrical_measurement_reset(void)
{
    //struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
  
  struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)&ctx;
    int32_t temp = 0;
    
    mbase_control_block->voltage_ac = 0;
    mbase_control_block->current_l = 0;
    mbase_control_block->active_power = 0;
    mbase_control_block->current_n = 0;
    mbase_control_block->frequency = 0;
    mbase_control_block->power_factor = 1000;
    
    mbase_control_block->k_u = 1.413436490e-2;
    mbase_control_block->k_il = 0.166826248;//火线
    mbase_control_block->k_in = 0.166826248;
    mbase_control_block->k_p = 0.0155295;
    
    DISPLAY_KEY_INPUT();
    
    key_cnt = 0;
}






