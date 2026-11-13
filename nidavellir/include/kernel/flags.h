#ifndef ND_KERNEL_FLAGS_H
#define ND_KERNEL_FLAGS_H


#define  MSG_SECOND         0x0001
#define  MSG_PAGE_KEY_UP    0x0002
#define  MSG_POWERON        0x0003
#define  MSG_POWEROFF       0x0004
#define  MSG_RS485_BYTE_TIMEOUT   0x0005
#define  MSG_PLC_BYTE_TIMEOUT     0x0006
#define  MSG_IR_BYTE_TIMEOUT      0x0007
#define  MSG_CARD_INSERT          0x0008
#define  MSG_CARD_PULLOUT         0x0009





//Flag.Power
//Flag.Key
//#define		F_KeyDisp0			0x01
//#define		F_KeyDisp1			0x02
#define		F_KeyDispay			0x04
#define		F_KeyAwake			0x08
//Flag.Disp
#define		F_DispStop			0x01
#define		F_DispOff			0x02
#define		F_DispUpdate		0x04
#define		F_DispFlash			0x08
#define		F_DispPause			0x10
#define		F_DispRst			0x20
#define		F_DispIr_BG			0x40
//Flag.CommDisp
#define		F_CommDisp485		0x10
#define		F_CommDispIr		0x20
#define		F_CommDispPlc		0x40
//Flag.Run
#define		F_Disp              0x01

//继电器命令状态
#define     F_RemoteNull            0x00    //合闸
#define		F_RemoteRelayOn  	    0x01	//保电
#define		F_RemoteRelayOff 	    0x02    //拉闸
#define		F_RemoteAlarm	  	    0x04    //告警
#define		F_RemoteGaoJing0	    0x08    //预跳闸
#define		F_RemoteGaoJing1	    0x10    //预跳闸
#define     F_RemoteRelayOnAllow    0x20    //合闸允许
#define     F_RemoteRelayOnDirect   0x40    //直接允许

//多功能配置字
#define     F_Plc_En                0x0001  //带模块
#define     F_Password_En           0x0002  //645明文模式
#define     F_RelayonAuto_En        0x0004  //自动合闸
#define     F_Crad_En               0x0008  //带CPU卡
#define     F_ExternRelay_En        0x0020  //外置继电器
#define     F_Compen_En             0x0800  //送检补偿

//继电器命令状态
#define REMOTE_CMD_REALAY_OFF_STA   (F_RemoteRelayOff|F_RemoteGaoJing0|F_RemoteGaoJing1)




#define     chl_no_empty    0
#define     chl_rx_empty    1
#define     chl_tx_empty    2
#define     chl_empty       3
    

//多功能口  
enum MultiPort
{
    F_PortSec       = 0,   //秒信号
    F_PortTouCut    = 2    //时段投切
};

#endif
