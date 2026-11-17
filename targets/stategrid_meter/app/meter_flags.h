#ifndef ND_TARGET_STATEGRID_METER_FLAGS_H
#define ND_TARGET_STATEGRID_METER_FLAGS_H

/* Event and relay command bits used by the product. Values are protocol
 * constants inherited from the retired kernel flags header. */
#define MSG_SECOND              0x0001
#define MSG_PAGE_KEY_UP         0x0002
#define MSG_POWERON             0x0003
#define MSG_POWEROFF            0x0004
#define MSG_RS485_BYTE_TIMEOUT  0x0005
#define MSG_PLC_BYTE_TIMEOUT    0x0006
#define MSG_IR_BYTE_TIMEOUT     0x0007
#define MSG_CARD_INSERT         0x0008
#define MSG_CARD_PULLOUT        0x0009

#define F_KeyDispay             0x04
#define F_KeyAwake              0x08

#define F_DispStop              0x01
#define F_DispOff               0x02
#define F_DispUpdate            0x04
#define F_DispFlash             0x08
#define F_DispPause             0x10
#define F_DispRst               0x20
#define F_DispIr_BG             0x40
#define F_CommDisp485           0x10
#define F_CommDispIr            0x20
#define F_CommDispPlc           0x40
#define F_Disp                  0x01

#define F_RemoteNull            0x00
#define F_RemoteRelayOn         0x01
#define F_RemoteRelayOff        0x02
#define F_RemoteAlarm           0x04
#define F_RemoteGaoJing0        0x08
#define F_RemoteGaoJing1        0x10
#define F_RemoteRelayOnAllow    0x20
#define F_RemoteRelayOnDirect   0x40

#define F_Plc_En                0x0001
#define F_Password_En           0x0002
#define F_RelayonAuto_En        0x0004
#define F_Crad_En               0x0008
#define F_ExternRelay_En        0x0020
#define F_Compen_En             0x0800

#define REMOTE_CMD_REALAY_OFF_STA \
    (F_RemoteRelayOff | F_RemoteGaoJing0 | F_RemoteGaoJing1)

#define chl_no_empty    0
#define chl_rx_empty    1
#define chl_tx_empty    2
#define chl_empty       3

enum MultiPort
{
    F_PortSec = 0,
    F_PortTouCut = 2
};

#endif
