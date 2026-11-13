#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "application/ports/resource_access.h"
#include "type_def.h"
#include "domain/common/status.h"
#include "fr_698.h"
#include "domain/load_control/relay_status.h"
#include "domain/events/legacy/events_applications.h"
#include "domain/metering/legacy/metering_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"

extern uint16_t do_block_static_checksum(struct instance_t* instance);

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

#define		Status_TermIdentityAuthen   0x2000  //bit10//终端身份认证状态
#define		Status_TermAuthen		    0x1000  //bit11//终端安全认证状态
#define		Status_BaoDian		0x0800  //bit12
#define		Status_Authen		0x0400  //bit13 //主站安全认证
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
	if( nd_mbase_voltage_battery(mbase_cb) < 300 ) //电池电压低
		nd_mbase_run_state(mbase_cb)[1] |= Status_BatLow;
	else
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_BatLow;
	
	if( nd_mbase_power_dir(mbase_cb) == 1 ) //有功功率反向
	{
		nd_mbase_run_state(mbase_cb)[1] |= Status_Reverse;
		nd_mbase_run_state(mbase_cb)[2] |= Status_Negative;
	}
	else
	{
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_Reverse;
		nd_mbase_run_state(mbase_cb)[2] &= ~Status_Negative;
	}
	
	//if (true == get_activeflag(instance, SwitchErrS)) 
    if(nd_mbase_SwitchErrS_status(mbase_cb) == 1)//控制回路错误
		nd_mbase_run_state(mbase_cb)[1] |= Status_RelayErr;
	else
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_RelayErr;
	
	if( nd_mbase_esam_chip_abnormal(mbase_cb) == ND_EVENT_SET ) //ESAM错误
		nd_mbase_run_state(mbase_cb)[1] |= Status_EsamErr;
	else
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_EsamErr;
	
	if( DEVICE_ERR & BIT1 ) //存储器故障
		nd_mbase_run_state(mbase_cb)[1] |= Status_E2promErr;
	else
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_E2promErr;
	
	if( nd_mbase_RTCErrS_status(mbase_cb) == 1 ) //时钟故障
		nd_mbase_run_state(mbase_cb)[1] |= Status_ClockErr;
	else
		nd_mbase_run_state(mbase_cb)[1] &= ~Status_ClockErr;
	

    /***************** 运行状态字4 *****************/
	if (true == get_activeflag(instance, OverCurrentAS))  //过流
		nd_mbase_run_state(mbase_cb)[4] |= Status_ImaxOver;
	else
		nd_mbase_run_state(mbase_cb)[4] &= ~Status_ImaxOver;  
    
    /***************** 运行状态字3 *****************/
	if( nd_mbase_infrared_auth_timeout(mbase_cb) > 0 )    //红外认证
		nd_mbase_run_state(mbase_cb)[3] |= Status_IrAuthen;
	else
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_IrAuthen;

    if( nd_mbase_term_auth_timeout(mbase_cb) > 0 )    //终端身份认证
		nd_mbase_run_state(mbase_cb)[3] |= Status_TermIdentityAuthen;
	else
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_TermIdentityAuthen;
    
    if((fr_698_current_connect_status_get(instance, NULL, 1)== true) )    //终端安全认证
		nd_mbase_run_state(mbase_cb)[3] |= Status_TermAuthen;
	else
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_TermAuthen;
    
    if((fr_698_current_connect_status_get(instance, NULL, 0)== true) || (nd_mbase_auth_timeout_645(mbase_cb)) )    //主站安全认证 应用链接
		nd_mbase_run_state(mbase_cb)[3] |= Status_Authen;
	else
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_Authen;
    
    //if(nd_mbase_sta(mbase_cb) & 0x18)	    //预跳闸
    if(nd_mbase_sta(mbase_cb) & F_RemoteAlarm)
		nd_mbase_run_state(mbase_cb)[3] |= Status_RelayAlm;
	else
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_RelayAlm;	
    
	if( nd_mbase_sta(mbase_cb) & REMOTE_CMD_REALAY_OFF_STA )  //继电器命令状态
		nd_mbase_run_state(mbase_cb)[3] |= Status_RelayCmd;
	else
	   	nd_mbase_run_state(mbase_cb)[3] &= ~Status_RelayCmd;
    
    if(nd_mbase_sta(mbase_cb) & 0x01)    //保电
    {
        nd_mbase_run_state(mbase_cb)[3] |= Status_BaoDian;
        nd_mbase_run_state(mbase_cb)[3] &= ~Status_RelayCmd;
    }
    else
        nd_mbase_run_state(mbase_cb)[3] &= ~Status_BaoDian;
    
	if( nd_relay_status_is_off()) //继电器状态
		nd_mbase_run_state(mbase_cb)[3] |= Status_RelayOff;
	else 
		nd_mbase_run_state(mbase_cb)[3] &= ~Status_RelayOff;
    
//    nd_mbase_run_state(mbase_cb)[3] |= Status_KaiHuBen;  //本地未开户
//    nd_mbase_run_state(mbase_cb)[3] |= Status_KaiHuYuan; //远程未开户
    
    /***************** 运行状态字7 *****************/
	if (true == get_activeflag(instance, CoverOpenS))   //开表盖
		nd_mbase_run_state(mbase_cb)[7] |= Status_Open;
	else
		nd_mbase_run_state(mbase_cb)[7] &= ~Status_Open;
    
    if (true == get_activeflag(instance, PowerOffS))   //掉电
		nd_mbase_run_state(mbase_cb)[7] |= Status_Poweroff;
	else
		nd_mbase_run_state(mbase_cb)[7] &= ~Status_Poweroff;
}

//static 参数校验
int16_t Check_para_Pro(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb;
    uint32_t check_sum[2] = {0,};
    uint32_t addr = 0x00,s_index;
    size_t mbase_block_static_size = nd_mbase_offset_static_section_end() - nd_mbase_offset_static_section_start() + 1;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    /******************* mbase_control_block_t *******************/
    check_sum[0] = do_block_static_checksum(instance);
    
    if(nd_mbase_static_chksum(mbase_cb) != check_sum[0]) //RAM异常
    {
        get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
        uint32_t res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom);
        nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size); //从EE主区获取，覆盖至RAM
        check_sum[0] = do_block_static_checksum(instance);
        if(nd_mbase_static_chksum(mbase_cb) != check_sum[0])    //主区效验错误
        {
            res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom_bp);
            nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size);//从EE备份区获取，覆盖至RAM
            check_sum[0] = do_block_static_checksum(instance);
            if(nd_mbase_static_chksum(mbase_cb) == check_sum[0])    //备份正确
            {
                //备份区覆盖主区   
                res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom);
                nd_set_block(instance, res,(uint8_t *)addr,mbase_block_static_size); //将RAM中的数据保存到EPROM;
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else    //主区正确，获取备份数据
        {
            res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom_bp);
            nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size);//从EE备份区获取，覆盖至RAM
            check_sum[0] = do_block_static_checksum(instance);
            if(nd_mbase_static_chksum(mbase_cb) != check_sum[0])    //备份错误
            {
                //主区覆盖备份区
                res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom_bp);
                nd_set_block(instance, res,(uint8_t *)addr,mbase_block_static_size); //将RAM中的数据保存到EPROM;
                return 0;
            }
        }
    }
    return 1;
}
