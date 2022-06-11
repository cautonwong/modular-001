#include "nidavellir.h"
#include "applications.h"
#include "fr_698.h"
#include "pin_function_def.h"

//bit0~bit15
//MeterRun1
#define		Status_BatLow		0x0020  //bit2		
#define		Status_Reverse		0x0008  //bit4
#define		Status_MeterIcErr	0x0000  //bit6
#define		Status_RelayErr	    0x8000  //bit8
#define		Status_EsamErr		0x4000  //bit9
#define		Status_E2promErr	0x0400  //bit13
#define		Status_TouZhi		0x0200  //bit14
#define		Status_ClockErr	    0x0100  //bit15
//MeterRun2
#define		Status_Negative	    0x0080  //bit0
//MeterRun4
#define		Status_ImaxOver	    0x0008  //bit4
#define		Status_Rev		    0x0002  //bit6
//MeterRun3
#define		Status_IrAuthen		0x0010  //bit3
#define		Status_RelayOff	    0x0008  //bit4
#define		Status_RelayCmd     0x0002  //bit6
#define		Status_RelayAlm     0x0001  //bit7
#define		Status_BaoDian		0x0800  //bit12
#define		Status_Authen		0x0400  //bit13
#define		Status_KaiHuBen	    0x0200  //bit14
#define		Status_KaiHuYuan	0x0100  //bit15
//MeterRun7
#define		Status_Poweroff	    0x0004  //bit5
#define		Status_Open		    0x4000  //bit9

extern uint8_t DEVICE_ERR;

void Meter_Run_State(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    /***************** 运行状态字1.2 *****************/
	if( mbase_cb->voltage_battery < 300 ) //电池电压低
		mbase_cb->run_state[1] |= Status_BatLow;
	else
		mbase_cb->run_state[1] &= ~Status_BatLow;
	
	if( mbase_cb->power_dir == 1 ) //有功功率反向
	{
		mbase_cb->run_state[1] |= Status_Reverse;
		mbase_cb->run_state[2] |= Status_Negative;
	}
	else
	{
		mbase_cb->run_state[1] &= ~Status_Reverse;
		mbase_cb->run_state[2] &= ~Status_Negative;
	}
	
	//if (true == get_activeflag(instance, SwitchErrS)) 
    if(mbase_cb->SwitchErrS_status == 1)//控制回路错误
		mbase_cb->run_state[1] |= Status_RelayErr;
	else
		mbase_cb->run_state[1] &= ~Status_RelayErr;
	
	if( mbase_cb->esam_chip_abnormal == EVENT_SET ) //ESAM错误
		mbase_cb->run_state[1] |= Status_EsamErr;
	else
		mbase_cb->run_state[1] &= ~Status_EsamErr;
	
	if( DEVICE_ERR & BIT1 ) //存储器故障
		mbase_cb->run_state[1] |= Status_E2promErr;
	else
		mbase_cb->run_state[1] &= ~Status_E2promErr;
	
	if( mbase_cb->RTCErrS_status == 1 ) //时钟故障
		mbase_cb->run_state[1] |= Status_ClockErr;
	else
		mbase_cb->run_state[1] &= ~Status_ClockErr;
	

    /***************** 运行状态字4 *****************/
	if (true == get_activeflag(instance, OverCurrentAS))  //过流
		mbase_cb->run_state[4] |= Status_ImaxOver;
	else
		mbase_cb->run_state[4] &= ~Status_ImaxOver;
    
	if( mbase_cb->power_dir == 1 ) //有功功率反向
		mbase_cb->run_state[4] |= Status_Rev;
	else
		mbase_cb->run_state[4] &= ~Status_Rev;
    
    
    /***************** 运行状态字3 *****************/
	if( mbase_cb->irauthen_tmr > 0 )    //红外认证
		mbase_cb->run_state[3] |= Status_IrAuthen;
	else
		mbase_cb->run_state[3] &= ~Status_IrAuthen;
    
    if( mbase_cb->authen_tmr > 0 )    //应用链接
		mbase_cb->run_state[3] |= Status_Authen;
	else
		mbase_cb->run_state[3] &= ~Status_Authen;
    
    if(mbase_cb->sta & 0x01)    //保电
        mbase_cb->run_state[3] |= Status_BaoDian;
    else
        mbase_cb->run_state[3] &= ~Status_BaoDian;
    
    if(mbase_cb->sta & 0x18)	    //预跳闸
		mbase_cb->run_state[3] |= Status_RelayAlm;
	else
		mbase_cb->run_state[3] &= ~Status_RelayAlm;	
    
	if( mbase_cb->sta & 0x02 )  //继电器命令状态
		mbase_cb->run_state[3] |= Status_RelayCmd;
	else
	   	mbase_cb->run_state[3] &= ~Status_RelayCmd;
    
	if( RELAY_CHECK() == 1) //继电器状态
		mbase_cb->run_state[3] |= Status_RelayOff;
	else 
		mbase_cb->run_state[3] &= ~Status_RelayOff;
    
    mbase_cb->run_state[3] |= Status_KaiHuBen;  //本地未开户
	
    mbase_cb->run_state[3] |= Status_KaiHuYuan; //远程未开户
    
    
    /***************** 运行状态字7 *****************/
	if (true == get_activeflag(instance, CoverOpenS))   //开表盖
		mbase_cb->run_state[7] |= Status_Open;
	else
		mbase_cb->run_state[7] &= ~Status_Open;
    
    if (true == get_activeflag(instance, PowerOffS))   //掉电
		mbase_cb->run_state[7] |= Status_Poweroff;
	else
		mbase_cb->run_state[7] &= ~Status_Poweroff;
}